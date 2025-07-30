#ifndef TENTRIS_SPARQLSTREAMINGENDPOINT_HPP
#define TENTRIS_SPARQLSTREAMINGENDPOINT_HPP

#include <dice/endpoint/Endpoint.hpp>

namespace dice::endpoint {

	class SPARQLStreamingEndpoint final : public Endpoint {

	public:
		SPARQLStreamingEndpoint(tf::Executor &executor, triple_store::TripleStore &triplestore, SparqlQueryCache &sparql_query_cache, EndpointCfg const &endpoint_cfg);

	protected:
		void handle_query(restinio::request_handle_t req, std::chrono::steady_clock::time_point timeout) override;
	};
}// namespace dice::endpoint

#endif//TENTRIS_SPARQLSTREAMINGENDPOINT_HPP
