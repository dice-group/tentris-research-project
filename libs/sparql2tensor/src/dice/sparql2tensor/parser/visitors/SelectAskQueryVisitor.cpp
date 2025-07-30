#include "dice/sparql2tensor/parser/visitors/SelectAskQueryVisitor.hpp"

#include <boost/container/flat_set.hpp>

#include <algorithm>
#include <ranges>

namespace dice::sparql2tensor::parser::visitors {

	std::any SelectAskQueryVisitor::visitAskQuery(SparqlParser::AskQueryContext *ctx) {
		if (auto where_clause_ctx = ctx->whereClause(); where_clause_ctx)
			visitWhereClause(where_clause_ctx);
		else
			throw std::runtime_error("Query does not contain a WHERE clause");
		return nullptr;
	}

	std::any SelectAskQueryVisitor::visitSelectQuery(SparqlParser::SelectQueryContext *ctx) {
		if (auto where_clause_ctx = ctx->whereClause(); where_clause_ctx)
			visitWhereClause(where_clause_ctx);
		else
			throw std::runtime_error("Query does not contain a WHERE clause");
		visitSelectClause(ctx->selectClause());
		return nullptr;
	}

	std::any SelectAskQueryVisitor::visitSelectClause(SparqlParser::SelectClauseContext *ctx) {
		if (auto select_modifier_ctx = ctx->selectModifier(); select_modifier_ctx) {
			if (select_modifier_ctx->DISTINCT())
				query->distinct_ = true;
		}
		if (ctx->ASTERISK()) {
			query->project_all_variables_ = true;
			std::unordered_set<rdf4cpp::rdf::query::Variable> seen_vars;
			// set all non-anonymous variables from the triple patterns
			for (auto const &tp : query->triple_patterns_) {
				for (auto const &node : tp) {
					if (node.is_variable()) {
						auto var = node.as_variable();
						if (not var.is_anonymous()) {
							auto [_, was_new] = seen_vars.insert(var);
							if (was_new)
								query->projected_variables_.push_back(var);
						}
					}
				}
			}
		} else {
			for (auto sel_ctx : ctx->selectVariables()) {
				if (auto var_ctx = sel_ctx->var(); var_ctx) {
					auto var = std::any_cast<rdf4cpp::rdf::query::Variable>(visitVar(var_ctx));
					register_var(var);
					query->projected_variables_.push_back(var);
				} else {
					throw std::runtime_error("Expressions in SELECT clause are not supported yet.");
				}
			}
		}
		if (query->projected_variables_.empty())
			throw std::runtime_error("At least one variable should be projected.");
		return nullptr;
	}

	std::any SelectAskQueryVisitor::visitWhereClause(SparqlParser::WhereClauseContext *ctx) {
		// push a new entry into the stacks, as we are about to visit a graph pattern
		group_patterns.emplace_back();
		triples_blocks.emplace_back();
		optional_blocks.emplace_back();
		visitGroupGraphPattern(ctx->groupGraphPattern());
		// pop the top entry of the stacks, as we have finished visiting the graph pattern
		optional_blocks.pop_back();
		triples_blocks.pop_back();
		group_patterns.pop_back();
		return nullptr;
	}

	std::any SelectAskQueryVisitor::visitGroupGraphPattern(SparqlParser::GroupGraphPatternContext *ctx) {
		if (ctx->subSelect())
			throw std::runtime_error("Subqueries are not supported yet");
		else if (auto group_graph_pattern_sub_ctx = ctx->groupGraphPatternSub(); group_graph_pattern_sub_ctx)
			visitGroupGraphPatternSub(group_graph_pattern_sub_ctx);
		else
			throw std::runtime_error("Malformed query");
		return nullptr;
	}

	std::any SelectAskQueryVisitor::visitGroupGraphPatternSub(SparqlParser::GroupGraphPatternSubContext *ctx) {
		visitWellDesignedPattern(ctx, {});
		return nullptr;
	}

