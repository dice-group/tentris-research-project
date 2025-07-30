#include "SPARQLQuery.hpp"

#include <SparqlLexer/SparqlLexer.h>
#include <SparqlParser/SparqlParser.h>

#include "dice/sparql2tensor/parser/visitors/PrologueVisitor.hpp"

#include "dice/sparql2tensor/parser/visitors/SelectAskQueryVisitor.hpp"


namespace dice::sparql2tensor {

	SPARQLQuery dice::sparql2tensor::SPARQLQuery::parse(std::string const &sparql_query_str) {
		antlr4::ANTLRInputStream input(sparql_query_str);
		dice::sparql_parser::base::SparqlLexer lexer(&input);
		antlr4::CommonTokenStream tokens(&lexer);
		dice::sparql_parser::base::SparqlParser parser(&tokens);

		auto q_ctx = parser.query();

		if (not q_ctx->selectQuery() and not q_ctx->askQuery())
			throw std::runtime_error("Only SELECT & ASK queries are supported currently.");

		SPARQLQuery p_sparql{};
		if (q_ctx->prologue()) {
			parser::visitors::PrologueVisitor p_visitor{p_sparql.prefixes_};
			p_visitor.visitPrologue(q_ctx->prologue());
		}

		parser::visitors::SelectAskQueryVisitor visitor{&p_sparql};
		if (q_ctx->selectQuery())
			visitor.visitSelectQuery(q_ctx->selectQuery());
		else if(q_ctx->askQuery())
			visitor.visitAskQuery(q_ctx->askQuery());

		return p_sparql;
	}

	bool SPARQLQuery::is_distinct() const noexcept {
		return distinct_;
	}

	std::vector<rdf_tensor::SliceKey> SPARQLQuery::get_slice_keys() const {
		std::vector<rdf_tensor::SliceKey> slice_keys;
		slice_keys.reserve(triple_patterns_.size());
		for (auto const &tp : triple_patterns_) {
			rdf_tensor::SliceKey slice_key;
			slice_key.reserve(3);
			for (auto const &node : tp) {
				if (node.is_variable())
					slice_key.push_back(std::nullopt);
				else
					slice_key.push_back(node);
			}
			slice_keys.push_back(std::move(slice_key));
		}
		return slice_keys;
	}
}// namespace dice::sparql2tensor