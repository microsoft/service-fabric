// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpServer
{
    class LookasideAllocatorSettings
    {
    public:
        LookasideAllocatorSettings()
            : initialNumberOfAllocations_(0)
            , percentExtraAllocationsToMaintain_(0)
            , allocationBlockSizeInKb_(0)
        {}

        LookasideAllocatorSettings(
            ULONG initialNumberOfAllocations,
            ULONG percentExtraAllocationsToMaintain,
            ULONG allocationBlockSizeInKb)
            : initialNumberOfAllocations_(initialNumberOfAllocations)
            , percentExtraAllocationsToMaintain_(percentExtraAllocationsToMaintain)
            , allocationBlockSizeInKb_(allocationBlockSizeInKb)
        {
        }

        __declspec(property(get = get_InitialAllocs)) ULONG InitialNumberOfAllocations;
        ULONG get_InitialAllocs() const { return initialNumberOfAllocations_; }

        __declspec(property(get = get_ExtraAllocsPercent)) ULONG PercentExtraAllocationsToMaintain;
        ULONG get_ExtraAllocsPercent() const { return percentExtraAllocationsToMaintain_; }

        __declspec(property(get = get_AllocationBlockSizeInKb)) ULONG AllocationBlockSizeInKb;
        ULONG get_AllocationBlockSizeInKb() const { return allocationBlockSizeInKb_; }

    private:
        ULONG initialNumberOfAllocations_;
        ULONG percentExtraAllocationsToMaintain_;
        ULONG allocationBlockSizeInKb_;
    };
}
