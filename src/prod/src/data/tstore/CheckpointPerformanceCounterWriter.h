// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace TStore
    {
        class CheckpointPerformanceCounterWriter : PerformanceCounterWriter
        {
            DENY_COPY(CheckpointPerformanceCounterWriter)
        public:
            CheckpointPerformanceCounterWriter(__in StorePerformanceCountersSPtr & perfCounters);

            void StartMeasurement();

            void StopMeasurement();

            void ResetMeasurement();

            ULONG64 UpdatePerformanceCounter(__in ULONG64 bytesWritten);

        private:
            Common::Stopwatch diskWriteWatch_;
        };
    }
}
