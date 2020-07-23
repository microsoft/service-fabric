// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;
using namespace Data::Utilities;

const ULONG PhysicalLogRecord::DiskSpaceUsed = sizeof(LONG32) + sizeof(ULONG64);

PhysicalLogRecord::PhysicalLogRecord()
    : LogRecord()
    , KWeakRefType()
    , linkedPhysicalRecordOffset_(Constants::InvalidPhysicalRecordOffset)
    , linkedPhysicalRecord_(nullptr)
    , nextPhysicalRecord_(nullptr)
{
}

PhysicalLogRecord::PhysicalLogRecord(
    __in LogRecordType::Enum recordType,
    __in ULONG64 recordPosition,
    __in LONG64 lsn,
    __in PhysicalLogRecord & invalidPhysicalLogRecord)
    : LogRecord(recordType, recordPosition, lsn, invalidPhysicalLogRecord)
    , KWeakRefType()
    , linkedPhysicalRecordOffset_(Constants::InvalidPhysicalRecordOffset)
    , linkedPhysicalRecord_(&invalidPhysicalLogRecord)
    , nextPhysicalRecord_(invalidPhysicalLogRecord.GetWeakRef())
{
    ASSERT_IFNOT(
        invalidPhysicalLogRecord.linkedPhysicalRecordOffset_ == Constants::InvalidPhysicalRecordOffset,
        "Invalid physical record cannot have a valid linked physical record offset");
}

PhysicalLogRecord::PhysicalLogRecord(
    __in LogRecordType::Enum recordType, 
    __in LONG64 lsn, 
    __in_opt PhysicalLogRecord * const linkedPhysicalRecord,
    __in PhysicalLogRecord & invalidPhysicalLogRecord)
    : LogRecord(recordType, invalidPhysicalLogRecord)
    , KWeakRefType()
    , linkedPhysicalRecordOffset_(0)
    , linkedPhysicalRecord_(linkedPhysicalRecord)
    , nextPhysicalRecord_(invalidPhysicalLogRecord.GetWeakRef())
{
    ASSERT_IFNOT(
        linkedPhysicalRecord == nullptr ||
        linkedPhysicalRecord->RecordType == LogRecordType::Enum::EndCheckpoint ||
        linkedPhysicalRecord->RecordType == LogRecordType::Enum::CompleteCheckpoint ||
        linkedPhysicalRecord->RecordType == LogRecordType::Enum::TruncateHead,
        "Invalid linked physical record. Acceptable values are EndCheckpointLogRecord, CompleteCheckpointLogRecord, TruncateHeadLogRecord or nullptr");

    Lsn = lsn;

    if (linkedPhysicalRecord_ != nullptr)
    {
        linkedPhysicalRecordOffset_ = Constants::InvalidPhysicalRecordOffset;
    }

    ASSERT_IFNOT(
        invalidPhysicalLogRecord.linkedPhysicalRecordOffset_ == Constants::InvalidPhysicalRecordOffset,
        "Invalid physical record cannot have a valid linked physical record offset");

    UpdateApproximateDiskSize();
}

PhysicalLogRecord::~PhysicalLogRecord()
{
    FreePhysicalLinkChainProactively();
}

void PhysicalLogRecord::UpdateApproximateDiskSize()
{
    ApproximateSizeOnDisk = ApproximateSizeOnDisk + DiskSpaceUsed;
}

void PhysicalLogRecord::Read(
    __in BinaryReader & binaryReader, 
    __in bool isPhysicalRead)
{
    __super::Read(binaryReader, isPhysicalRead);
    ULONG32 startingPosition = binaryReader.Position;
    ULONG32 sizeOfSection;
    binaryReader.Read(sizeOfSection);

    ULONG32 endPosition = startingPosition + sizeOfSection;
    binaryReader.Read(linkedPhysicalRecordOffset_);

    // Jump to the end of the section ignoring fields that are not understood.
    ASSERT_IFNOT(endPosition >= binaryReader.Position,
        "Endposition: {0} should be greater than binary reader position: {1} reading physical log record",
        endPosition, binaryReader.Position);

    binaryReader.Position = endPosition;

    UpdateApproximateDiskSize();
}

