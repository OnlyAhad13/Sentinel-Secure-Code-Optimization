/**
 * @file vector_store.cpp
 * @brief Implementation of SIMD-accelerated vector store
 */

#include "../include/vector_store.hpp"
#include <cstring>
#include <limits>

namespace sentinel {

// ============================================================================
// Construction
// ============================================================================

VectorStore::VectorStore(size_t dimension) : dimension_(dimension) {
  if (dimension == 0) {
    throw std::invalid_argument("Vector dimension must be greater than 0");
  }
}

// ============================================================================
// CPU Feature Detection
// ============================================================================

bool VectorStore::has_avx2_support() {
#ifdef __AVX2__
#if defined(__GNUC__) || defined(__clang__)
  return __builtin_cpu_supports("avx2");
#elif defined(_MSC_VER)
  int cpuInfo[4];
  __cpuid(cpuInfo, 7);
  return (cpuInfo[1] & (1 << 5)) != 0; // AVX2 is bit 5 of EBX
#else
  return false;
#endif
#else
  return false;
#endif
}

// ============================================================================
// Vector Operations
// ============================================================================

void VectorStore::add_vector(const std::string &id,
                             const std::vector<float> &vector) {
  if (vector.size() != dimension_) {
    throw std::invalid_argument("Vector dimension mismatch: expected " +
                                std::to_string(dimension_) + ", got " +
                                std::to_string(vector.size()));
  }

  // Store vector (contiguous memory for SIMD access)
  vectors_.push_back(vector);
  ids_.push_back(id);

  // Precompute L2 norm for cosine similarity
  // This avoids redundant computation during search
  norms_.push_back(compute_norm(vector));

  // Index needs rebuild after adding vectors
  index_built_ = false;
}

void VectorStore::build_index() {
  // Currently a no-op for brute-force search
  // Future: Build IVF clusters or HNSW graph here
  index_built_ = true;
}

// ============================================================================
// Search Implementation
// ============================================================================

std::vector<SearchResult> VectorStore::search(const std::vector<float> &query,
                                              size_t k) const {
  if (query.size() != dimension_) {
    throw std::invalid_argument("Query dimension mismatch: expected " +
                                std::to_string(dimension_) + ", got " +
                                std::to_string(query.size()));
  }

  if (vectors_.empty()) {
    return {};
  }

  // Precompute query norm once
  float query_norm = compute_norm(query);

  // Handle zero-norm query (all zeros)
  if (query_norm < std::numeric_limits<float>::epsilon()) {
    // Return first k results with 0 similarity
    std::vector<SearchResult> results;
    size_t count = std::min(k, vectors_.size());
    for (size_t i = 0; i < count; ++i) {
      results.push_back({ids_[i], 0.0f});
    }
    return results;
  }

  // Min-heap to track top-k results
  // We keep the smallest score at top, so we can efficiently evict
  // when a better candidate is found
  std::priority_queue<SearchResult, std::vector<SearchResult>,
                      std::greater<SearchResult>>
      min_heap;

  // Scan all vectors
  for (size_t i = 0; i < vectors_.size(); ++i) {
    float score = cosine_similarity(query, vectors_[i], query_norm, norms_[i]);

    if (min_heap.size() < k) {
      // Haven't filled k results yet, add unconditionally
      min_heap.push({ids_[i], score});
    } else if (score > min_heap.top().score) {
      // Found a better result, replace the worst one
      min_heap.pop();
      min_heap.push({ids_[i], score});
    }
  }

  // Extract results in descending order (best first)
  std::vector<SearchResult> results;
  results.reserve(min_heap.size());

  while (!min_heap.empty()) {
    results.push_back(min_heap.top());
    min_heap.pop();
  }

  // Reverse to get descending order (heap gave us ascending)
  std::reverse(results.begin(), results.end());

  return results;
}

// ============================================================================
// Similarity Computation
// ============================================================================

float VectorStore::compute_norm(const std::vector<float> &vec) {
  float sum = 0.0f;
  for (float v : vec) {
    sum += v * v;
  }
  return std::sqrt(sum);
}

float VectorStore::cosine_similarity(const std::vector<float> &a,
                                     const std::vector<float> &b, float norm_a,
                                     float norm_b) const {
  // Handle zero-norm vectors
  if (norm_a < std::numeric_limits<float>::epsilon() ||
      norm_b < std::numeric_limits<float>::epsilon()) {
    return 0.0f;
  }

  float dot;

#ifdef __AVX2__
  if (has_avx2_support()) {
    dot = dot_product_avx2(a.data(), b.data(), dimension_);
  } else {
    dot = dot_product_scalar(a.data(), b.data(), dimension_);
  }
#else
  dot = dot_product_scalar(a.data(), b.data(), dimension_);
#endif

  return dot / (norm_a * norm_b);
}

// ============================================================================
// SIMD Dot Product (AVX2)
// ============================================================================

float VectorStore::dot_product_avx2(const float *a, const float *b,
                                    size_t size) {
#ifdef __AVX2__
  /*
   * AVX2 Strategy:
   * - Process 8 floats (256 bits) per iteration
   * - Use FMA (Fused Multiply-Add) for precision and speed
   * - Accumulate in 4 separate accumulators to hide latency
   * - Horizontal sum for final reduction
   *
   * Memory Access Pattern:
   * - Unaligned loads (_mm256_loadu_ps) for flexibility
   * - Sequential access maximizes cache line utilization
   */

  __m256 sum0 = _mm256_setzero_ps();
  __m256 sum1 = _mm256_setzero_ps();
  __m256 sum2 = _mm256_setzero_ps();
  __m256 sum3 = _mm256_setzero_ps();

  size_t i = 0;

  // Process 32 floats per iteration (4 AVX registers × 8 floats)
  // Using 4 accumulators to hide instruction latency
  for (; i + 31 < size; i += 32) {
    __m256 a0 = _mm256_loadu_ps(a + i);
    __m256 b0 = _mm256_loadu_ps(b + i);
    sum0 = _mm256_fmadd_ps(a0, b0, sum0);

    __m256 a1 = _mm256_loadu_ps(a + i + 8);
    __m256 b1 = _mm256_loadu_ps(b + i + 8);
    sum1 = _mm256_fmadd_ps(a1, b1, sum1);

    __m256 a2 = _mm256_loadu_ps(a + i + 16);
    __m256 b2 = _mm256_loadu_ps(b + i + 16);
    sum2 = _mm256_fmadd_ps(a2, b2, sum2);

    __m256 a3 = _mm256_loadu_ps(a + i + 24);
    __m256 b3 = _mm256_loadu_ps(b + i + 24);
    sum3 = _mm256_fmadd_ps(a3, b3, sum3);
  }

  // Process remaining 8-float chunks
  for (; i + 7 < size; i += 8) {
    __m256 av = _mm256_loadu_ps(a + i);
    __m256 bv = _mm256_loadu_ps(b + i);
    sum0 = _mm256_fmadd_ps(av, bv, sum0);
  }

  // Combine all accumulators
  sum0 = _mm256_add_ps(sum0, sum1);
  sum2 = _mm256_add_ps(sum2, sum3);
  sum0 = _mm256_add_ps(sum0, sum2);

  // Horizontal sum of the 8 floats in sum0
  // Step 1: Add high 128 bits to low 128 bits
  __m128 high = _mm256_extractf128_ps(sum0, 1);
  __m128 low = _mm256_castps256_ps128(sum0);
  __m128 sum128 = _mm_add_ps(high, low);

  // Step 2: Horizontal add within 128-bit register
  sum128 = _mm_hadd_ps(sum128, sum128);
  sum128 = _mm_hadd_ps(sum128, sum128);

  float result = _mm_cvtss_f32(sum128);

  // Process remaining elements with scalar code
  for (; i < size; ++i) {
    result += a[i] * b[i];
  }

  return result;
#else
  // Fallback to scalar if AVX2 not compiled in
  return dot_product_scalar(a, b, size);
#endif
}

// ============================================================================
// Scalar Fallback
// ============================================================================

float VectorStore::dot_product_scalar(const float *a, const float *b,
                                      size_t size) {
  /*
   * Scalar Implementation Notes:
   * - Uses 4-way loop unrolling for instruction-level parallelism
   * - Compiler can often auto-vectorize this with -O3
   * - Serves as fallback for non-AVX2 CPUs and verification
   */

  float sum0 = 0.0f, sum1 = 0.0f, sum2 = 0.0f, sum3 = 0.0f;
  size_t i = 0;

  // Process 4 elements per iteration
  for (; i + 3 < size; i += 4) {
    sum0 += a[i] * b[i];
    sum1 += a[i + 1] * b[i + 1];
    sum2 += a[i + 2] * b[i + 2];
    sum3 += a[i + 3] * b[i + 3];
  }

  // Process remaining elements
  float result = sum0 + sum1 + sum2 + sum3;
  for (; i < size; ++i) {
    result += a[i] * b[i];
  }

  return result;
}

} // namespace sentinel
