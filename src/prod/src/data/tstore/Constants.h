// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace TStore
    {
        class Constants
        {
        public:
            // Temporary until Concurrent Dictionary is replaced by the actual implementation.
            static const ULONG32 NumberOfBuckets = 64;

            //
            // Alignment for disk writes and reads.
            //
            static const ULONG32 Alignment = 8;

            //
            // Constant bytes used for padding.
            // 
            static const byte BadFood[];

            //
            // Bad food Length.
            //
            static const ULONG32 BadFoodLength = 4;

            static const ULONG32 SerializedVersion = 1;

            static const ULONG32 MaxRetryCount = 16;

            static const ULONG32 StartingBackOffInMs = 8;

            // Max backoff in milliseconds.
            static const ULONG32 MaxBackOffInMs = 4 * 1024;

            static const ULONG32 InitialRecoveryComponentSize = 1024 << 3;
            
            //
            // Default number of delta components that can exist before checkpoint decides to consolidate.
            //
            static ULONG32 const DefaultNumberOfDeltasTobeConsolidated = 3;

            static const LONG64 ZeroLsn = 0;

            static const LONG64 InvalidLsn = -1;

            static const LONG64 InvalidTxnId = -1;
            static const ULONG64 InvalidKeyHash = 0;

            // Default timeout for getting values in enumeration
            static const ULONG32 EnumerationGetValueTimeoutSeconds = 4;

            // Default timeout for acquiring metadata table lock
            static const ULONG32 MetadataTableLockTimeoutMilliseconds = 1000;

            // Metadata size for VersionedItem
            static const LONG64 ValueMetadataSizeBytes = 24;

            // Used with rate performance counters and stopwatch
            static const ULONG64 TicksPerSecond = 10'000'000;
        };
    }
}