void PhysicalLogRecord::Write(
    __in BinaryWriter & binaryWriter, 
    __in OperationData & operationData,
    __in bool isPhysicalWrite,
    __in bool forceRecomputeOffsets)
{
    __super::Write(binaryWriter, operationData, isPhysicalWrite, forceRecomputeOffsets);
    ULONG32 startingPosition = binaryWriter.Position;
    binaryWriter.Position += sizeof(ULONG32);

    if(linkedPhysicalRecordOffset_ == Constants::InvalidPhysicalRecordOffset || forceRecomputeOffsets == true)
    {
        ASSERT_IFNOT(
            linkedPhysicalRecord_ != nullptr || forceRecomputeOffsets == true,
            "Linked physical record must not be null. (linkedPhysicalRecord_ != nullptr)={0}. forceRecomputeOffsets={1}",
            linkedPhysicalRecord_ != nullptr,
            forceRecomputeOffsets);

        if (linkedPhysicalRecord_ == nullptr)
        {
            linkedPhysicalRecordOffset_ = 0;
        }
        else
        {
            ASSERT_IFNOT(
                linkedPhysicalRecord_->RecordPosition != Constants::InvalidRecordPosition,
                "Linked physical record position must be valid");

            ASSERT_IFNOT(
                RecordPosition != Constants::InvalidRecordPosition,
                "Record position must be valid");

            linkedPhysicalRecordOffset_ = RecordPosition - linkedPhysicalRecord_->RecordPosition;
        }
    }

    binaryWriter.Write(linkedPhysicalRecordOffset_);
    ULONG32 endPosition = binaryWriter.Position;
    ULONG32 sizeOfSection = endPosition - startingPosition;

    binaryWriter.Position = startingPosition;
    binaryWriter.Write(sizeOfSection);
    binaryWriter.Position = endPosition;

    KBuffer::SPtr kBuffer = binaryWriter.GetBuffer(startingPosition);
    operationData.Append(*kBuffer);
}

std::wstring PhysicalLogRecord::ToString() const
{
    return __super::ToString();
}

bool PhysicalLogRecord::FreePreviousLinksLowerThanPsn(
    __in LONG64 logHeadPsn,
    __in InvalidLogRecords & invalidLogRecords)
{
    bool ret = __super::FreePreviousLinksLowerThanPsn(
        logHeadPsn, 
        invalidLogRecords);

    if (linkedPhysicalRecord_ != nullptr &&
        linkedPhysicalRecord_->Psn < logHeadPsn)
    {
        ASSERT_IF(
            linkedPhysicalRecordOffset_ == Constants::InvalidPhysicalRecordOffset,
            "FreePreviousLinksLowerThanPsn: Invalid linked physical record offset: {0}",
            linkedPhysicalRecordOffset_);

        linkedPhysicalRecord_ = linkedPhysicalRecordOffset_ == 0 ?
            nullptr :
            invalidLogRecords.Inv_PhysicalLogRecord;

        return true;
    }

    return ret;
}

void PhysicalLogRecord::FreePhysicalLinkChainProactively()
{
    // To prevent a stack overflow, release the back references in a loop proactively
    // Using raw pointers to not increment refcount as it is needed for the checks below
    PhysicalLogRecord * parent = previousPhysicalRecord_.RawPtr();
    PhysicalLogRecord * child = previousPhysicalRecord_.RawPtr();

    // Step 1- go back all the way to the first physical record 
    // Stop if parent is INVALID or null
    while (
        parent != nullptr && 
        parent->RecordPosition != Constants::InvalidRecordPosition)
    {
        child = parent;
        parent = parent->PreviousPhysicalRecord.RawPtr();
    }

    // Step 2 - Free the parent one by one 
    while (parent != previousPhysicalRecord_.RawPtr())
    {
        parent = child;
        child = child->NextPhysicalRecord.RawPtr();
        parent->previousPhysicalRecord_ = nullptr;
    }
}

bool PhysicalLogRecord::Test_Equals(__in LogRecord const & other) const
{
    PhysicalLogRecord const & otherPhysicalRecord = dynamic_cast<PhysicalLogRecord const &>(other);

    if (__super::Test_Equals(other))
    {
        return linkedPhysicalRecordOffset_ == otherPhysicalRecord.linkedPhysicalRecordOffset_;
    }

    return false;
}
