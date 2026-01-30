"""
Test script for Sentinel Core VectorStore.

This script verifies:
1. Module import works
2. Vector store creation succeeds
3. Vector addition and search work correctly
4. AVX2 detection is functioning
5. Results are sorted by descending similarity
"""

import sys
import time
from typing import List

# Try to use numpy for test data generation
try:
    import numpy as np
    HAS_NUMPY = True
except ImportError:
    import random
    HAS_NUMPY = False
    print("NumPy not available, using random module for test data")


def generate_random_vector(dim: int) -> List[float]:
    """Generate a random unit vector."""
    if HAS_NUMPY:
        vec = np.random.randn(dim).astype(np.float32)
        vec = vec / np.linalg.norm(vec)  # Normalize
        return vec.tolist()
    else:
        vec = [random.gauss(0, 1) for _ in range(dim)]
        norm = sum(x * x for x in vec) ** 0.5
        return [x / norm for x in vec]


def test_import() -> None:
    """Test that the module can be imported."""
    print("Test 1: Module import...", end=" ")
    import sentinel_core
    print(f"OK (version: {sentinel_core.__doc__[:50]}...)")


def test_avx2_detection() -> None:
    """Test AVX2 CPU feature detection."""
    print("Test 2: AVX2 detection...", end=" ")
    import sentinel_core
    has_avx2 = sentinel_core.has_avx2()
    print(f"OK (AVX2 available: {has_avx2})")


def test_create_store() -> None:
    """Test vector store creation."""
    print("Test 3: Create store...", end=" ")
    import sentinel_core
    
    store = sentinel_core.VectorStore(128)
    assert store.dimension == 128
    assert store.size == 0
    print("OK")


def test_add_vectors() -> None:
    """Test adding vectors to the store."""
    print("Test 4: Add vectors...", end=" ")
    import sentinel_core
    
    dim = 128
    store = sentinel_core.VectorStore(dim)
    
    # Add 100 vectors
    for i in range(100):
        vec = generate_random_vector(dim)
        store.add_vector(f"doc_{i}", vec)
    
    assert store.size == 100
    print("OK")


def test_search() -> None:
    """Test similarity search."""
    print("Test 5: Search...", end=" ")
    import sentinel_core
    
    dim = 128
    store = sentinel_core.VectorStore(dim)
    
    # Add vectors
    vectors = []
    for i in range(100):
        vec = generate_random_vector(dim)
        vectors.append(vec)
        store.add_vector(f"doc_{i}", vec)
    
    store.build_index()
    
    # Search with the first vector (should find itself with score ~1.0)
    results = store.search(vectors[0], 5)
    
    assert len(results) == 5
    assert results[0].id == "doc_0"  # Should find itself
    assert results[0].score > 0.99   # Should be very similar (almost exact)
    
    # Verify descending order
    for i in range(len(results) - 1):
        assert results[i].score >= results[i + 1].score
    
    print("OK")


def test_dimension_mismatch() -> None:
    """Test that dimension mismatch raises an error."""
    print("Test 6: Dimension mismatch...", end=" ")
    import sentinel_core
    
    store = sentinel_core.VectorStore(128)
    
    try:
        store.add_vector("bad", [1.0, 2.0, 3.0])  # Wrong dimension
        print("FAIL (should have raised ValueError)")
        sys.exit(1)
    except ValueError:
        pass
    
    print("OK")


def test_performance() -> None:
    """Benchmark search performance."""
    print("Test 7: Performance benchmark...", end=" ")
    import sentinel_core
    
    dim = 768  # Typical embedding dimension
    n_vectors = 10000
    k = 10
    
    store = sentinel_core.VectorStore(dim)
    
    # Add vectors
    print(f"\n  - Adding {n_vectors} vectors of dim {dim}...", end=" ")
    start = time.perf_counter()
    for i in range(n_vectors):
        vec = generate_random_vector(dim)
        store.add_vector(f"doc_{i}", vec)
    add_time = time.perf_counter() - start
    print(f"{add_time:.2f}s")
    
    store.build_index()
    
    # Benchmark search
    print(f"  - Searching (k={k})...", end=" ")
    query = generate_random_vector(dim)
    
    # Warm-up
    _ = store.search(query, k)
    
    # Timed run (average of 100 queries)
    n_queries = 100
    start = time.perf_counter()
    for _ in range(n_queries):
        _ = store.search(query, k)
    search_time = (time.perf_counter() - start) / n_queries * 1000
    
    print(f"{search_time:.2f}ms/query")
    print(f"  - Throughput: {1000 / search_time:.0f} queries/sec")


def main() -> None:
    """Run all tests."""
    print("=" * 60)
    print("Sentinel Core - Test Suite")
    print("=" * 60)
    print()
    
    tests = [
        test_import,
        test_avx2_detection,
        test_create_store,
        test_add_vectors,
        test_search,
        test_dimension_mismatch,
        test_performance,
    ]
    
    for test in tests:
        try:
            test()
        except Exception as e:
            print(f"FAIL ({e})")
            import traceback
            traceback.print_exc()
            sys.exit(1)
    
    print()
    print("=" * 60)
    print("All tests passed!")
    print("=" * 60)


if __name__ == "__main__":
    main()
