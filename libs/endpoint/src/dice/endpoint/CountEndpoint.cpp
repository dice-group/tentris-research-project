#include "dice/endpoint/CountEndpoint.hpp"
#include <spdlog/spdlog.h>

#include <dice/endpoint/ParseSPARQLQueryParam.hpp>

namespace dice::endpoint {

	CountEndpoint::CountEndpoint(tf::Executor &executor,
								 triple_store::TripleStore &triplestore,
								 SparqlQueryCache &sparql_query_cache,
								 EndpointCfg const &endpoint_cfg)
		: Endpoint(executor, triplestore, sparql_query_cache, endpoint_cfg) {}

	void CountEndpoint::handle_query(restinio::request_handle_t req, std::chrono::steady_clock::time_point timeout) {
		using namespace dice::sparql2tensor;
		using namespace restinio;

		auto sparql_query = parse_sparql_query_param(req, this->sparql_query_cache_);
		if (not sparql_query)
			return;

		auto const count = this->triplestore_.count(*sparql_query, timeout);

		req->create_response(status_ok())
				.set_body(fmt::format("{}", count))
				.done();
		spdlog::info("HTTP response {}: counted {} results", status_ok(), count);
	}
}// namespace dice::endpoint