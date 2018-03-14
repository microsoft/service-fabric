// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace TStore
    {
        class Factory
        {
        public:
            static NTSTATUS Create(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in Data::IComparer<KString::SPtr>& keyComparer,
                __in KDelegate<ULONG(const KString::SPtr& Key)> func,
                __in KAllocator& allocator,
                __in KUriView& name,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in Data::StateManager::IStateSerializer<KString::SPtr>& keySerializer,
                __in Data::StateManager::IStateSerializer<KBuffer::SPtr>& valueSerializer,
                __out IStore<KString::SPtr, KBuffer::SPtr>::SPtr& result);
        };
    }
}
