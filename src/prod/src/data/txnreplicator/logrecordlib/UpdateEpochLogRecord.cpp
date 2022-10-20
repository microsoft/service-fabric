// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;
using namespace Data::Utilities;

const ULONG UpdateEpochLogRecord::DiskSpaceUsed = sizeof(LONG32) + (4 * sizeof(LONG64));
const ULONG UpdateEpochLogRecord::SizeOnWireIncrement = sizeof(LONG32) + (4 * sizeof(LONG64));

UpdateEpochLogRecord::UpdateEpochLogRecord(
    __in LogRecordType::Enum recordType,
    __in ULONG64 recordPosition,
    __in LONG64 lsn,
    __in PhysicalLogRecord & invalidPhysicalLogRecord)
    : LogicalLogRecord(recordType, recordPosition, lsn, invalidPhysicalLogRecord)
    , epoch_(Epoch::InvalidEpoch())
    , primaryReplicaId_(Constants::InvalidReplicaId)
    , timeStamp_(Common::DateTime::Zero)
{
    ASSERT_IFNOT(
        recordType == LogRecordType::Enum::UpdateEpoch,
        "Invalid update epoch log record in constructor: {0}", recordType);
}

UpdateEpochLogRecord::UpdateEpochLogRecord(
    __in Epoch const & epoch,
    __in LONG64 primaryReplicaId,
    __in PhysicalLogRecord & invalidPhysicalLogRecord)
    : LogicalLogRecord(LogRecordType::Enum::UpdateEpoch, invalidPhysicalLogRecord)
    , epoch_(epoch)
    , primaryReplicaId_(primaryReplicaId)
    , timeStamp_(Common::DateTime::Now())
{
    UpdateApproximateDiskSize();
}

UpdateEpochLogRecord::UpdateEpochLogRecord(__in PhysicalLogRecord & invalidPhysicalLogRecord)
    : LogicalLogRecord(LogRecordType::Enum::UpdateEpoch, invalidPhysicalLogRecord)
    , epoch_(Epoch::ZeroEpoch())
    , primaryReplicaId_(Constants::UniversalReplicaId)
{
    Lsn = Constants::ZeroLsn;
}

UpdateEpochLogRecord::~UpdateEpochLogRecord()
{
}

UpdateEpochLogRecord::SPtr UpdateEpochLogRecord::CreateZeroUpdateEpochLogRecord(
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in KAllocator & allocator)
{
    UpdateEpochLogRecord * pointer = _new(UPDATEEPOCHLOGRECORD_TAG, allocator)UpdateEpochLogRecord(invalidPhysicalLogRecord);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return UpdateEpochLogRecord::SPtr(pointer);
}

std::wstring UpdateEpochLogRecord::ToString() const
{
    std::wstring logRecordString = Constants::CompEndlJSON;

    logRecordString += L"Primary Replica Id" + Constants::DivisionJSON +
        std::to_wstring(primaryReplicaId_) + Constants::Quote;
    logRecordString += Constants::CloseJSON;

    return __super::ToString() + logRecordString;
}

UpdateEpochLogRecord::SPtr UpdateEpochLogRecord::Create(
    __in LogRecordType::Enum recordType,
    __in ULONG64 recordPosition,
    __in LONG64 lsn,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in KAllocator & allocator)
{
    UpdateEpochLogRecord* pointer = _new(UPDATEEPOCHLOGRECORD_TAG, allocator) UpdateEpochLogRecord(
        recordType, 
        recordPosition, 
        lsn, 
        invalidPhysicalLogRecord);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return UpdateEpochLogRecord::SPtr(pointer);
}

UpdateEpochLogRecord::SPtr UpdateEpochLogRecord::Create(
    __in Epoch const & epoch,
    __in LONG64 primaryReplicaId,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in KAllocator & allocator)
{
    UpdateEpochLogRecord* pointer = _new(UPDATEEPOCHLOGRECORD_TAG, allocator) UpdateEpochLogRecord(
        epoch, 
        primaryReplicaId, 
        invalidPhysicalLogRecord);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return UpdateEpochLogRecord::SPtr(pointer);
}