	void SelectAskQueryVisitor::visitWellDesignedPattern(SparqlParser::GroupGraphPatternSubContext *ctx,
														 std::vector<SparqlParser::GroupOrUnionGraphPatternContext *> gou_ctxs) {
		// store the context of the first triples block, if it is provided
		if (auto triples_block = ctx->triplesBlock(); triples_block)
			triples_blocks.back().push_back(triples_block);
		// iterate over all GroupGraphPatternSubs
		for (auto sub_ctx : ctx->groupGraphPatternSubList()) {
			if (auto graph_pattern_not_triples_ctx = sub_ctx->graphPatternNotTriples(); graph_pattern_not_triples_ctx) {
				// store all GroupOrUnionGraphPatterns that appear in the pattern
				if (auto group_or_union_graph_pattern_ctx = graph_pattern_not_triples_ctx->groupOrUnionGraphPattern(); group_or_union_graph_pattern_ctx)
					gou_ctxs.push_back(group_or_union_graph_pattern_ctx);
				// store all OptionalGraphPatterns that appear in the pattern
				else if (auto optional_graph_pattern_ctx = sub_ctx->graphPatternNotTriples()->optionalGraphPattern(); optional_graph_pattern_ctx)
					optional_blocks.back().push_back(optional_graph_pattern_ctx);
			}
			// store all triples blocks that appear in the pattern
			if (auto triples_block_ctx = sub_ctx->triplesBlock(); triples_block_ctx)
				triples_blocks.back().push_back(triples_block_ctx);
		}
		// the current pattern does not contain any GroupOrUnionGraphPatterns
		if (gou_ctxs.empty()) {
			// visit all triples blocks first
			for (auto tb_ctx : triples_blocks.back()) {
				visitTriplesBlock(tb_ctx);
			}
			// if we are in an optional pattern we need to capture dependencies
			if (not opt_operands.empty()) {
				// dependencies with parent group
				group_dependencies(group_patterns[group_patterns.size() - 2], group_patterns.back());
				// cartesian connections between optional patterns
				for (auto cur_op : group_patterns.back()) {
					for (auto opt_op : opt_operands.back()) {
						// do not connect groups of the same union pattern
						if (std::ranges::find(union_operands.back(), opt_op) == union_operands.back().end()) {
							query->odg_.add_connection(cur_op, opt_op);
							query->odg_.add_connection(opt_op, cur_op);
						}
					}
				}
				for (auto cur_op : group_patterns.back()) {
					union_operands.back().push_back(cur_op);
					opt_operands.back().push_back(cur_op);
				}
			}
			opt_operands.emplace_back();
			union_operands.emplace_back();
			// visit all optional patterns
			for (auto opt_ctx : optional_blocks.back()) {
				// push a new vector into the stacks, as we are going to visit a new graph pattern
				group_patterns.emplace_back();
				triples_blocks.emplace_back();
				optional_blocks.emplace_back();
				visitWellDesignedPattern(opt_ctx->groupGraphPattern()->groupGraphPatternSub(), {});
				// clear the vector from the operands of the visited graph pattern
				// the top vector of the stack is shared across all optional subgraph pattern of the current graph pattern
				union_operands.back().clear();
				// pop the top vector from the stack, as we have finished processing the graph pattern
				optional_blocks.pop_back();
				triples_blocks.pop_back();
				group_patterns.pop_back();
			}
			union_operands.pop_back();
			opt_operands.pop_back();
			// prepare for the next union
			group_patterns.back().clear();
		}
		// the pattern contains at least one GroupOrUnionGraphPattern
		// in case of multiple GroupOrUnionGraphPatterns, join operations are distributed over unions
		else {
			SparqlParser::GroupOrUnionGraphPatternContext *cur_gou_ctx = gou_ctxs.back();
			gou_ctxs.pop_back();
			size_t current_tbs = triples_blocks.back().size();
			size_t current_opts = optional_blocks.back().size();
			// visit each group graph pattern of the GroupOrUnionGraphPattern
			// while visiting each group graph pattern, the triples and optional blocks stored until this point will also be visited
			for (auto grp_ctx : cur_gou_ctx->groupGraphPattern()) {
				visitWellDesignedPattern(grp_ctx->groupGraphPatternSub(), gou_ctxs);
				// we resize the vectors in order to keep only the blocks that were present before visiting grp_ctx
				triples_blocks.back().resize(current_tbs);
				optional_blocks.back().resize(current_opts);
			}
		}
	}

