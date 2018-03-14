// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#define STORE_TRACE_COMPONENT_TAG 'tcTS'

namespace Data
{
    namespace TStore
    {
        class StoreTraceComponent :
            public KObject<StoreTraceComponent>,
            public KShared<StoreTraceComponent>
        {
            K_FORCE_SHARED(StoreTraceComponent)

        public:
            static NTSTATUS Create(
                __in KGuid partitionId,
                __in FABRIC_REPLICA_ID replicaId,
                __in LONG64 stateProviderId,
                __in KAllocator & allocator,
                __out SPtr & result);

            __declspec(property(get = get_PartitionId)) Common::Guid PartitionId;
            Common::Guid get_PartitionId()
            {
                return partitionId_;
            }

            __declspec(property(get = get_TraceTag)) Common::WStringLiteral TraceTag;
            Common::WStringLiteral get_TraceTag()
            {
                return traceTag_;
            }

            __declspec(property(get = get_AssertTag)) Common::WStringLiteral AssertTag;
            Common::WStringLiteral get_AssertTag()
            {
                return assertTag_;
            }

        private:
            StoreTraceComponent(
                __in KGuid partitionId,
                __in FABRIC_REPLICA_ID replicaId,
                __in LONG64 stateProviderId);

            KLocalString<2 * KStringView::Max64BitNumString + 2> traceTagString_;
            KLocalString<KStringView::MaxGuidString + 2 * KStringView::Max64BitNumString + 3> assertTagString_;

            Common::Guid partitionId_;
            Common::WStringLiteral traceTag_;
            Common::WStringLiteral assertTag_;
        };
    }
}
