<<<<<<< HEAD
# Sentinel-Secure-Code-Optimization
An autonomous AI agent that detects security vulnerabilities and performance bottlenecks in large codebases. Unlike standard tools, it uses a custom-built C++ Vector Database for millisecond-latency retrieval and a Fine-Tuned Llama-3 model to autonomously patch code, verify fixes, and submit pull requests.
=======
# Sentinel Core

High-performance vector search engine with AVX2 SIMD acceleration.

## Quick Start

```bash
# Install dependencies
pip install pybind11 numpy

# Build and install
pip install .

# Run tests
python tests/test_vector_store.py
```

## Usage

```python
import sentinel_core
import numpy as np

# Create a store for 768-dimensional vectors (e.g., BERT embeddings)
store = sentinel_core.VectorStore(768)

# Add vectors with IDs
for doc_id, embedding in your_embeddings:
    store.add_vector(doc_id, embedding.tolist())

# Build the index
store.build_index()

# Search for similar vectors
query = np.random.randn(768).astype(np.float32).tolist()
results = store.search(query, k=10)

for r in results:
    print(f"{r.id}: {r.score:.4f}")
```

## Features

- **AVX2 SIMD Acceleration**: 8x parallel float processing
- **Precomputed Norms**: Fast cosine similarity computation
- **Min-Heap Top-K**: Memory-efficient result tracking
- **Python Bindings**: Seamless integration via pybind11

## Build from Source

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```
>>>>>>> 4e05630 (Updated Vector DB)
