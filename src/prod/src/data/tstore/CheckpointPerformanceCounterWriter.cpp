// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::TStore;

CheckpointPerformanceCounterWriter::CheckpointPerformanceCounterWriter(__in StorePerformanceCountersSPtr & perfCounters)
    : PerformanceCounterWriter(perfCounters != nullptr ? &(perfCounters->CheckpointFileWriteBytesPerSec) : nullptr)
{

}

void CheckpointPerformanceCounterWriter::StartMeasurement()
{
    diskWriteWatch_.Start();
}

void CheckpointPerformanceCounterWriter::StopMeasurement()
{
    diskWriteWatch_.Stop();
}

void CheckpointPerformanceCounterWriter::ResetMeasurement()
{
    diskWriteWatch_.Reset();
}

ULONG64 CheckpointPerformanceCounterWriter::UpdatePerformanceCounter(__in ULONG64 bytesWritten)
{
    ULONG64 writeTicks = static_cast<ULONG64>(diskWriteWatch_.ElapsedTicks);
    ULONG64 writeBytesPerSec = bytesWritten * Constants::TicksPerSecond / (writeTicks + 1);

    if (this->IsEnabled())
    {
        this->Value = writeBytesPerSec;
    }

    return writeBytesPerSec;
}
