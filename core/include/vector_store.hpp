/**
 * @file vector_store.hpp
 * @brief High-performance vector database with AVX2 SIMD acceleration
 *
 * Memory Layout Design:
 * ---------------------
 * We use parallel arrays for cache-efficient sequential access:
 * - vectors_: Each vector stored contiguously for SIMD streaming
 * - ids_: Metadata IDs at matching indices (avoids struct padding)
 * - norms_: Precomputed L2 norms for fast cosine similarity
 *
 * The cosine similarity formula is: dot(a,b) / (||a|| * ||b||)
 * By precomputing ||v|| at insert time, search only needs dot product +
 * division.
 *
 * SIMD Strategy:
 * - AVX2 processes 8 floats (256 bits) per instruction
 * - FMA (Fused Multiply-Add) for dot product accumulation
 * - 32-byte alignment suggested for optimal AVX2 performance
 * - Runtime detection falls back to scalar if AVX2 unavailable
 */

#ifndef SENTINEL_VECTOR_STORE_HPP
#define SENTINEL_VECTOR_STORE_HPP

#include <algorithm>
#include <cmath>
#include <queue>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#ifdef __AVX2__
#include <immintrin.h>
#endif

namespace sentinel {

/**
 * @brief Result of a similarity search
 */
struct SearchResult {
  std::string id;
  float score;

  // For min-heap with std::greater: smallest score at top
  bool operator<(const SearchResult &other) const {
    return score < other.score;
  }

  bool operator>(const SearchResult &other) const {
    return score > other.score;
  }
};

/**
 * @brief High-performance vector store with SIMD-accelerated similarity search
 *
 * This class provides:
 * - O(1) vector insertion with automatic norm computation
 * - O(N*D) brute-force search with AVX2 acceleration
 * - Top-k retrieval using a min-heap for memory efficiency
 *
 * @note Vectors must all have the same dimension (set at construction)
 */
class VectorStore {
public:
  /**
   * @brief Construct a vector store for fixed-dimension vectors
   * @param dimension The dimension of all vectors to be stored
   * @throws std::invalid_argument if dimension is 0
   */
  explicit VectorStore(size_t dimension);

  /**
   * @brief Add a vector with associated metadata ID
   *
   * The vector is copied and its L2 norm is precomputed for fast search.
   *
   * @param id Unique identifier for this vector
   * @param vector The embedding vector (must match store dimension)
   * @throws std::invalid_argument if vector dimension doesn't match
   */
  void add_vector(const std::string &id, const std::vector<float> &vector);

  /**
   * @brief Build the search index (placeholder for future IVF/HNSW)
   *
   * Currently a no-op for brute-force search. Will be extended for
   * approximate nearest neighbor algorithms.
   */
  void build_index();

  /**
   * @brief Search for the k most similar vectors
   *
   * Uses cosine similarity with AVX2 acceleration when available.
   * Results are sorted by similarity score (highest first).
   *
   * @param query The query vector
   * @param k Number of results to return
   * @return Vector of (id, score) pairs, sorted by descending similarity
   * @throws std::invalid_argument if query dimension doesn't match
   */
  std::vector<SearchResult> search(const std::vector<float> &query,
                                   size_t k) const;

  /**
   * @brief Get the number of vectors in the store
   */
  size_t size() const { return vectors_.size(); }

  /**
   * @brief Get the vector dimension
   */
  size_t dimension() const { return dimension_; }

  /**
   * @brief Check if AVX2 is available at runtime
   */
  static bool has_avx2_support();

private:
  size_t dimension_;

  // Parallel arrays for cache-efficient access during search
  // Layout: vectors_[i] corresponds to ids_[i] and norms_[i]
  std::vector<std::vector<float>> vectors_;
  std::vector<std::string> ids_;
  std::vector<float> norms_; // Precomputed L2 norms

  bool index_built_ = false;

  /**
   * @brief Compute L2 norm of a vector
   */
  static float compute_norm(const std::vector<float> &vec);

  /**
   * @brief Compute cosine similarity between two vectors
   *
   * Dispatches to AVX2 or scalar implementation based on CPU support.
   *
   * @param a First vector
   * @param b Second vector
   * @param norm_a Precomputed norm of a
   * @param norm_b Precomputed norm of b
   * @return Cosine similarity in range [-1, 1]
   */
  float cosine_similarity(const std::vector<float> &a,
                          const std::vector<float> &b, float norm_a,
                          float norm_b) const;

  /**
   * @brief AVX2-optimized dot product
   *
   * Processes 8 floats per iteration using 256-bit SIMD registers.
   * Uses FMA (Fused Multiply-Add) for better precision and performance.
   */
  static float dot_product_avx2(const float *a, const float *b, size_t size);

  /**
   * @brief Scalar fallback for dot product
   */
  static float dot_product_scalar(const float *a, const float *b, size_t size);
};

} // namespace sentinel

#endif // SENTINEL_VECTOR_STORE_HPP
