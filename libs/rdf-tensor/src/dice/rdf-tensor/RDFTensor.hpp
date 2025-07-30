#ifndef TENTRIS_RDFTENSOR_HPP
#define TENTRIS_RDFTENSOR_HPP

#include "dice/rdf-tensor/HypertrieTrait.hpp"
#include "dice/rdf-tensor/metall_manager.hpp"

namespace dice::rdf_tensor {
	using HypertrieContext = dice::hypertrie::HypertrieContext<htt_t, allocator_type>;
	using BoolHypertrie = dice::hypertrie::Hypertrie<htt_t, allocator_type>;
	using const_BoolHypertrie = dice::hypertrie::const_Hypertrie<htt_t, allocator_type>;
	using HypertrieBulkInserter = dice::hypertrie::AsyncBulkInserter<htt_t, allocator_type>;
	using HypertrieSyncBulkInserter = dice::hypertrie::SyncBulkInserter<htt_t, allocator_type>;
	using HypertrieBulkRemover = dice::hypertrie::AsyncBulkRemover<htt_t, allocator_type>;
	using HypertrieSyncBulkRemover = dice::hypertrie::SyncBulkRemover<htt_t, allocator_type>;
	using HypertrieContext_ptr = dice::hypertrie::HypertrieContext_ptr<htt_t, allocator_type>;
}// namespace dice::rdf-tensor
#endif//TENTRIS_RDFTENSOR_HPP
