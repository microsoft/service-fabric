// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class CopyStatistics
    {
    private:
        static const size_t PartialCopyIndex;
        static const size_t FullCopyIndex;
        static const size_t FileStreamCopyIndex;
        static const size_t FileStreamRebuildCopyIndex;
        static const size_t FalseProgressIndex;
        static const size_t StaleSecondaryIndex;

    public:
        CopyStatistics();

        uint64 OnPartialCopy();
        uint64 OnFullCopy();
        uint64 OnFileStreamFullCopy();
        uint64 OnFileStreamRebuildFullCopy();
        uint64 OnFalseProgress();
        uint64 OnStaleSecondary();

        uint64 GetPartialCopyCount();
        uint64 GetFullCopyCount();
        uint64 GetFileStreamFullCopyCount();
        uint64 GetFileStreamRebuildFullCopyCount();
        uint64 GetFalseProgressCount();
        uint64 GetStaleSecondaryCount();

    private:
        uint64 IncrementCount(size_t index);
        uint64 GetCount(size_t index);

        std::vector<uint64> stats_;
        Common::RwLock statsLock_;
    };
}
