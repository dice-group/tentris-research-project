#include "dice/endpoint/SparqlStreamingEndpoint.hpp"

#include <spdlog/spdlog.h>

#include <dice/endpoint/ParseSPARQLQueryParam.hpp>
#include <dice/endpoint/SparqlJsonResultSAXWriter.hpp>

namespace dice::endpoint {


	SPARQLStreamingEndpoint::SPARQLStreamingEndpoint(tf::Executor &executor,
													 triple_store::TripleStore &triplestore,
													 SparqlQueryCache &sparql_query_cache,
													 EndpointCfg const &endpoint_cfg)
		: Endpoint(executor, triplestore, sparql_query_cache, endpoint_cfg) {}

	void SPARQLStreamingEndpoint::handle_query(restinio::request_handle_t req, std::chrono::steady_clock::time_point timeout) {
		using namespace dice::sparql2tensor;
		using namespace restinio;

		std::shared_ptr<SPARQLQuery const> sparql_query = parse_sparql_query_param(req, this->sparql_query_cache_);
		if (not sparql_query)
			return;

		bool asio_write_failed = false;

		SparqlJsonResultSAXWriter json_writer{sparql_query->projected_variables_, 100'000};

		response_builder_t<chunked_output_t> resp = req->template create_response<chunked_output_t>();
		resp.append_header(http_field::content_type, "application/sparql-results+json");

		for (auto const &entry : this->triplestore_.eval_select(*sparql_query, timeout)) {
			json_writer.add(entry);
			if (json_writer.full()) {
				resp.append_chunk(std::string{json_writer.string_view()});
				resp.flush([&](auto const &status) { asio_write_failed = status.failed(); });
				if (asio_write_failed) {
					spdlog::warn("Writing chunked HTTP response failed.");
					return;
				}
				json_writer.clear();
			}
		}
		json_writer.close();
		resp.append_chunk(std::string{json_writer.string_view()});
		resp.done();
		spdlog::info("HTTP response {}: {} variables, {} solutions, {} bindings",
					 status_ok(),
					 sparql_query->projected_variables_.size(),
					 json_writer.number_of_written_solutions(),
					 json_writer.number_of_written_bindings());
	}
}// namespace dice::endpoint