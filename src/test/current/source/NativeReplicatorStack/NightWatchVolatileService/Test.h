// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace V1ReplPerf
{
    class Test
    {
    public:
        static NightWatchTXRService::PerfResult::SPtr Execute(NightWatchTXRService::NightWatchTestParameters const & testParameters, Common::ComPointer<IFabricStateReplicator> const & replicator, KAllocator & allocator);

    private:
        static void ReplicateServiceEntry(ServiceEntry & entry, Test & test);
        static void StartReplicatingIfNeeded(ServiceEntry & entry, Test & test);

        Test(
            NightWatchTXRService::NightWatchTestParameters const & parameters,
            Common::ComPointer<IFabricStateReplicator> const & replicator);

        NightWatchTXRService::PerfResult::SPtr Run(KAllocator & allocator);

        Common::atomic_long numberOfOperations_;
        Common::atomic_long inflightOperationCount_;
        NightWatchTXRService::NightWatchTestParameters const & parameters_;
        Common::ComPointer<IFabricStateReplicator> replicator_;
        std::vector<ServiceEntry> data_;
    };
}
