/**
 * @file bindings.cpp
 * @brief pybind11 Python bindings for the Sentinel VectorStore
 *
 * Exposes the C++ VectorStore class to Python with:
 * - Automatic type conversion (std::vector <-> Python list)
 * - NumPy array support via buffer protocol
 * - Exception translation (C++ exceptions -> Python exceptions)
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h> // For automatic std::vector, std::string conversion

#include "vector_store.hpp"

namespace py = pybind11;

PYBIND11_MODULE(sentinel_core, m) {
  m.doc() = R"pbdoc(
        Sentinel Core - High-Performance Vector Search Engine
        ======================================================
        
        A SIMD-accelerated vector database for semantic search.
        
        Example:
            >>> import sentinel_core
            >>> store = sentinel_core.VectorStore(128)  # 128-dimensional vectors
            >>> store.add_vector("doc1", [0.1] * 128)
            >>> store.build_index()
            >>> results = store.search([0.1] * 128, k=5)
            >>> for r in results:
            ...     print(f"{r.id}: {r.score}")
    )pbdoc";

  // Expose SearchResult struct
  py::class_<sentinel::SearchResult>(
      m, "SearchResult",
      "Result from a similarity search containing the document ID and "
      "similarity score.")
      .def_readonly("id", &sentinel::SearchResult::id,
                    "The unique identifier of the matched document")
      .def_readonly(
          "score", &sentinel::SearchResult::score,
          "Cosine similarity score in range [-1, 1], higher is more similar")
      .def("__repr__", [](const sentinel::SearchResult &r) {
        return "SearchResult(id='" + r.id +
               "', score=" + std::to_string(r.score) + ")";
      });

  // Expose VectorStore class
  py::class_<sentinel::VectorStore>(m, "VectorStore",
                                    R"pbdoc(
            High-performance vector store with SIMD-accelerated similarity search.
            
            The store uses AVX2 instructions (when available) for fast cosine similarity
            computation. Vectors are stored with precomputed L2 norms for efficiency.
            
            Args:
                dimension: The dimension of all vectors to be stored (must be > 0)
            
            Raises:
                ValueError: If dimension is 0
        )pbdoc")

      .def(py::init<size_t>(), py::arg("dimension"),
           "Create a new vector store for the specified dimension")

      .def("add_vector", &sentinel::VectorStore::add_vector, py::arg("id"),
           py::arg("vector"),
           R"pbdoc(
                Add a vector with an associated metadata ID.
                
                Args:
                    id: Unique identifier for this vector (string)
                    vector: List of floats representing the embedding
                
                Raises:
                    ValueError: If vector dimension doesn't match store dimension
            )pbdoc")

      .def("build_index", &sentinel::VectorStore::build_index,
           R"pbdoc(
                Build the search index.
                
                Currently a no-op for brute-force search. Will be required for
                approximate nearest neighbor search in future versions.
            )pbdoc")

      .def("search", &sentinel::VectorStore::search, py::arg("query"),
           py::arg("k"),
           R"pbdoc(
                Search for the k most similar vectors.
                
                Uses cosine similarity with AVX2 acceleration when available.
                
                Args:
                    query: Query vector (list of floats)
                    k: Number of results to return
                
                Returns:
                    List of SearchResult objects sorted by descending similarity
                
                Raises:
                    ValueError: If query dimension doesn't match store dimension
            )pbdoc")

      .def_property_readonly("size", &sentinel::VectorStore::size,
                             "Number of vectors currently stored")

      .def_property_readonly("dimension", &sentinel::VectorStore::dimension,
                             "Dimension of vectors in this store")

      .def_static("has_avx2_support", &sentinel::VectorStore::has_avx2_support,
                  "Check if AVX2 SIMD instructions are available on this CPU");

  // Module-level utility
  m.def("has_avx2", &sentinel::VectorStore::has_avx2_support,
        "Check if AVX2 SIMD instructions are available on this CPU");
}
