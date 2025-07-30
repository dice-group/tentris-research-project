#ifndef TENTRIS_PERSISTENTNODESTORAGEBACKENDIMPL_HPP
#define TENTRIS_PERSISTENTNODESTORAGEBACKENDIMPL_HPP

#include <rdf4cpp/rdf/storage/node/INodeStorageBackend.hpp>

#include "dice/node-store/MetallBNodeBackend.hpp"
#include "dice/node-store/MetallIRIBackend.hpp"
#include "dice/node-store/MetallLiteralBackend.hpp"
#include "dice/node-store/MetallNodeTypeStorage.hpp"
#include "dice/node-store/MetallVariableBackend.hpp"


namespace dice::node_store {

	class PersistentNodeStorageBackendImpl {
		using RDFNodeType = rdf4cpp::rdf::storage::node::identifier::RDFNodeType;
		using NodeID = rdf4cpp::rdf::storage::node::identifier::NodeID;
		using LiteralType = rdf4cpp::rdf::storage::node::identifier::LiteralType;
		using LiteralID = rdf4cpp::rdf::storage::node::identifier::LiteralID;
		using LiteralBackendView = rdf4cpp::rdf::storage::node::view::LiteralBackendView;
		using BNodeBackendView = rdf4cpp::rdf::storage::node::view::BNodeBackendView;
		using IRIBackendView = rdf4cpp::rdf::storage::node::view::IRIBackendView;
		using VariableBackendView = rdf4cpp::rdf::storage::node::view::VariableBackendView;

	public:
		template<typename T>
		using pointer = typename metall_manager::allocator_type<T>::pointer;

	private:
		metall_manager::allocator_type<std::byte> allocator;
		MetallNodeTypeStorage<MetallBNodeBackend> bnode_storage_;
		MetallNodeTypeStorage<MetallIRIBackend> iri_storage_;
		MetallNodeTypeStorage<MetallLiteralBackend> literal_storage_;
		MetallNodeTypeStorage<MetallVariableBackend> variable_storage_;

		constexpr static rdf4cpp::rdf::storage::node::identifier::NodeStorageID manager_id = rdf4cpp::rdf::storage::node::identifier::NodeStorageID{0};

		LiteralID next_literal_id = NodeID::min_literal_id;
		NodeID next_bnode_id = NodeID::min_bnode_id;
		NodeID next_iri_id = NodeID::min_iri_id;
		NodeID next_variable_id = NodeID::min_variable_id;


	public:
		explicit PersistentNodeStorageBackendImpl(metall_manager::allocator_type<std::byte> const &allocator);

		size_t size() const noexcept;
		bool has_specialized_storage_for(rdf4cpp::rdf::storage::node::identifier::LiteralType type);

		[[nodiscard]] NodeID find_or_make_id(BNodeBackendView const &) noexcept;
		[[nodiscard]] NodeID find_or_make_id(IRIBackendView const &) noexcept;
		[[nodiscard]] NodeID find_or_make_id(LiteralBackendView const &) noexcept;
		[[nodiscard]] NodeID find_or_make_id(VariableBackendView const &) noexcept;

		[[nodiscard]] NodeID find_id(BNodeBackendView const &) const noexcept;
		[[nodiscard]] NodeID find_id(IRIBackendView const &) const noexcept;
		[[nodiscard]] NodeID find_id(LiteralBackendView const &) const noexcept;
		[[nodiscard]] NodeID find_id(VariableBackendView const &) const noexcept;

		[[nodiscard]] IRIBackendView find_iri_backend_view(NodeID id) const;
		[[nodiscard]] LiteralBackendView find_literal_backend_view(NodeID id) const;
		[[nodiscard]] BNodeBackendView find_bnode_backend_view(NodeID id) const;
		[[nodiscard]] VariableBackendView find_variable_backend_view(NodeID id) const;
	};

}// namespace dice::node_store

#endif//TENTRIS_PERSISTENTNODESTORAGEBACKENDIMPL_HPP
