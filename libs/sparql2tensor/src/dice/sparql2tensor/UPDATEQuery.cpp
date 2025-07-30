#include "UPDATEQuery.hpp"

#include <algorithm>
#include <type_traits>

#include <SparqlLexer/SparqlLexer.h>
#include <SparqlParser/SparqlParser.h>

#include "dice/sparql2tensor/parser/visitors/PrologueVisitor.hpp"
#include "dice/sparql2tensor/parser/exception/SPARQLErrorListener.hpp"

namespace dice::sparql2tensor {

	static bool is_alpha(char const ch) noexcept {
		return std::isalpha(ch);
	}

	static bool is_ws(char const ch) noexcept {
		return std::isspace(ch);
	}

	/**
	 * @brief reads a single word (determined by matcher) from the start of s
	 * @param s input string; will be modified to not include the word after extraction
	 * @param matcher determines the charset the word is made of
	 * @return the extracted word
	 */
	template<typename CharMatcher> requires std::is_nothrow_invocable_r_v<bool, CharMatcher, char>
	static std::string_view read_word(std::string_view &s, CharMatcher &&matcher) noexcept {
		auto const first_word_begin = std::find_if_not(s.begin(), s.end(), is_ws);
		auto const first_word_end = std::find_if_not(first_word_begin, s.end(), std::forward<CharMatcher>(matcher));

		auto word = s.substr(std::distance(s.begin(), first_word_begin), std::distance(first_word_begin, first_word_end));
		s.remove_prefix(std::distance(s.begin(), first_word_end));

		return word;
	}

	/**
	 * @brief reads a single word (determined by matcher) from the end of s
	 * @param s input string; will be modified to not include the word after extraction
	 * @param matcher determines the charset the word is made of
	 * @return the extracted word
	 */
	template<typename CharMatcher> requires std::is_nothrow_invocable_r_v<bool, CharMatcher, char>
	static std::string_view read_word_rev(std::string_view &s, CharMatcher &&matcher) noexcept {
		auto const first_word_rbegin = std::find_if_not(s.rbegin(), s.rend(), is_ws);
		auto const first_word_rend = std::find_if_not(first_word_rbegin, s.rend(), std::forward<CharMatcher>(matcher));

		auto word = s.substr(std::distance(first_word_rend, s.rend()), std::distance(first_word_rbegin, first_word_rend));
		s.remove_suffix(std::distance(s.rbegin(), first_word_rend));

		return word;
	}

	/**
	 * @brief extracts the prologue from an update query
	 * @param s the whole query, will be modified to not include the extracted prologue afterwards
	 * @return the extracted prologue
	 */
	static std::string_view read_prologue(std::string_view &s) noexcept {
		auto const query_body_begin = s.find_first_of('{');
		if (query_body_begin == std::string_view::npos) {
			// body begin not found, error will be handled by calling function
			return "";
		}

		auto const prologue_last_char = s.substr(0, query_body_begin).find_last_of('>');
		if (prologue_last_char == std::string_view::npos) {
			// no prologue found
			return "";
		}

		auto const prologue = s.substr(0, prologue_last_char + 1);
		s.remove_prefix(prologue_last_char + 1);

		return prologue;
	}

	enum struct QueryType {
		INSERT_DATA,
		DELETE_DATA,
		UNKNOWN,
	};

	/**
	 * @brief reads the beginning of the actual query (after prologue) and tries to recognize the query type
	 * @param s the whole query without the prologue, will be modified to not include the query type
	 * @return the extracted query type
	 *
	 * @example
	 * @code
	 * 		std::string_view s = "DELETE DATA { ... }";
	 * 		QueryType const query_type = read_query_type(s);
	 *
	 * 		assert(query_type == QueryType::DELETE_DATA);
	 * 		assert(s == " { ... }");
	 * @endcode
	 */
	static QueryType read_query_type(std::string_view &s) noexcept {
		auto const first_word = read_word(s, is_alpha);
		auto const second_word = read_word(s, is_alpha);

		if (second_word != "DATA") {
			return QueryType::UNKNOWN;
		}

		if (first_word == "DELETE") {
			return QueryType::DELETE_DATA;
		} else if (first_word == "INSERT") {
			return QueryType::INSERT_DATA;
		}

		return QueryType::UNKNOWN;
	}

	UPDATEDATAQueryData UPDATEDATAQueryData::parse(std::string_view const sparql_update_str) {
		std::string_view rest_mut = sparql_update_str;
		auto const prologue = read_prologue(rest_mut);

		UPDATEDATAQueryData update_query;

		// expected structure for fast path: 'prologue... (DELETE|INSERT) DATA { triples... }'
		auto const query_type = read_query_type(rest_mut);
		auto const third_word = read_word(rest_mut, [](char const ch) noexcept { return ch == '{'; });

		if (query_type != QueryType::UNKNOWN) {
			// fast path for DELETE DATA / INSERT DATA

			if (third_word != "{") {
				// missing (or too many) '{' after '(DELETE|INSERT) DATA'
				std::ostringstream err;
				err << "syntax error: expected '{' after " << (query_type == QueryType::DELETE_DATA ? "DELETE DATA" : "INSERT DATA");
				throw std::runtime_error{err.str()};
			}

			auto const last_word = read_word_rev(rest_mut, [](char const ch) noexcept { return ch == '}'; });

			if (last_word != "}") {
				// closing brace is missing from query
				throw std::runtime_error{"syntax error: expected '}' at end of query"};
			}

			using namespace rdf_tensor::parser;

			{ // parse only prologue using antlr
				parser::exception::SPARQLErrorListener error_listener{};
				antlr4::ANTLRInputStream input{prologue};
				dice::sparql_parser::base::SparqlLexer lexer{&input};
				antlr4::CommonTokenStream tokens{&lexer};
				dice::sparql_parser::base::SparqlParser parser{&tokens};
				parser.removeErrorListeners();
				parser.addErrorListener(&error_listener);

				auto update_ctx = parser.updateCommand();

				{ // visit prologue and store prefixes
					parser::visitors::PrologueVisitor p_visitor{update_query.prefixes};
					for (auto prefix_ctx : update_ctx->prologue()) {
						p_visitor.visitPrologue(prefix_ctx);
					}
				}
			}

			std::vector<rdf_tensor::NonZeroEntry> entries;

			{ // try to parse all triples between '{' and '}' with rdf4cpp and then store them in 'entries'
				std::istringstream iss{std::string{rest_mut}};
				IStreamQuadIterator::state_type st{.iri_factory = std::move(update_query.prefixes)};
				for (IStreamQuadIterator qit{iss, ParsingFlag::NoParsePrefix, &st}; qit != std::default_sentinel; ++qit) {
					if (qit->has_value()) {
						auto const &quad = **qit;
						entries.push_back(rdf_tensor::NonZeroEntry{{quad.subject(), quad.predicate(), quad.object()}});
					} else {
						std::ostringstream oss;
						oss << qit->error();
						throw std::runtime_error{oss.str()};
					}
				}
			}

			update_query.is_delete = query_type == QueryType::DELETE_DATA;
			update_query.entries = std::move(entries);
		} else {
			throw std::runtime_error{"Currently only DELETE DATA and INSERT DATA updates are supported"};
		}

		return update_query;
	}

}// namespace dice::sparql2tensor