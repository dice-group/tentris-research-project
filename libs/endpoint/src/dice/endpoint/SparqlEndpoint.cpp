#include "dice/endpoint/SparqlEndpoint.hpp"

#include <dice/endpoint/TimeoutCheck.hpp>

#include <spdlog/spdlog.h>

#include <dice/endpoint/ParseSPARQLQueryParam.hpp>
#include <dice/endpoint/SparqlJsonResultSAXWriter.hpp>

namespace dice::endpoint {

	SPARQLEndpoint::SPARQLEndpoint(tf::Executor &executor,
								   triple_store::TripleStore &triplestore,
								   SparqlQueryCache &sparql_query_cache,
								   EndpointCfg const &endpoint_cfg)
		: Endpoint(executor, triplestore, sparql_query_cache, endpoint_cfg) {}

	void SPARQLEndpoint::handle_query(restinio::request_handle_t req, std::chrono::steady_clock::time_point timeout) {
		using namespace dice::sparql2tensor;
		using namespace restinio;

		auto sparql_query = parse_sparql_query_param(req, this->sparql_query_cache_);
		if (not sparql_query)
			return;

		if (sparql_query->ask_) {
			bool ask_res = this->triplestore_.eval_ask(*sparql_query, timeout);
			std::string res = ask_res ? "true" : "false";
			req->create_response(status_ok())
					.append_header(http_field::content_type, "application/sparql-results+json")
					.set_body(R"({ "head" : {}, "boolean" : )" + res + " }")
					.done();
		} else {
			SparqlJsonResultSAXWriter json_writer{sparql_query->projected_variables_, 100'000};

			for (auto const &entry : this->triplestore_.eval_select(*sparql_query, timeout)) {
				json_writer.add(entry);
			}
			json_writer.close();
			check_timeout(timeout);

			req->create_response(status_ok())
					.append_header(http_field::content_type, "application/sparql-results+json")
					.set_body(std::string{json_writer.string_view()})
					.done();
			spdlog::info("HTTP response {}: {} variables, {} solutions, {} bindings",
						 status_ok(),
						 sparql_query->projected_variables_.size(),
						 json_writer.number_of_written_solutions(),
						 json_writer.number_of_written_bindings());
		}
	}

}// namespace dice::endpoint