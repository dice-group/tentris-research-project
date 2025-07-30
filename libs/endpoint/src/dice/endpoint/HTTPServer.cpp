#include "HTTPServer.hpp"

#include <dice/endpoint/CountEndpoint.hpp>
#include <dice/endpoint/SparqlEndpoint.hpp>
#include <dice/endpoint/SparqlStreamingEndpoint.hpp>
#include <dice/endpoint/SparqlUpdateEndpoint.hpp>

#include <csignal>
#include <cstring>
#include <unistd.h>

#include <spdlog/spdlog.h>

volatile sig_atomic_t signalReceived = 0;// Flag to indicate a signal was received

namespace dice::endpoint {

	class restinio_spd_logger_t {
	public:
		template<typename Msg_Builder>
		void trace(Msg_Builder &&mb) {
			spdlog::trace(mb());
		}

		template<typename Msg_Builder>
		void info(Msg_Builder &&mb) {
			spdlog::info(mb());
		}

		template<typename Msg_Builder>
		void warn(Msg_Builder &&mb) {
			spdlog::warn(mb());
		}


		template<typename Msg_Builder>
		void error(Msg_Builder &&mb) {
			spdlog::error(mb());
		}
	};

	struct tentris_restinio_traits : public restinio::traits_t<
											 restinio::asio_timer_manager_t,
											 restinio_spd_logger_t,
											 restinio::router::express_router_t<>> {
		static constexpr bool use_connection_count_limiter = true;
	};

	HTTPServer::HTTPServer(tf::Executor &executor, triple_store::TripleStore &triplestore, EndpointCfg const &cfg)
		: executor_(executor),
		  triplestore_(triplestore),
		  sparql_query_cache_(),// TODO: override default parameter
		  router_(std::make_unique<restinio::router::express_router_t<>>()),
		  cfg_(cfg) {}

	void HTTPServer::operator()() {
		spdlog::info("Available endpoints:");
		router_->http_get(R"(/sparql)",
						  SPARQLEndpoint{executor_, triplestore_, sparql_query_cache_, cfg_});
		spdlog::info("  GET /sparql?query= for normal queries");

		router_->http_post(R"(/sparql)",
						   SPARQLUpdateEndpoint{executor_, triplestore_, sparql_query_cache_, cfg_});
		spdlog::info("  POST  /sparql for update queries");

		router_->http_get(R"(/stream)",
						  SPARQLStreamingEndpoint{executor_, triplestore_, sparql_query_cache_, cfg_});
		spdlog::info("  GET  /stream?query= for queries with huge results");

		router_->http_get(R"(/count)",
						  CountEndpoint{executor_, triplestore_, sparql_query_cache_, cfg_});
		spdlog::info("  GET  /count?query= as a workaround for count");


		router_->non_matched_request_handler(
				[](auto req) -> restinio::request_handling_status_t {
					return req->create_response(restinio::status_not_found()).connection_close().done();
				});

		spdlog::info("Use Ctrl+C on the terminal or SIGINT to shut down tentris gracefully. If tentris is killed or crashes, the index files will be corrupted.");
		using namespace std::chrono;
		auto const time_limit = (cfg_.opt_timeout_duration)
										? duration_cast<steady_clock::duration>(cfg_.opt_timeout_duration.value() * 0.95)
										: steady_clock::duration::max();
		auto const pool_size = std::max(4, cfg_.threads / 4);
		auto server = restinio::run_async(restinio::own_io_context(),

										  restinio::server_settings_t<tentris_restinio_traits>{}
												  .max_parallel_connections(cfg_.threads)
												  .handle_request_timeout(time_limit)
												  .read_next_http_message_timelimit(seconds{1})
												  .write_http_response_timelimit(time_limit)
												  .max_pipelined_requests(1)
												  .address("0.0.0.0")
												  .port(cfg_.port)
												  .request_handler(std::move(router_))
												  .cleanup_func([this]() { this->executor_.wait_for_all(); }),
										  pool_size);


		auto signal_handler = [](int signum) {
			spdlog::info("Interrupt signal ({}) received.\n", signum);
			signalReceived = signum;// Set the signal received flag
									// Cleanup and close the program
									// wake
		};

		// Define the signal handler structure
		struct sigaction sa;
		memset(&sa, 0, sizeof(sa));
		sa.sa_handler = signal_handler;
		sigemptyset(&sa.sa_mask);


		// Register the signal handler for SIGINT (CTRL+C) and SIGHUP (terminal closure)
		sigaction(SIGINT, &sa, nullptr);
		sigaction(SIGHUP, &sa, nullptr);

		while (!signalReceived) {
			pause();
		}
		server->stop();
		server->wait();
	}
}// namespace dice::endpoint