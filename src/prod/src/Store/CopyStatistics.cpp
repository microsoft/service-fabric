// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Store;

const size_t CopyStatistics::PartialCopyIndex = 0;
const size_t CopyStatistics::FullCopyIndex = 1;
const size_t CopyStatistics::FileStreamCopyIndex = 2;
const size_t CopyStatistics::FileStreamRebuildCopyIndex = 3;
const size_t CopyStatistics::FalseProgressIndex = 4;
const size_t CopyStatistics::StaleSecondaryIndex = 5;

CopyStatistics::CopyStatistics()
    : stats_()
    , statsLock_()
{
    stats_.resize(6);
    stats_[PartialCopyIndex] = 0;
    stats_[FullCopyIndex] = 0;
    stats_[FileStreamCopyIndex] = 0;
    stats_[FileStreamRebuildCopyIndex] = 0;
    stats_[FalseProgressIndex] = 0;
    stats_[StaleSecondaryIndex] = 0;
}

uint64 CopyStatistics::OnPartialCopy()
{
    return IncrementCount(PartialCopyIndex);
}

uint64 CopyStatistics::OnFullCopy()
{
    return IncrementCount(FullCopyIndex);
}

uint64 CopyStatistics::OnFileStreamFullCopy()
{
    return IncrementCount(FileStreamCopyIndex);
}

uint64 CopyStatistics::OnFileStreamRebuildFullCopy()
{
    return IncrementCount(FileStreamRebuildCopyIndex);
}

uint64 CopyStatistics::OnFalseProgress()
{
    return IncrementCount(FalseProgressIndex);
}

uint64 CopyStatistics::OnStaleSecondary()
{
    return IncrementCount(StaleSecondaryIndex);
}

uint64 CopyStatistics::GetPartialCopyCount()
{
    return GetCount(PartialCopyIndex);
}

uint64 CopyStatistics::GetFullCopyCount()
{
    return GetCount(FullCopyIndex);
}

uint64 CopyStatistics::GetFileStreamFullCopyCount()
{
    return GetCount(FileStreamCopyIndex);
}

uint64 CopyStatistics::GetFileStreamRebuildFullCopyCount()
{
    return GetCount(FileStreamRebuildCopyIndex);
}

uint64 CopyStatistics::GetFalseProgressCount()
{
    return GetCount(FalseProgressIndex);
}

uint64 CopyStatistics::GetStaleSecondaryCount()
{
    return GetCount(StaleSecondaryIndex);
}

uint64 CopyStatistics::IncrementCount(size_t index)
{
    AcquireWriteLock lock(statsLock_);

    return ++stats_[index];
}

uint64 CopyStatistics::GetCount(size_t index)
{
    AcquireReadLock lock(statsLock_);

    return stats_[index];
}
