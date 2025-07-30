#include "dice/endpoint/SparqlUpdateEndpoint.hpp"

#include <dice/endpoint/ParseSPARQLUpdateParam.hpp>

#include <rapidjson/stringbuffer.h>

#include <rapidjson/writer.h>
#include <spdlog/spdlog.h>

#include <dice/endpoint/TimeoutCheck.hpp>

namespace dice::endpoint {


	SPARQLUpdateEndpoint::SPARQLUpdateEndpoint(tf::Executor &executor,
											   triple_store::TripleStore &triplestore,
											   SparqlQueryCache &sparql_query_cache,
											   EndpointCfg const &endpoint_cfg)
		: Endpoint(executor, triplestore, sparql_query_cache, endpoint_cfg) {}

	inline rapidjson::StringBuffer mutation_result_json(size_t const mutation_count) noexcept {
		rapidjson::StringBuffer buf;
		rapidjson::Writer jw{buf};

		jw.StartObject();
		jw.Key("mutation_count");
		jw.Uint64(mutation_count);
		jw.EndObject();
		return buf;
	}
	void SPARQLUpdateEndpoint::handle_query(restinio::request_handle_t req, std::chrono::steady_clock::time_point timeout) {
		using namespace dice::sparql2tensor;
		using namespace restinio;

		check_timeout(timeout);
		// parse sparql update
		UPDATEDATAQueryData update_query;
		try {
			update_query = parse_sparql_update_param(req);
		} catch (update_error const &e) {
			static constexpr auto message = "Request error";
			req->create_response(status_bad_request()).set_body(std::string{message} + ": " + e.what()).done();
			spdlog::warn("HTTP response {}: {} (detail: {})", status_bad_request(), message, e.what());
			return;
		}
		check_timeout(timeout);

		spdlog::debug("Incoming {} DATA", update_query.is_delete ? "DELETE" : "INSERT");
		spdlog::debug("Number of triples: {}", update_query.entries.size());

		// noop if entries is empty
		if (update_query.entries.empty()) {
			static auto return_buffer = mutation_result_json(0);

			req->create_response(status_ok())
					.append_header(http_field::content_type, "application/json")
					.set_body(std::string{return_buffer.GetString(), return_buffer.GetSize()})
					.done();
			spdlog::info("HTTP response {}, mutation_count: 0", status_ok());
			return;
		}

		// execute update
		// note: will not throw on timeout
		{
			auto const writer_lock = triplestore_.acquire_writer_lock();

			spdlog::debug("hypertrie size: {}", triplestore_.size());

			size_t const size_before = triplestore_.size();


			if (update_query.is_delete) {
				triplestore_.remove(update_query.entries, writer_lock);
			} else {
				triplestore_.insert(update_query.entries, writer_lock);
			}

			spdlog::debug("hypertrie size after: {}", triplestore_.size());
			size_t const mutation_count = update_query.is_delete
												  ? size_before - triplestore_.size()
												  : triplestore_.size() - size_before;

			auto return_buffer = mutation_result_json(mutation_count);

			req->create_response(status_ok())
					.append_header(http_field::content_type, "application/json")
					.set_body(std::string{return_buffer.GetString(), return_buffer.GetSize()})
					.done();

			spdlog::info("HTTP response {}, mutation_count: {}", status_ok(), mutation_count);
		}
	}

}// namespace dice::endpoint