// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace TStore
    {
        class CopyPerformanceCounterWriter : PerformanceCounterWriter
        {
            DENY_COPY(CopyPerformanceCounterWriter)
        public:
            CopyPerformanceCounterWriter(__in StorePerformanceCountersSPtr & perfCounters);

            __declspec(property(get = get_AvgDiskTransferBytesPerSec)) ULONG64 AvgDiskTransferBytesPerSec;
            ULONG64 get_AvgDiskTransferBytesPerSec() const
            {
                return GetBytesPerSecond(
                    totalDiskTransferBytes_ + deltaDiskTransferBytes_,
                    totalDiskTransferTicks_ + static_cast<ULONG64>(diskTransferWatch_.ElapsedTicks));
            }

            void StartMeasurement();

            void StopMeasurement(__in ULONG32 bytesTransferred = 0);

            void UpdatePerformanceCounter();

        private:
            static ULONG64 GetBytesPerSecond(__in ULONG64 bytes, __in ULONG64 ticks);

            static ULONG64 const DiskTransferBytesThreshold;
            ULONG64 totalDiskTransferBytes_;
            ULONG64 totalDiskTransferTicks_;
            ULONG64 deltaDiskTransferBytes_;
            Common::Stopwatch diskTransferWatch_;
        };
    }
}
