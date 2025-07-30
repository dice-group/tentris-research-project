#include "PersistentNodeStorageBackend.hpp"
namespace dice::node_store {

	PersistentNodeStorageBackend::PersistentNodeStorageBackend(PersistentNodeStorageBackendImpl *impl)
		: INodeStorageBackend(), impl_(impl) {}
	size_t PersistentNodeStorageBackend::size() const noexcept {
		return impl_->size();
	}
	bool PersistentNodeStorageBackend::has_specialized_storage_for(rdf4cpp::rdf::storage::node::identifier::LiteralType datatype) const noexcept {
		return impl_->has_specialized_storage_for(datatype);
	}
	rdf4cpp::rdf::storage::node::identifier::NodeID PersistentNodeStorageBackend::find_or_make_id(const rdf4cpp::rdf::storage::node::view::BNodeBackendView &view) noexcept {
		return impl_->find_or_make_id(view);
	}
	rdf4cpp::rdf::storage::node::identifier::NodeID PersistentNodeStorageBackend::find_or_make_id(const rdf4cpp::rdf::storage::node::view::IRIBackendView &view) noexcept {
		return impl_->find_or_make_id(view);
	}
	rdf4cpp::rdf::storage::node::identifier::NodeID PersistentNodeStorageBackend::find_or_make_id(const rdf4cpp::rdf::storage::node::view::LiteralBackendView &view) noexcept {
		return impl_->find_or_make_id(view);
	}
	rdf4cpp::rdf::storage::node::identifier::NodeID PersistentNodeStorageBackend::find_or_make_id(const rdf4cpp::rdf::storage::node::view::VariableBackendView &view) noexcept {
		return impl_->find_or_make_id(view);
	}
	rdf4cpp::rdf::storage::node::identifier::NodeID PersistentNodeStorageBackend::find_id(const rdf4cpp::rdf::storage::node::view::BNodeBackendView &view) const noexcept {
		return impl_->find_id(view);
	}
	rdf4cpp::rdf::storage::node::identifier::NodeID PersistentNodeStorageBackend::find_id(const rdf4cpp::rdf::storage::node::view::IRIBackendView &view) const noexcept {
		return impl_->find_id(view);
	}
	rdf4cpp::rdf::storage::node::identifier::NodeID PersistentNodeStorageBackend::find_id(const rdf4cpp::rdf::storage::node::view::LiteralBackendView &view) const noexcept {
		return impl_->find_id(view);
	}
	rdf4cpp::rdf::storage::node::identifier::NodeID PersistentNodeStorageBackend::find_id(const rdf4cpp::rdf::storage::node::view::VariableBackendView &view) const noexcept {
		return impl_->find_id(view);
	}
	rdf4cpp::rdf::storage::node::view::IRIBackendView PersistentNodeStorageBackend::find_iri_backend_view(rdf4cpp::rdf::storage::node::identifier::NodeID id) const {
		return impl_->find_iri_backend_view(id);
	}
	rdf4cpp::rdf::storage::node::view::LiteralBackendView PersistentNodeStorageBackend::find_literal_backend_view(rdf4cpp::rdf::storage::node::identifier::NodeID id) const {
		return impl_->find_literal_backend_view(id);
	}
	rdf4cpp::rdf::storage::node::view::BNodeBackendView PersistentNodeStorageBackend::find_bnode_backend_view(rdf4cpp::rdf::storage::node::identifier::NodeID id) const {
		return impl_->find_bnode_backend_view(id);
	}
	rdf4cpp::rdf::storage::node::view::VariableBackendView PersistentNodeStorageBackend::find_variable_backend_view(rdf4cpp::rdf::storage::node::identifier::NodeID id) const {
		return impl_->find_variable_backend_view(id);
	}
	bool PersistentNodeStorageBackend::erase_iri([[maybe_unused]] rdf4cpp::rdf::storage::node::identifier::NodeID id) {
		throw std::runtime_error{"Not implemented."};
	}
	bool PersistentNodeStorageBackend::erase_literal([[maybe_unused]] rdf4cpp::rdf::storage::node::identifier::NodeID id) {
		throw std::runtime_error{"Not implemented."};
	}
	bool PersistentNodeStorageBackend::erase_bnode([[maybe_unused]] rdf4cpp::rdf::storage::node::identifier::NodeID id) {
		throw std::runtime_error{"Not implemented."};
	}
	bool PersistentNodeStorageBackend::erase_variable([[maybe_unused]] rdf4cpp::rdf::storage::node::identifier::NodeID id) {
		throw std::runtime_error{"Not implemented."};
	}
}// namespace dice::node_store