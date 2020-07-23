// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;
using namespace Data::Utilities;

const ULONG InformationLogRecord::DiskSpaceUsed = 2 * sizeof(LONG32);

InformationLogRecord::InformationLogRecord()
    : PhysicalLogRecord()
    , informationEvent_(InformationEvent::Enum::Invalid)
{
}

InformationLogRecord::InformationLogRecord(
    __in LogRecordType::Enum recordType,
    __in ULONG64 recordPosition,
    __in LONG64 lsn,
    __in PhysicalLogRecord & invalidPhysicalLogRecord)
    : PhysicalLogRecord(recordType, recordPosition, lsn, invalidPhysicalLogRecord)
    , informationEvent_(InformationEvent::Enum::Invalid)
{
    ASSERT_IFNOT(recordType == LogRecordType::Information, "Expected information record type, actual: {0}", recordType);
}

InformationLogRecord::InformationLogRecord(
    __in LONG64 lsn,
    __in_opt PhysicalLogRecord * const linkedPhysicalRecord,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in InformationEvent::Enum informationEvent)
    : PhysicalLogRecord(LogRecordType::Enum::Information, lsn, linkedPhysicalRecord, invalidPhysicalLogRecord)
    , informationEvent_(informationEvent)
{
    UpdateApproximateDiskSize();
}

InformationLogRecord::~InformationLogRecord()
{
}

InformationLogRecord::SPtr InformationLogRecord::Create(
    __in LogRecordType::Enum recordType,
    __in ULONG64 recordPosition,
    __in LONG64 lsn,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in KAllocator & allocator)
{
    InformationLogRecord* pointer = _new(INFORMATIONLOGRECORD_TAG, allocator) InformationLogRecord(
        recordType, 
        recordPosition, 
        lsn, 
        invalidPhysicalLogRecord);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return InformationLogRecord::SPtr(pointer);
}

std::wstring InformationLogRecord::ToString() const
{
    std::wstring logRecordString = Constants::CompEndlJSON;

    logRecordString += L"Information Event" + Constants::DivisionJSON +
        std::to_wstring(informationEvent_) + Constants::Quote;
    logRecordString += Constants::CloseJSON;

    return __super::ToString() + logRecordString;
}

InformationLogRecord::SPtr InformationLogRecord::Create(
    __in LONG64 lsn,
    __in_opt PhysicalLogRecord * const linkedPhysicalRecord,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in InformationEvent::Enum informationEvent,
    __in KAllocator & allocator)
{
    InformationLogRecord* pointer = _new(INFORMATIONLOGRECORD_TAG, allocator) InformationLogRecord(
        lsn, 
        linkedPhysicalRecord, 
        invalidPhysicalLogRecord, 
        informationEvent);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return InformationLogRecord::SPtr(pointer);;
}

void InformationLogRecord::UpdateApproximateDiskSize()
{
    ApproximateSizeOnDisk = ApproximateSizeOnDisk + DiskSpaceUsed;
}

void InformationLogRecord::Read(
    __in BinaryReader & binaryReader,
    __in bool isPhysicalRead)
{
    __super::Read(binaryReader, isPhysicalRead);

    ULONG32 startingPosition = binaryReader.Position;
    ULONG32 sizeOfSection = 0;
    binaryReader.Read(sizeOfSection);
    ULONG32 endPosition = startingPosition + sizeOfSection;

    LONG32 informationEventInt;
    binaryReader.Read(informationEventInt);
    informationEvent_ = static_cast<InformationEvent::Enum>(informationEventInt);

    ASSERT_IFNOT(
        endPosition >= binaryReader.Position,
        "Invalid information log record, end position: {0} reader position:{1}",
        endPosition, binaryReader.Position);

    binaryReader.Position = endPosition;

    UpdateApproximateDiskSize();
}

void InformationLogRecord::Write(
    __in BinaryWriter & binaryWriter,
    __inout OperationData & operationData,
    __in bool isPhysicalWrite,
    __in bool forceRecomputeOffsets)
{
    __super::Write(binaryWriter, operationData, isPhysicalWrite, forceRecomputeOffsets);

    ULONG32 startingPosition = binaryWriter.Position;
    binaryWriter.Position += sizeof(ULONG32);

    binaryWriter.Write(static_cast<LONG32>(informationEvent_));

    ULONG32 endPosition = binaryWriter.Position;
    ULONG32 sizeOfSection = endPosition - startingPosition;
    binaryWriter.Position = startingPosition;
    binaryWriter.Write(sizeOfSection);
    binaryWriter.Position = endPosition;

    KBuffer::SPtr kBuffer = binaryWriter.GetBuffer(startingPosition);
    operationData.Append(*kBuffer);
}

bool InformationLogRecord::Test_Equals(__in LogRecord const & other) const
{
    InformationLogRecord const & otherInformationLogRecord = dynamic_cast<InformationLogRecord const &>(other);

    if (__super::Test_Equals(other))
    {
        return informationEvent_ == otherInformationLogRecord.informationEvent_;
    }

    return false;
}
