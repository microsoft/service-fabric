// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        // Version Manager Factory is a test class that is being exposed tp enable upper layer components to be tested.
        // DO NOT USE IT IN PRODUCTION.
        class VersionManagerFactory
        {
        public:
            static NTSTATUS Create(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in TxnReplicator::IVersionProvider & versionProvider,
                __in KAllocator & allocator,
                __out TxnReplicator::IVersionManager::SPtr & versionManager);
        };
    }
}
