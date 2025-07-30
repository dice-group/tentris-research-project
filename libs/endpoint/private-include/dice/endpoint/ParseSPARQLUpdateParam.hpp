#ifndef TENTRIS_PARSESPARQLUPDATEPARAM_HPP
#define TENTRIS_PARSESPARQLUPDATEPARAM_HPP

#include <spdlog/spdlog.h>

#include <restinio/helpers/http_field_parsers/content-type.hpp>
#include <restinio/request_handler.hpp>
#include <restinio/uri_helpers.hpp>

#include <dice/sparql2tensor/UPDATEQuery.hpp>


namespace dice::endpoint {

	struct update_error : std::runtime_error {
		explicit update_error(const std::string &message)
			: std::runtime_error(message) {}
	};

	inline sparql2tensor::UPDATEDATAQueryData parse_sparql_update_param(restinio::request_handle_t &req) {
		using namespace dice::sparql2tensor;
		using namespace restinio;
		auto content_type = req->header().opt_value_of(http_field::content_type);
		auto content_type_value = http_field_parsers::content_type_value_t::try_parse(*content_type);
		if (not content_type_value.has_value() or
			content_type_value.value().media_type.type != "application" or
			content_type_value.value().media_type.subtype != "sparql-update") {
			throw update_error("Expected content-type: application/sparql-update");
		}
		std::string sparql_update_str{req->body()};
		try {
			auto update_query = UPDATEDATAQueryData::parse(sparql_update_str);
			return update_query;
		} catch (std::exception &ex) {
			static constexpr auto message = "Value of parameter 'update' is not parsable: ";
			throw update_error{std::string{message} + ex.what()};
		} catch (...) {
			static constexpr auto message = "Unknown error";
			throw update_error{message};
		}
	}

}// namespace dice::endpoint


#endif//TENTRIS_PARSESPARQLUPDATEPARAM_HPP
