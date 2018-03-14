// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        class ReplicatorBackup final
        {
        public:
            static ReplicatorBackup Invalid();

        public: // Properties
            __declspec(property(get = get_LogCount)) ULONG32 LogCount;
            ULONG32 get_LogCount() const;

            __declspec(property(get = get_AccumulatedLogSizeInKB)) LONG64 AccumulatedLogSizeInKB;
            LONG64 get_AccumulatedLogSizeInKB() const;

            __declspec(property(get = get_AccumulatedLogSizeInMB)) LONG64 AccumulatedLogSizeInMB;
            LONG64 get_AccumulatedLogSizeInMB() const;

            __declspec(property(get = get_EpochOfFirstBackedUpLogRecord)) FABRIC_EPOCH EpochOfFirstBackedUpLogRecord;
            FABRIC_EPOCH get_EpochOfFirstBackedUpLogRecord() const;

            __declspec(property(get = get_LsnOfFirstLogicalLogRecord)) FABRIC_SEQUENCE_NUMBER LsnOfFirstLogicalLogRecord;
            FABRIC_SEQUENCE_NUMBER get_LsnOfFirstLogicalLogRecord() const;

            __declspec(property(get = get_EpochOfHighestBackedUpLogRecord)) FABRIC_EPOCH EpochOfHighestBackedUpLogRecord;
            FABRIC_EPOCH get_EpochOfHighestBackedUpLogRecord() const;

            __declspec(property(get = get_LsnOfHighestBackedUpLogRecord)) FABRIC_SEQUENCE_NUMBER LsnOfHighestBackedUpLogRecord;
            FABRIC_SEQUENCE_NUMBER get_LsnOfHighestBackedUpLogRecord() const;

        public:
            // Invalid constructor
            ReplicatorBackup();

            ReplicatorBackup(
                __in ULONG32 logCount,
                __in LONG64 logSize,
                __in LONG64 accumulatedLogSize,
                __in FABRIC_EPOCH epochOfFirstLogicalLogRecord,
                __in FABRIC_SEQUENCE_NUMBER lsnOfFirstLogicalLogRecord,
                __in FABRIC_EPOCH epochOfHighestBackedUpLogRecord,
                __in FABRIC_SEQUENCE_NUMBER lsnOfHighestBackedUpLogRecord);

            ReplicatorBackup & operator=(
                __in ReplicatorBackup const & other);

        private:
            static LONG64 InvalidLogSize;
            static LONG64 InvalidDataLossNumber;
            static LONG64 InvalidConfigurationNumber;

        private:
            ULONG32 logCount_;
            LONG64 logSize_;
            LONG64 accumulatedLogSize_;

            FABRIC_EPOCH epochOfFirstLogicalLogRecord_;
            FABRIC_SEQUENCE_NUMBER lsnOfFirstLogicalLogRecord_;
            FABRIC_EPOCH epochOfHighestBackedUpLogRecord_;
            FABRIC_SEQUENCE_NUMBER lsnOfHighestBackedUpLogRecord_;
        };
    }

}
