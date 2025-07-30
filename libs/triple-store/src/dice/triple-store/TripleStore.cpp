#include "TripleStore.hpp"

#include <rdf4cpp/rdf.hpp>

#include <fstream>

namespace dice::triple_store {
	TripleStore::TripleStore(TripleStore::BoolHypertrie &hypertrie) : hypertrie_(hypertrie) {}

	TripleStore::~TripleStore() {
		WriterLock writer_lock{mutex_};
	}

	void TripleStore::load_ttl(std::string const &file_path, uint32_t bulk_size,
							   rdf_tensor::HypertrieBulkInserter::BulkProcessed_callback const &call_back,
							   std::function<void(rdf_tensor::parser::ParsingError const &)> const &error_callback) {
		std::ifstream ifs{file_path};

		if (!ifs.is_open()) {
			throw std::runtime_error{"unable to open provided file " + file_path};
		}

		WriterLock writer_lock{mutex_};
		HypertrieBulkInserter bulk_inserter{hypertrie_, bulk_size, call_back};
		for (rdf4cpp::rdf::parser::IStreamQuadIterator qit{ifs}; qit != std::default_sentinel; ++qit) {
			if (qit->has_value()) {
				auto const &quad = qit->value();
				bulk_inserter.add(
						hypertrie::internal::raw::SingleEntry<3, htt_t>{{quad.subject(), quad.predicate(), quad.object()}});
			} else {
				error_callback(qit->error());
			}
		}
	}

	void TripleStore::remove(std::vector<rdf_tensor::NonZeroEntry> const &entries,
							 [[maybe_unused]] WriterLock const &writer_lock, uint32_t const bulk_size) {
		HypertrieBulkRemover bulk_remover{hypertrie_, bulk_size};

		for (auto const &e : entries) {
			bulk_remover.add(e);
		}
	}

	void TripleStore::insert(std::vector<rdf_tensor::NonZeroEntry> const &entries,
							 [[maybe_unused]] WriterLock const &writer_lock, uint32_t const bulk_size) {
		assert(writer_lock.owns_lock());
		HypertrieBulkInserter bulk_inserter{hypertrie_, bulk_size};

		for (auto const &e : entries) {
			bulk_inserter.add(e);
		}
	}

	bool TripleStore::is_rdf_list(rdf4cpp::rdf::Node list) const noexcept {
		ReaderLock reader_lock{mutex_};

		using IRI = rdf4cpp::rdf::IRI;
		IRI rdf_nil("http://www.w3.org/1999/02/22-rdf-syntax-ns#nil");

		if (list == rdf_nil) return true;// empty collection

		auto prop_obj = std::get<0>(hypertrie_[rdf_tensor::SliceKey{list, std::nullopt, std::nullopt}]);
		if (prop_obj.empty()) return false;

		{
			IRI rdf_first("http://www.w3.org/1999/02/22-rdf-syntax-ns#first");
			auto has_first = std::get<0>(prop_obj[rdf_tensor::SliceKey{rdf_first, std::nullopt}]);
			if (has_first.size() != 1) return false;
		}

		{
			IRI rdf_rest("http://www.w3.org/1999/02/22-rdf-syntax-ns#rest");
			auto has_rest = std::get<0>(prop_obj[rdf_tensor::SliceKey{rdf_rest, std::nullopt}]);
			if (has_rest.size() != 1) return false;
		}

		return true;
	}
	std::vector<rdf4cpp::rdf::Node> TripleStore::get_rdf_list(rdf4cpp::rdf::Node list) const {
		ReaderLock reader_lock{mutex_};
		using IRI = rdf4cpp::rdf::IRI;
		using Node = rdf4cpp::rdf::Node;

		IRI rdf_first("http://www.w3.org/1999/02/22-rdf-syntax-ns#first");
		IRI rdf_rest("http://www.w3.org/1999/02/22-rdf-syntax-ns#rest");
		IRI rdf_nil("http://www.w3.org/1999/02/22-rdf-syntax-ns#nil");

		std::vector<Node> node_vector;
		auto head = list;
		while (head != rdf_nil) {
			auto element = std::get<0>(hypertrie_[rdf_tensor::SliceKey{list, rdf_first, std::nullopt}]);
			if (element.size() > 1)
				throw std::runtime_error("Invalid RDF seq. Multiple first elements for list node {}" + std::string(head));
			if (element.empty())
				throw std::runtime_error("Invalid RDF seq. No first elements for list node {}" + std::string(head));

			node_vector.push_back((*element.begin())[0]);
			auto rest = std::get<0>(hypertrie_[rdf_tensor::SliceKey{list, rdf_rest, std::nullopt}]);
			if (rest.size() > 1) {
				throw std::runtime_error("Invalid RDF seq. Multiple rest elements for list node {}" + std::string(head));
			} else if (rest.size() == 1) {
				head = (*element.begin())[0];
			} else /* rest.size() == 0 */ {

				head = rdf_nil;// this is not canonical but seems better than throwing an error
			}
		}
		return node_vector;
	}