	std::any SelectAskQueryVisitor::visitTriplesBlock(SparqlParser::TriplesBlockContext *ctx) {
		for (auto sub_ctx : ctx->triplesSameSubjectPath())
			visitTriplesSameSubjectPath(sub_ctx);
		return nullptr;
	}

	std::any SelectAskQueryVisitor::visitTriplesSameSubjectPath(SparqlParser::TriplesSameSubjectPathContext *ctx) {
		if (ctx->varOrTerm() and ctx->propertyListPathNotEmpty()) {
			active_subject = std::any_cast<rdf4cpp::rdf::Node>(visitVarOrTerm(ctx->varOrTerm()));
			if (active_subject.is_variable())
				register_var(active_subject.as_variable());
			visitPropertyListPathNotEmpty(ctx->propertyListPathNotEmpty());
		} else if (ctx->triplesNodePath() and ctx->propertyListPath()) {
			return nullptr;
		}
		return nullptr;
	}

	std::any SelectAskQueryVisitor::visitPropertyListPathNotEmpty(SparqlParser::PropertyListPathNotEmptyContext *ctx) {
		if (ctx->verbPath()) {
			active_predicate = std::any_cast<rdf4cpp::rdf::Node>(visitPath(ctx->verbPath()->path()));
		} else {
			auto var = std::any_cast<rdf4cpp::rdf::query::Variable>(visitVar(ctx->verbSimple()->var()));
			register_var(var);
			active_predicate = rdf4cpp::rdf::Node(var);
		}
		auto object_list_path_ctx = ctx->objectListPath();
		if (not object_list_path_ctx)
			throw std::runtime_error("Triple requires at least one object");
		visitObjectListPath(object_list_path_ctx);
		for (auto prop_ctx : ctx->propertyListPathNotEmptyList()) {
			if (auto verb_path_ctx = prop_ctx->verbPath(); verb_path_ctx) {
				active_predicate = std::any_cast<rdf4cpp::rdf::Node>(visitPath(verb_path_ctx->path()));
			} else {
				auto var = std::any_cast<rdf4cpp::rdf::query::Variable>(visitVar(prop_ctx->verbSimple()->var()));
				register_var(var);
				active_predicate = rdf4cpp::rdf::Node(var);
			}
			auto object_list_ctx = prop_ctx->objectList();
			if (not object_list_ctx)
				throw std::runtime_error("Triple requires at least one object");
			visitObjectList(object_list_ctx);
		}
		return nullptr;
	}

	std::any SelectAskQueryVisitor::visitVarOrTerm(SparqlParser::VarOrTermContext *ctx) {
		return rdf4cpp::rdf::Node([&]() -> rdf4cpp::rdf::Node {
			if (ctx->var()) {
				return std::any_cast<rdf4cpp::rdf::query::Variable>(visitVar(ctx->var()));
			} else {
				if (auto iri_ctx = ctx->graphTerm()->iri())
					return std::any_cast<rdf4cpp::rdf::IRI>(visitIri(iri_ctx));
				else if (auto blank_node_ctx = ctx->graphTerm()->blankNode(); blank_node_ctx)
					return std::any_cast<rdf4cpp::rdf::query::Variable>(visitBlankNode(blank_node_ctx));
				else if (auto rdf_literal_ctx = ctx->graphTerm()->rdfLiteral(); rdf_literal_ctx)
					return std::any_cast<rdf4cpp::rdf::Literal>(visitRdfLiteral(rdf_literal_ctx));
				else if (auto boolean_literal_ctx = ctx->graphTerm()->booleanLiteral(); boolean_literal_ctx)
					return std::any_cast<rdf4cpp::rdf::Literal>(visitBooleanLiteral(boolean_literal_ctx));
				else if (auto numberic_literal_ctx = ctx->graphTerm()->numericLiteral(); numberic_literal_ctx)
					return std::any_cast<rdf4cpp::rdf::Literal>(visitNumericLiteral(numberic_literal_ctx));
				else
					throw std::runtime_error("RDF collections are not supported yet.");
			}
		}());
	}

