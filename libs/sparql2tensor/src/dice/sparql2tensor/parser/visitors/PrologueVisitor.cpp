#include "dice/sparql2tensor/parser/visitors/PrologueVisitor.hpp"

namespace dice::sparql2tensor::parser::visitors {

	PrologueVisitor::PrologueVisitor(rdf4cpp::rdf::IRIFactory &prefixes) noexcept : prefixes_{&prefixes} {
	}

	std::any PrologueVisitor::visitPrologue(SparqlParser::PrologueContext *ctx) {
		for (auto pref_ctx : ctx->prefixDecl())
			visitPrefixDecl(pref_ctx);
		for ([[maybe_unused]] auto base_ctx : ctx->baseDecl())
			throw std::runtime_error("Base Declarations not supported yet.");
		return nullptr;
	}

	std::any PrologueVisitor::visitBaseDecl([[maybe_unused]] SparqlParser::BaseDeclContext *ctx) {
		return nullptr;
	}

	std::any PrologueVisitor::visitPrefixDecl(SparqlParser::PrefixDeclContext *ctx) {
		std::string prefix{};
		if (ctx->PNAME_NS())
			prefix = ctx->PNAME_NS()->getText();
		auto ns = ctx->IRIREF()->getText();
		prefixes_->assign_prefix(prefix.substr(0, prefix.size() - 1), ns.substr(1, ns.size() - 2));
		return nullptr;
	}


}// namespace dice::sparql2tensor::parser::visitors