void UpdateEpochLogRecord::UpdateApproximateDiskSize()
{
    ApproximateSizeOnDisk = ApproximateSizeOnDisk + DiskSpaceUsed;
}

void UpdateEpochLogRecord::ReadLogical(
    __in OperationData const & operationData,
    __in INT & index)
{
    __super::ReadLogical(operationData, index);
    KBuffer::CSPtr buffer = OperationData::IncrementIndexAndGetBuffer(operationData, index);
    BinaryReader reader(*buffer, GetThisAllocator());
    ReadPrivate(reader, false);
}

void UpdateEpochLogRecord::Read(
    __in BinaryReader & binaryReader,
    __in bool isPhysicalRead)
{
    __super::Read(binaryReader, isPhysicalRead);
    ReadPrivate(binaryReader, isPhysicalRead);
}

void UpdateEpochLogRecord::ReadPrivate(
    __in BinaryReader & binaryReader,
    __in bool isPhysicalRead)
{
    UNREFERENCED_PARAMETER(isPhysicalRead);

    ULONG32 startingPosition = binaryReader.Position;
    ULONG32 sizeOfSection;
    binaryReader.Read(sizeOfSection);
    ULONG32 endPosition = startingPosition + sizeOfSection;

    LONG64 dataLossVersion;
    LONG64 configurationVersion;
    binaryReader.Read(dataLossVersion);
    binaryReader.Read(configurationVersion);

    epoch_ = Epoch(dataLossVersion, configurationVersion);
    binaryReader.Read(primaryReplicaId_);

    LONG64 ticks;
    binaryReader.Read(ticks);
    timeStamp_ = Common::DateTime(ticks);
    
    ASSERT_IFNOT(
        endPosition >= binaryReader.Position,
        "Could not have read more than section size: {0} {1}", endPosition, binaryReader.Position);

    binaryReader.Position = endPosition;

    UpdateApproximateDiskSize();
}

void UpdateEpochLogRecord::Write(
    __in BinaryWriter & binaryWriter,
    __inout OperationData & operationData,
    __in bool isPhysicalWrite,
    __in bool forceRecomputeOffsets)
{
    __super::Write(binaryWriter, operationData, isPhysicalWrite, forceRecomputeOffsets);

    ULONG32 startingPosition = binaryWriter.Position;
    binaryWriter.Position += sizeof(ULONG32);

    binaryWriter.Write(epoch_.DataLossVersion);
    binaryWriter.Write(epoch_.ConfigurationVersion);
    binaryWriter.Write(primaryReplicaId_);    
    binaryWriter.Write(timeStamp_.Ticks);

    ULONG32 endPosition = binaryWriter.Position;
    ULONG32 sizeOfSection = endPosition - startingPosition;

    binaryWriter.Position = startingPosition;
    binaryWriter.Write(sizeOfSection);
    binaryWriter.Position = endPosition;

    KBuffer::SPtr kBuffer = binaryWriter.GetBuffer(startingPosition);
    operationData.Append(*kBuffer);
}

ULONG UpdateEpochLogRecord::GetSizeOnWire() const
{
    return __super::GetSizeOnWire() + SizeOnWireIncrement;
}

bool UpdateEpochLogRecord::Test_Equals(__in LogRecord const & other) const
{
    UpdateEpochLogRecord const & otherUpdateEpochLogRecord = dynamic_cast<UpdateEpochLogRecord const &>(other);

    if (__super::Test_Equals(other))
    {
        return
            epoch_ == otherUpdateEpochLogRecord.epoch_ &&
            primaryReplicaId_ == otherUpdateEpochLogRecord.primaryReplicaId_ &&
            timeStamp_ == otherUpdateEpochLogRecord.timeStamp_;
    }

    return false;
}