	std::any SelectAskQueryVisitor::visitIri(SparqlParser::IriContext *ctx) {
		if (ctx->IRIREF()) {
			auto iri = ctx->IRIREF()->getText();
			return rdf4cpp::rdf::IRI(iri.substr(1, iri.size() - 2));
		}
		std::string predicate = ctx->prefixedName()->PNAME_LN()->getText();
		std::size_t split = predicate.find(':');

		auto maybe_iri = query->prefixes_.from_prefix(predicate.substr(0, split), predicate.substr(split + 1));
		if (maybe_iri.has_value()) {
			return *maybe_iri;
		} else {
			throw std::runtime_error("Invalid prefixed IRI");
		}
	}

	std::any SelectAskQueryVisitor::visitBlankNode(SparqlParser::BlankNodeContext *ctx) {
		if (auto blank_node_label_ctx = ctx->BLANK_NODE_LABEL(); blank_node_label_ctx)
			return rdf4cpp::rdf::query::Variable(blank_node_label_ctx->getText().substr(2), true);
		else
			throw std::runtime_error("BlankNode ANON not supported.");
	}

	std::any SelectAskQueryVisitor::visitVar(SparqlParser::VarContext *ctx) {
		return rdf4cpp::rdf::query::Variable(ctx->getText().substr(1));
	}

	std::any SelectAskQueryVisitor::visitObjectListPath(SparqlParser::ObjectListPathContext *ctx) {
		for (auto objp_ctx : ctx->objectPath())
			visitObjectPath(objp_ctx);
		return nullptr;
	}

	std::any SelectAskQueryVisitor::visitObjectList(SparqlParser::ObjectListContext *ctx) {
		for (auto obj_ctx : ctx->object())
			visitObject(obj_ctx);
		return nullptr;
	}

	std::any SelectAskQueryVisitor::visitObjectPath(SparqlParser::ObjectPathContext *ctx) {
		if (auto var_or_term_ctx = ctx->graphNodePath()->varOrTerm(); var_or_term_ctx) {
			auto obj = std::any_cast<rdf4cpp::rdf::Node>(visitVarOrTerm(var_or_term_ctx));
			if (obj.is_variable())
				register_var(obj.as_variable());
			query->triple_patterns_.emplace_back(active_subject, active_predicate, obj);
			add_tp(query->triple_patterns_.back());
		} else {
			throw std::runtime_error("not supported");
		}
		return nullptr;
	}

	std::any SelectAskQueryVisitor::visitObject(SparqlParser::ObjectContext *ctx) {
		if (auto var_or_term_ctx = ctx->graphNode()->varOrTerm(); var_or_term_ctx) {
			auto obj = std::any_cast<rdf4cpp::rdf::Node>(visitVarOrTerm(var_or_term_ctx));
			if (obj.is_variable())
				register_var(obj.as_variable());
			query->triple_patterns_.emplace_back(active_subject, active_predicate, obj);
			add_tp(query->triple_patterns_.back());
		} else {
			throw std::runtime_error("not supported");
		}
		return nullptr;
	}

	std::any SelectAskQueryVisitor::visitPath(SparqlParser::PathContext *ctx) {
		if (auto path_alternative_ctx = ctx->pathAlternative(); path_alternative_ctx)
			return visitPathAlternative(path_alternative_ctx);
		else
			throw std::runtime_error("Malformed query.");
	}

	std::any SelectAskQueryVisitor::visitPathAlternative(SparqlParser::PathAlternativeContext *ctx) {
		if (ctx->pathSequence().size() > 1)
			throw std::runtime_error("Property paths are not supported yet");
		return visitPathSequence(ctx->pathSequence(0));
	}

	std::any SelectAskQueryVisitor::visitPathSequence(SparqlParser::PathSequenceContext *ctx) {
		if (ctx->pathEltOrInverse().size() > 1)
			throw std::runtime_error("Property paths are not supported yet");
		return visitPathEltOrInverse(ctx->pathEltOrInverse(0));
	}