	/**
	 * @brief Generates the tensor operands of a query
	 * @param slice_keys The slice keys corresponding to the query being evaluated
	 * @return A vector of tensor operands (const_BoolHypertries).
	 */
	std::vector<rdf_tensor::const_BoolHypertrie> generate_operands(rdf_tensor::BoolHypertrie rdf_tensor, std::vector<rdf_tensor::SliceKey> const &slice_keys, [[maybe_unused]] TripleStore::ReaderLock const &reader_lock) {
		using const_BoolHypertrie = rdf_tensor::const_BoolHypertrie;
		using BoolHypertrie = rdf_tensor::BoolHypertrie;

		assert(reader_lock.owns_lock());

		std::vector<const_BoolHypertrie> operands;
		for (auto const &slice_key : slice_keys) {
			auto slice_result = rdf_tensor[slice_key];
			if (slice_key.get_fixed_depth() == 3) {
				auto entry_exists = std::get<bool>(slice_result);
				BoolHypertrie ht_0{0, rdf_tensor.context()};
				if (entry_exists)
					ht_0.set({}, true);
				operands.push_back(ht_0);
			} else {
				auto operand = std::get<const_BoolHypertrie>(slice_result);
				operands.push_back(std::move(operand));
			}
		}
		return operands;
	}

	std::generator<rdf_tensor::Entry const &> TripleStore::eval_select(const sparql2tensor::SPARQLQuery &query, std::chrono::steady_clock::time_point endtime) const {
		ReaderLock reader_lock{mutex_};
		auto operands = generate_operands(hypertrie_, query.get_slice_keys(), reader_lock);
		std::vector<char> proj_vars_id{};
		for (auto const &proj_var : query.projected_variables_) {
			proj_vars_id.push_back(query.var_to_id_.at(proj_var));
		}
		rdf_tensor::Query q{query.odg_, operands, proj_vars_id, endtime};
		if (query.distinct_) {
			rdf_tensor::Entry entry;
			entry.key().resize(query.projected_variables_.size());
			for (auto const &distinct_entry : dice::query::Evaluation::evaluate<htt_t, allocator_type, true>(q)) {
				std::copy(distinct_entry.key().begin(), distinct_entry.key().end(), entry.key().begin());
				co_yield entry;
			}
		} else {
			for (auto const &entry : dice::query::Evaluation::evaluate<htt_t, allocator_type>(q)) {
				co_yield entry;
			}
		}
	}
	bool TripleStore::eval_ask(const sparql2tensor::SPARQLQuery &query, std::chrono::steady_clock::time_point endtime) const {
		ReaderLock reader_lock{mutex_};
		auto operands = generate_operands(hypertrie_, query.get_slice_keys(), reader_lock);
		rdf_tensor::Query q{query.odg_, operands, {}, endtime};
		return dice::query::Evaluation::evaluate_ask<htt_t, allocator_type>(q);
	}
	size_t TripleStore::count(const sparql2tensor::SPARQLQuery &query, std::chrono::steady_clock::time_point endtime) const {
		ReaderLock reader_lock{mutex_};
		using namespace sparql2tensor;
		if (query.triple_patterns_.size() == 1) {// O(1)
			auto slice_key = query.get_slice_keys()[0];
			if (slice_key.get_fixed_depth() == 3)
				return static_cast<size_t>(std::get<bool>(get_hypertrie()[slice_key]));
			return std::get<const_BoolHypertrie>(get_hypertrie()[slice_key]).size();
		}
		size_t count = 0;
		for (auto const &entry : this->eval_select(query, endtime))
			count += entry.value();
		return count;
	}
	bool TripleStore::contains(const rdf4cpp::rdf::Statement &statement) const {
		ReaderLock reader_lock{mutex_};
		return hypertrie_[Key{statement.subject(), statement.predicate(), statement.object()}];
	}
	size_t TripleStore::size() const {
		return hypertrie_.size();
	}
}// namespace dice::triple_store
