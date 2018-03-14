// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <ktl.h>
#include <Common/KtlSF.Common.h>

namespace Common
{
    #if KTL_USER_MODE

        static KSpinLock _gs_GetSFDefaultKtlSystemLock;

        KtlSystem& GetSFDefaultKtlSystem()
        {
            static KtlSystem* volatile system = nullptr;

            if (system != nullptr)
            {
                // High perf path (normal)
                return *system;
            }

            _gs_GetSFDefaultKtlSystemLock.Acquire();
            {
                if (system == nullptr)
                {
                    KGuid       thisSystemGuid;
                    thisSystemGuid.CreateNew();

                    KtlSystemBase* newSys = new KtlSystemBase(thisSystemGuid, CommonConfig::GetConfig().KtlSystemThreadLimit);
                    KInvariant(newSys != nullptr);
                    newSys->Activate(30 * 1000);                // Allow a long time to activate
                    KInvariant(NT_SUCCESS(newSys->Status()));

                    newSys->SetStrictAllocationChecks(false);   // assume process unload as shutdown

#ifndef DBG
                    // Allocation counter disabling adds huge benefits to replication scenarios with high density
                    // where the allocs per second is high
                    newSys->SetEnableAllocationCounters(false);
                    KAllocatorSupport::gs_AllocCountsDisabled = true;
#endif

                    // NOTE: This is needed for high performance of v2 replicator stack as more threads in the threadpool
                    // are used to perform completions of awaitables rather than using current thread's local work items queue
                    newSys->SetDefaultSystemThreadPoolUsage(FALSE);

                    system = newSys;
                }
            }
            _gs_GetSFDefaultKtlSystemLock.Release();

            return *system;
        }

        KAllocator& GetSFDefaultPagedKAllocator()
        {
            static  KAllocator* volatile allocator = nullptr;

            if (allocator == nullptr)
            {
                // Races are fine
                allocator = &GetSFDefaultKtlSystem().PagedAllocator();
            }
            return *allocator;
        }

        KAllocator& GetSFDefaultNonPagedKAllocator()
        {
            static  KAllocator* volatile allocator = nullptr;

            if (allocator == nullptr)
            {
                // Races are fine
                allocator = &GetSFDefaultKtlSystem().NonPagedAllocator();
            }
            return *allocator;
        }

    #endif
}