	std::any SelectAskQueryVisitor::visitPathEltOrInverse(SparqlParser::PathEltOrInverseContext *ctx) {
		if (ctx->INVERSE())
			throw std::runtime_error("Property paths are not supported yet");
		return visitPathElt(ctx->pathElt());
	}

	std::any SelectAskQueryVisitor::visitPathElt(SparqlParser::PathEltContext *ctx) {
		auto path_primary_ctx = ctx->pathPrimary();
		if (auto iri_ctx = path_primary_ctx->iri(); iri_ctx)
			return rdf4cpp::rdf::Node(std::any_cast<rdf4cpp::rdf::IRI>(visitIri(iri_ctx)));
		else if (path_primary_ctx->A())
			return rdf4cpp::rdf::Node(rdf4cpp::rdf::IRI("http://www.w3.org/1999/02/22-rdf-syntax-ns#type"));
		else if (path_primary_ctx->NEGATION())
			throw std::runtime_error("Property paths are not supported yet");
		else
			return visitPath(ctx->pathPrimary()->path());
	}

	std::any SelectAskQueryVisitor::visitRdfLiteral(SparqlParser::RdfLiteralContext *ctx) {
		auto value = std::any_cast<std::string>(visitString(ctx->string()));
		if (auto iri_ctx = ctx->iri(); iri_ctx)
			return rdf4cpp::rdf::Literal::make_typed(value, std::any_cast<rdf4cpp::rdf::IRI>(visitIri(iri_ctx)));
		else if (auto langtag_ctx = ctx->LANGTAG(); langtag_ctx)
			return rdf4cpp::rdf::Literal::make_lang_tagged(value, langtag_ctx->getText().substr(1));
		else
			return rdf4cpp::rdf::Literal::make_simple(value);
	}

	std::any SelectAskQueryVisitor::visitNumericLiteral(SparqlParser::NumericLiteralContext *ctx) {
		auto number = ctx->getText();
		if (auto pos_literal_ctx = ctx->numericLiteralPositive(); pos_literal_ctx) {
			if (pos_literal_ctx->DECIMAL_POSITIVE())
				return rdf4cpp::rdf::Literal::make_typed(number, rdf4cpp::rdf::IRI("http://www.w3.org/2001/XMLSchema#decimal"));
			else if (pos_literal_ctx->DOUBLE_POSITIVE())
				return rdf4cpp::rdf::Literal::make_typed(number, rdf4cpp::rdf::IRI("http://www.w3.org/2001/XMLSchema#double"));
			else
				return rdf4cpp::rdf::Literal::make_typed(number, rdf4cpp::rdf::IRI("http://www.w3.org/2001/XMLSchema#integer"));
		} else if (auto neg_literal_ctx = ctx->numericLiteralNegative(); neg_literal_ctx) {
			if (neg_literal_ctx->DECIMAL_NEGATIVE())
				return rdf4cpp::rdf::Literal::make_typed(number, rdf4cpp::rdf::IRI("http://www.w3.org/2001/XMLSchema#decimal"));
			else if (neg_literal_ctx->DOUBLE_NEGATIVE())
				return rdf4cpp::rdf::Literal::make_typed(number, rdf4cpp::rdf::IRI("http://www.w3.org/2001/XMLSchema#double"));
			else
				return rdf4cpp::rdf::Literal::make_typed(number, rdf4cpp::rdf::IRI("http://www.w3.org/2001/XMLSchema#integer"));
		} else {
			auto unsigned_literal_ctx = ctx->numericLiteralUnsigned();
			if (unsigned_literal_ctx->DECIMAL())
				return rdf4cpp::rdf::Literal::make_typed(number, rdf4cpp::rdf::IRI("http://www.w3.org/2001/XMLSchema#decimal"));
			else if (unsigned_literal_ctx->DOUBLE())
				return rdf4cpp::rdf::Literal::make_typed(number, rdf4cpp::rdf::IRI("http://www.w3.org/2001/XMLSchema#double"));
			else
				return rdf4cpp::rdf::Literal::make_typed(number, rdf4cpp::rdf::IRI("http://www.w3.org/2001/XMLSchema#integer"));
		}
	}

