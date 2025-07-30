#ifndef TENTRIS_STORE_TRIPLESTORE
#define TENTRIS_STORE_TRIPLESTORE

#include <dice/rdf-tensor/Query.hpp>
#include <dice/rdf-tensor/RDFTensor.hpp>

#include <dice/sparql2tensor/SPARQLQuery.hpp>

#ifndef BOOST_BIND_GLOBAL_PLACEHOLDERS
#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#endif
#include <metall/metall.hpp>

#include <shared_mutex>

namespace dice::triple_store {
	class TripleStore {
		using HypertrieContext = rdf_tensor::HypertrieContext;
		using HypertrieContext_ptr = rdf_tensor::HypertrieContext_ptr;
		using HypertrieSyncBulkInserter = rdf_tensor::HypertrieSyncBulkInserter;
		using HypertrieSyncBulkRemvoer = rdf_tensor::HypertrieSyncBulkRemover;
		using BoolHypertrie = rdf_tensor::BoolHypertrie;
		using const_BoolHypertrie = rdf_tensor::const_BoolHypertrie;
		using Key = rdf_tensor::Key;
		using htt_t = rdf_tensor::htt_t;

	public:
		using HypertrieBulkInserter = rdf_tensor::HypertrieBulkInserter;
		using HypertrieBulkRemover = rdf_tensor::HypertrieBulkRemover;
		using allocator_type = rdf_tensor::allocator_type;

	private:
		BoolHypertrie &hypertrie_;
		mutable std::shared_mutex mutex_;

	public:
		using ReaderLock = std::shared_lock<std::shared_mutex>;
		using WriterLock = std::unique_lock<std::shared_mutex>;

		explicit TripleStore(BoolHypertrie &hypertrie);

		~TripleStore();

		[[nodiscard]] BoolHypertrie const &get_hypertrie() const {
			return hypertrie_;
		}

		ReaderLock acquire_reader_lock() const {
			return ReaderLock{mutex_};
		}

		WriterLock acquire_writer_lock() const {
			return WriterLock{mutex_};
		}

		/**
		 * This function enforces stricter requirements upon rdf:Lists than described in <a href="https://www.w3.org/TR/2014/REC-rdf11-mt-20140225/#rdf-containers">D.3 RDF collections</a>.
		 * An rdf:List must either be the IRI rdf:nil or must have the properties rdf:first and rdf:rest, both with cardinality 1.
		 * @param list the node to be checked if it is a list
		 * @return if list is an rdf:List
		 */
		[[nodiscard]] bool is_rdf_list(rdf4cpp::rdf::Node list) const noexcept;

		/**
		 * Returns the items of an rdf:List as vector.
		 *
		 * Restrictions from is_rdf_list(rdf4cpp::rdf::Node) const noexcept apply.
		 *
		 * @param list the start node of the list
		 * @return the elements of the list as vector
		 * @throws std::runtime_error If the list is malformed.
		 */
		std::vector<rdf4cpp::rdf::Node> get_rdf_list(rdf4cpp::rdf::Node list) const;

		/**
		 * @brief Loads a turtle file into this triplestore
		 * @param file_path path to the file
		 * @param bulk_size number of entries to insert at once
		 * @param call_back function to call when a bulk is inserted
		 * @param error_callback function to call when an error is encountered in the file
		 */
		void load_ttl(
				std::string const &file_path,
				uint32_t bulk_size = 1'000'000,
				HypertrieBulkInserter::BulkProcessed_callback const &call_back = [](size_t, size_t, size_t) -> void {},
				std::function<void(rdf_tensor::parser::ParsingError const &)> const &error_callback = [](rdf_tensor::parser::ParsingError const &) -> void {});

		/**
		 * @brief Removes the given set of entries from the triplestore
		 * @note function blocks other readers and writers
		 * @param entries the set of entries to be removed from the triplestore.
		 * @param writer_lock a lock ensuring that write operations on the triple store are atomic and thread-safe.
		 * @param bulk_size defines how many entries should be removed at once. Default value is 1'000'000.
		 */
		void remove(std::vector<rdf_tensor::NonZeroEntry> const &entries, WriterLock const &writer_lock, uint32_t bulk_size = 1'000'000);

		/**
		 * @brief Inserts the given set of entries to the triplestore
		 * @note function blocks other readers and writers
		 * @param entries the set of entries to be inserted into the triplestore.
		 * @param writer_lock a lock ensuring that write operations on the triple store are atomic and thread-safe.
		 * @param bulk_size defines how many entries should be inserted at once. Default value is 1'000'000.
		 */
		void insert(std::vector<rdf_tensor::NonZeroEntry> const &entries, WriterLock const &writer_lock, uint32_t bulk_size = 1'000'000);

		/**
		 * @brief Evaluation of SPARQL SELECT queries.
		 * @param query The parsed SPARQL query.
		 * @param endtime The timeout value
		 * @return A generator yielding the solutions of the query
		 */
		std::generator<rdf_tensor::Entry const &>
		eval_select(const sparql2tensor::SPARQLQuery &query,
					std::chrono::steady_clock::time_point endtime = std::chrono::steady_clock::time_point::max()) const;

		/**
		 * @brief Evaluation of SPARQL ASK queries.
		 * @param query The parsed SPARQL query.
		 * @param endtime The timeout value
		 * @return The result of the ask query (true or false).
		 */
		bool eval_ask(const sparql2tensor::SPARQLQuery &query,
					  std::chrono::steady_clock::time_point endtime = std::chrono::steady_clock::time_point::max()) const;

		size_t count(const sparql2tensor::SPARQLQuery &query,
					 std::chrono::steady_clock::time_point endtime = std::chrono::steady_clock::time_point::max()) const;

		bool contains(const rdf4cpp::rdf::Statement &statement) const;

		[[nodiscard]] size_t size() const;
	};
};    // namespace dice::triple_store
#endif//TENTRIS_STORE_TRIPLESTORE
