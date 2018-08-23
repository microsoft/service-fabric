// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::TStore;

CopyPerformanceCounterWriter::CopyPerformanceCounterWriter(__in StorePerformanceCountersSPtr & perfCounters)
    : PerformanceCounterWriter(perfCounters != nullptr ? &(perfCounters->CopyDiskTransferBytesPerSec) : nullptr)
    , totalDiskTransferBytes_(0)
    , totalDiskTransferTicks_(0)
    , deltaDiskTransferBytes_(0)
{

}

void CopyPerformanceCounterWriter::StartMeasurement()
{
    diskTransferWatch_.Start();
}

void CopyPerformanceCounterWriter::StopMeasurement(__in ULONG32 bytesTransferred)
{
    diskTransferWatch_.Stop();
    deltaDiskTransferBytes_ += static_cast<ULONG64>(bytesTransferred);

    if (deltaDiskTransferBytes_ >= DiskTransferBytesThreshold)
    {
        UpdatePerformanceCounter();
    }
}

void CopyPerformanceCounterWriter::UpdatePerformanceCounter()
{
    auto deltaDiskTransferTicks = static_cast<ULONG64>(diskTransferWatch_.ElapsedTicks);

    if (this->IsEnabled() && deltaDiskTransferBytes_ > 0)
    {
        this->Value = GetBytesPerSecond(deltaDiskTransferBytes_, deltaDiskTransferTicks);
    }

    totalDiskTransferBytes_ += deltaDiskTransferBytes_;
    totalDiskTransferTicks_ += deltaDiskTransferTicks;
    diskTransferWatch_.Reset();
    deltaDiskTransferBytes_ = 0;
}

ULONG64 CopyPerformanceCounterWriter::GetBytesPerSecond(__in ULONG64 bytes, __in ULONG64 ticks)
{
    return bytes * Constants::TicksPerSecond / (ticks + 1);
}

ULONG64 const CopyPerformanceCounterWriter::DiskTransferBytesThreshold = 10 * 1024 * 1024;