	std::any SelectAskQueryVisitor::visitBooleanLiteral(SparqlParser::BooleanLiteralContext *ctx) {
		if (ctx->TRUE())
			return rdf4cpp::rdf::Literal::make_boolean(true);
		else
			return rdf4cpp::rdf::Literal::make_boolean(false);
	}

	std::any SelectAskQueryVisitor::visitString(SparqlParser::StringContext *ctx) {
		std::string value = ctx->getText();
		if (ctx->STRING_LITERAL1() or ctx->STRING_LITERAL2())
			return value.substr(1, value.size() - 2);
		else
			return value.substr(3, value.size() - 6);
	}

	void SelectAskQueryVisitor::register_var(rdf4cpp::rdf::query::Variable const &var) {
		if (query->var_to_id_.contains(var))
			return;
		query->var_to_id_[var] = var_id;
		var_id++;
	}

	void SelectAskQueryVisitor::add_tp(rdf4cpp::rdf::query::TriplePattern const &tp) {
		std::vector<char> var_ids{};
		for (auto const &node : tp) {
			if (not node.is_variable())
				continue;
			var_ids.push_back(query->var_to_id_[node.as_variable()]);
		}
		// create new node in the operand dependency graph
		auto v_id = query->odg_.add_operand(var_ids);
		auto &gp = group_patterns.back();
		// iterate over the tps of the group and capture dependencies
		for (auto iter = gp.rbegin(); iter != gp.rend(); iter++) {
			boost::container::flat_set<char> done{};// only one edge per label between two nodes
			auto const &tp_vars = query->odg_.operand_var_ids(*iter);
			bool cart = true;
			for (auto const &var : var_ids) {
				for (auto const &tp_var : tp_vars) {
					if (var == tp_var) {
						cart = false;
						if (done.contains(var))
							continue;
						done.insert(var);
						query->odg_.add_dependency(*iter, v_id, var);
						query->odg_.add_dependency(v_id, *iter, var);
					}
				}
			}
			// the triple patterns do not share a variable --> cartesian join
			if (cart) {
				query->odg_.add_dependency(*iter, v_id);
				query->odg_.add_dependency(v_id, *iter);
			}
		}
		// add current tp/node to the active group pattern
		gp.push_back(v_id);
	}

	void SelectAskQueryVisitor::group_dependencies(std::vector<uint8_t> const &prev_group,
												   std::vector<uint8_t> const &cur_group,
												   bool bidirectional) {
		// iterate of the triple patterns (nodes) of the previous group
		for (const auto &prev_tp : prev_group) {
			// get the variable ids of the node
			auto const &prev_labels = query->odg_.operand_var_ids(prev_tp);
			// iterate over the triple patterns (nodes) of the current group
			for (const auto &cur_tp : cur_group) {
				// get the variable ids of the node
				auto const &cur_labels = query->odg_.operand_var_ids(cur_tp);
				bool done = false;
				// create labelled dependencies if the nodes share variable ids
				for (auto const &prev_label : prev_labels) {
					if (std::find(cur_labels.begin(), cur_labels.end(), prev_label) != cur_labels.end()) {
						query->odg_.add_dependency(prev_tp, cur_tp, prev_label);
						if (bidirectional)
							query->odg_.add_dependency(cur_tp, prev_tp, prev_label);
						done = true;
					}
				}
				// if the nodes do not share a label, create an unlabelled dependency
				if (not done) {
					query->odg_.add_dependency(prev_tp, cur_tp);
					if (bidirectional)
						query->odg_.add_dependency(cur_tp, prev_tp);
				}
			}
		}
	}

	void SelectAskQueryVisitor::group_connections(std::vector<uint8_t> const &prev_group,
												  std::vector<uint8_t> const &cur_group) {
		for (const auto &prev_tp : prev_group) {
			for (const auto &cur_tp : cur_group) {
				query->odg_.add_connection(prev_tp, cur_tp);
				query->odg_.add_connection(cur_tp, prev_tp);
			}
		}
	}

}// namespace dice::sparql2tensor::parser::visitors