#ifndef TENTRIS_SPARQLUPDATEENDPOINT_HPP
#define TENTRIS_SPARQLUPDATEENDPOINT_HPP

#include <dice/endpoint/Endpoint.hpp>
#include <rapidjson/stringbuffer.h>

namespace dice::endpoint {

	class SPARQLUpdateEndpoint final : public Endpoint {
	public:
		SPARQLUpdateEndpoint(tf::Executor &executor, triple_store::TripleStore &triplestore, SparqlQueryCache &sparql_query_cache, EndpointCfg const &endpoint_cfg);

	protected:
		void handle_query(restinio::request_handle_t req, std::chrono::steady_clock::time_point timeout) override;
	};

}// namespace dice::endpoint

#endif//TENTRIS_SPARQLUPDATEENDPOINT_HPP
