// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;
using namespace Data::Utilities;

ULONG const ProgressVectorEntry::SerializedByteCount = 40;
ProgressVectorEntry const ProgressVectorEntry::ZeroProgressVectorEntry_ = ProgressVectorEntry(Epoch::ZeroEpoch(), Constants::ZeroLsn, Constants::InvalidReplicaId, Common::DateTime::Now());

LONG64 ProgressVectorEntry::SizeOfStringInBytes_ = -1;

ProgressVectorEntry::ProgressVectorEntry()
    : KObject()
    , epoch_(Epoch::InvalidEpoch())
    , lsn_(Constants::InvalidLsn)
    , primaryReplicaId_(Constants::InvalidReplicaId)
    , timeStamp_(Common::DateTime::Zero)
{
}

ProgressVectorEntry::ProgressVectorEntry(__in ProgressVectorEntry const & other)
    : epoch_(other.epoch_)
    , lsn_(other.lsn_)
    , primaryReplicaId_(other.primaryReplicaId_)
    , timeStamp_(other.timeStamp_)
{
}

ProgressVectorEntry::ProgressVectorEntry(__in UpdateEpochLogRecord const & record)
    : ProgressVectorEntry(record.EpochValue, record.Lsn, record.PrimaryReplicaId, record.TimeStamp)
{
}

ProgressVectorEntry::ProgressVectorEntry(
    __in Epoch const & epoch,
    __in LONG64 lsn,
    __in LONG64 primaryReplicaId,
    __in Common::DateTime timeStamp)
    : KObject()
    , epoch_(epoch)
    , lsn_(lsn)
    , primaryReplicaId_(primaryReplicaId)
    , timeStamp_(timeStamp)
{
}

ProgressVectorEntry const & ProgressVectorEntry::ZeroProgressVectorEntry()
{
    return ZeroProgressVectorEntry_;
}

LONG64 const ProgressVectorEntry::SizeOfStringInBytes()
{
    if(SizeOfStringInBytes_ == -1)
    {
        SizeOfStringInBytes_ = ZeroProgressVectorEntry().ToString().length() * sizeof(char);
    }

    return SizeOfStringInBytes_;
}

bool ProgressVectorEntry::operator==(__in ProgressVectorEntry const & other) const
{
    if(epoch_ == other.epoch_)
    {
        if(lsn_ == other.lsn_)
        {
            return true;
        }
    }

    return false;
}

bool ProgressVectorEntry::operator>(__in ProgressVectorEntry const & other) const
{
    if (epoch_.DataLossVersion > other.epoch_.DataLossVersion)
    {
        return true;
    }

    if (epoch_.DataLossVersion < other.epoch_.DataLossVersion)
    {
        return false;
    }

    if (epoch_.ConfigurationVersion > other.epoch_.ConfigurationVersion)
    {
        return true;
    }

    if (epoch_.ConfigurationVersion < other.epoch_.ConfigurationVersion)
    {
        return false;
    }

    return lsn_ > other.lsn_;
}

bool ProgressVectorEntry::operator>=(__in ProgressVectorEntry const & other) const
{
    return (*this < other) == false;
}

bool ProgressVectorEntry::operator!=(__in ProgressVectorEntry const & other) const
{
    return (*this == other) == false;
}

bool ProgressVectorEntry::operator<(__in ProgressVectorEntry const & other) const
{
    if (epoch_.DataLossVersion < other.epoch_.DataLossVersion)
    {
        return true;
    }

    if (epoch_.DataLossVersion > other.epoch_.DataLossVersion)
    {
        return false;
    }

    if (epoch_.ConfigurationVersion < other.epoch_.ConfigurationVersion)
    {
        return true;
    }

    if (epoch_.ConfigurationVersion > other.epoch_.ConfigurationVersion)
    {
        return false;
    }

    return lsn_ < other.lsn_;
}

bool ProgressVectorEntry::operator<=(__in ProgressVectorEntry const & other) const
{
    return (*this > other) == false;
}

void ProgressVectorEntry::Read(
    __in BinaryReader & binaryReader, 
    __in bool isPhysicalRead)
{
    UNREFERENCED_PARAMETER(isPhysicalRead);

    LONG64 dataLossNumber;
    LONG64 configurationNumber;
    binaryReader.Read(dataLossNumber);
    binaryReader.Read(configurationNumber);
    epoch_ = Epoch(dataLossNumber, configurationNumber);
    binaryReader.Read(lsn_);
    binaryReader.Read(primaryReplicaId_);
    
    LONG64 ticks;
    binaryReader.Read(ticks);
    timeStamp_ = Common::DateTime(ticks);
}

void ProgressVectorEntry::Write(__in BinaryWriter& binaryWriter)
{
    ULONG startingPos = binaryWriter.Position;

    binaryWriter.Write(epoch_.DataLossVersion);
    binaryWriter.Write(epoch_.ConfigurationVersion);
    binaryWriter.Write(lsn_);
    binaryWriter.Write(primaryReplicaId_);
    binaryWriter.Write(timeStamp_.Ticks);

    ASSERT_IFNOT(
        (binaryWriter.Position - startingPos) == SerializedByteCount,
        "Incorrect progress vector size during serialization: {0}, expected: {1}", 
        (binaryWriter.Position - startingPos),
        SerializedByteCount);
}

bool ProgressVectorEntry::IsDataLossBetween(__in ProgressVectorEntry const & other) const
{
    return epoch_.DataLossVersion != other.CurrentEpoch.DataLossVersion;
}

std::wstring ProgressVectorEntry::ToString() const
{
    std::wstring output;
    Common::StringWriter writer(output);
    writer.Write(
        "[({0},{1}), {2}, {3}, {4}]",
        epoch_.DataLossVersion,
        epoch_.ConfigurationVersion,
        lsn_,
        primaryReplicaId_,
        timeStamp_.Ticks);

    writer.Flush();
    return output;
}
