// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::LoggingReplicator;
using namespace TxnReplicator;
using namespace Data::Utilities;

LONG64 ReplicatorBackup::InvalidLogSize = -1;
LONG64 ReplicatorBackup::InvalidDataLossNumber = -1;
LONG64 ReplicatorBackup::InvalidConfigurationNumber = -1;

#define NumberOfBytesInKB 1024.0
#define NumberOfBytesInMB 1048576.0

ReplicatorBackup ReplicatorBackup::Invalid()
{
    return ReplicatorBackup();
}

ULONG32 ReplicatorBackup::get_LogCount() const
{
    return logCount_;
}

LONG64 ReplicatorBackup::get_AccumulatedLogSizeInKB() const
{
    return static_cast<LONG64>(ceil(accumulatedLogSize_ / NumberOfBytesInKB));
}

LONG64 ReplicatorBackup::get_AccumulatedLogSizeInMB() const
{
    return static_cast<LONG64>(ceil(accumulatedLogSize_ / NumberOfBytesInMB));
}

FABRIC_EPOCH ReplicatorBackup::get_EpochOfFirstBackedUpLogRecord() const
{
    return epochOfFirstLogicalLogRecord_;
}

FABRIC_SEQUENCE_NUMBER ReplicatorBackup::get_LsnOfFirstLogicalLogRecord() const
{
    return lsnOfFirstLogicalLogRecord_;
}

FABRIC_EPOCH ReplicatorBackup::get_EpochOfHighestBackedUpLogRecord() const
{
    return epochOfHighestBackedUpLogRecord_;
}

FABRIC_SEQUENCE_NUMBER ReplicatorBackup::get_LsnOfHighestBackedUpLogRecord() const
{
    return lsnOfHighestBackedUpLogRecord_;
}

ReplicatorBackup::ReplicatorBackup()
    : logCount_(0)
    , logSize_(InvalidLogSize)
    , accumulatedLogSize_(InvalidLogSize)
    , lsnOfFirstLogicalLogRecord_(FABRIC_INVALID_SEQUENCE_NUMBER)
    , lsnOfHighestBackedUpLogRecord_(FABRIC_INVALID_SEQUENCE_NUMBER)
{
    FABRIC_EPOCH invalidEpoch;
    invalidEpoch.DataLossNumber = InvalidDataLossNumber;;
    invalidEpoch.ConfigurationNumber = InvalidConfigurationNumber;

    epochOfFirstLogicalLogRecord_ = invalidEpoch;
    epochOfHighestBackedUpLogRecord_ = invalidEpoch;
}

ReplicatorBackup::ReplicatorBackup(
    __in ULONG32 logCount,
    __in LONG64 logSize,
    __in LONG64 accumulatedLogSize,
    __in FABRIC_EPOCH epochOfFirstLogicalLogRecord,
    __in FABRIC_SEQUENCE_NUMBER lsnOfFirstLogicalLogRecord,
    __in FABRIC_EPOCH epochOfHighestBackedUpLogRecord,
    __in FABRIC_SEQUENCE_NUMBER lsnOfHighestBackedUpLogRecord)
    : logCount_(logCount)
    , logSize_(logSize)
    , accumulatedLogSize_(accumulatedLogSize)
    , epochOfFirstLogicalLogRecord_(epochOfFirstLogicalLogRecord)
    , lsnOfFirstLogicalLogRecord_(lsnOfFirstLogicalLogRecord)
    , epochOfHighestBackedUpLogRecord_(epochOfHighestBackedUpLogRecord)
    , lsnOfHighestBackedUpLogRecord_(lsnOfHighestBackedUpLogRecord)
{
}

ReplicatorBackup & ReplicatorBackup::operator=(
    __in ReplicatorBackup const & other)
{
    if (&other == this)
    {
        return *this;
    }

    logCount_ = other.logCount_;
    logSize_ = other.logSize_;
    accumulatedLogSize_ = other.accumulatedLogSize_;

    epochOfFirstLogicalLogRecord_ = other.epochOfFirstLogicalLogRecord_;
    lsnOfFirstLogicalLogRecord_ = other.lsnOfFirstLogicalLogRecord_;
    epochOfHighestBackedUpLogRecord_ = other.epochOfHighestBackedUpLogRecord_;
    lsnOfHighestBackedUpLogRecord_ = other.lsnOfHighestBackedUpLogRecord_;

    return *this;
}
