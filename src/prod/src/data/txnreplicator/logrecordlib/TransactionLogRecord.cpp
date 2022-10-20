// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;
using namespace Data::Utilities;

const ULONG TransactionLogRecord::DiskSpaceUsed = sizeof(LONG32) + sizeof(LONG32) + sizeof(LONG64) + sizeof(ULONG64);
const ULONG TransactionLogRecord::SizeOnWireIncrement = sizeof(LONG32) + sizeof(LONG64);

TransactionLogRecord::TransactionLogRecord()
    : LogicalLogRecord()
    , KWeakRefType()
    , transactionBase_(nullptr)
    , childTransactionRecord_(nullptr)
    , parentTransactionRecord_(nullptr)
    , isEnlistedTransaction_(false)
    , parentTransactionRecordOffset_(Constants::InvalidLogicalRecordOffset)
    , replicatedData_()
{
}

TransactionLogRecord::TransactionLogRecord(
    __in LogRecordType::Enum recordType,
    __in ULONG64 recordPosition,
    __in LONG64 lsn,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in TransactionLogRecord & invalidTransactionLog)
    : LogicalLogRecord(recordType, recordPosition, lsn, invalidPhysicalLogRecord)
    , KWeakRefType()
    , transactionBase_(nullptr)
    , childTransactionRecord_(invalidTransactionLog.GetWeakRef())
    , parentTransactionRecord_(&invalidTransactionLog)
    , isEnlistedTransaction_(false)
    , parentTransactionRecordOffset_(Constants::InvalidLogicalRecordOffset)
    , replicatedData_()
{
    ASSERT_IFNOT(
        invalidTransactionLog.childTransactionRecord_ == nullptr && 
        invalidTransactionLog.parentTransactionRecord_ == nullptr,
        "Invalid parent or child xact log record");
}

TransactionLogRecord::TransactionLogRecord(
    __in LogRecordType::Enum recordType,
    __in TransactionBase & transactionBase,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in TransactionLogRecord & invalidTransactionLog,
    __in_opt TransactionLogRecord * parentTransactionRecord)
    : LogicalLogRecord(recordType, invalidPhysicalLogRecord)
    , KWeakRefType()
    , transactionBase_(&transactionBase)
    , childTransactionRecord_(invalidTransactionLog.GetWeakRef())
    , parentTransactionRecord_(parentTransactionRecord)
    , isEnlistedTransaction_(false)
    , parentTransactionRecordOffset_(Constants::InvalidLogicalRecordOffset)
    , replicatedData_()
{
    UpdateApproximateDiskSize();
}

TransactionLogRecord::~TransactionLogRecord()
{
    FreeParentChainProactively();
}

ULONG TransactionLogRecord::CalculateDiskWriteSize(__in_opt OperationData const * const data)
{
    ULONG size = sizeof(LONG32);

    if (data == nullptr)
    {
        return size;
    }

    for (ULONG i = 0; i < data->BufferCount; i++)
    {
        size += sizeof(LONG32); // We write the size of the buffer
        size += (*data)[i]->QuerySize();
    }

    return size;
}

ULONG TransactionLogRecord::CalculateWireSize(__in_opt OperationData const * const data)
{
    ULONG size = sizeof(LONG32);

    if (data == nullptr)
    {
        return size;
    }

    for (ULONG i = 0; i < data->BufferCount; i++)
    {
        size += (*data)[i]->QuerySize();
    }

    return size;
}

void TransactionLogRecord::UpdateApproximateDiskSize()
{
    ApproximateSizeOnDisk = ApproximateSizeOnDisk + DiskSpaceUsed;
}

void TransactionLogRecord::Read(
    __in BinaryReader & binaryReader,
    __in bool isPhysicalRead)
{
    __super::Read(binaryReader, isPhysicalRead);
    ReadPrivate(binaryReader, isPhysicalRead);
}

void TransactionLogRecord::ReadLogical(
    __in OperationData const & operationData,
    __inout INT & index)
{
    __super::ReadLogical(operationData, index);
    
    KBuffer::CSPtr buffer = OperationData::IncrementIndexAndGetBuffer(operationData, index);
    BinaryReader reader(*buffer, GetThisAllocator());
    
    ReadPrivate(reader, false);
}

ULONG TransactionLogRecord::GetSizeOnWire() const
{
    return __super::GetSizeOnWire() + SizeOnWireIncrement;
}

void TransactionLogRecord::Write(
    __in BinaryWriter & binaryWriter,
    __inout OperationData & operationData,
    __in bool isPhysicalWrite,
    __in bool forceRecomputeOffsets)
{
    __super::Write(binaryWriter, operationData, isPhysicalWrite, forceRecomputeOffsets);

    if (replicatedData_ == nullptr)
    {
        ULONG32 startingPosition = binaryWriter.Position;

        // Metadata Size
        binaryWriter.Position += sizeof(ULONG32);
        
        binaryWriter.Write(transactionBase_->TransactionId);

        // End of metadata.
        ULONG32 endPosition = binaryWriter.Position;
        ULONG32 sizeOfSection = endPosition - startingPosition;

        binaryWriter.Position = startingPosition;
        binaryWriter.Write(sizeOfSection);
        binaryWriter.Position = endPosition;

        replicatedData_ = binaryWriter.GetBuffer(startingPosition);
    }

    operationData.Append(*replicatedData_);

    if (isPhysicalWrite)
    {
        // Physical Metadata section
        ULONG32 physicalStartPosition = binaryWriter.Position;
        binaryWriter.Position += sizeof(ULONG32);
        
        if (parentTransactionRecordOffset_ == Constants::InvalidPhysicalRecordOffset || forceRecomputeOffsets == true)
        {
            ASSERT_IFNOT(
                !LogRecord::IsInvalid(parentTransactionRecord_.RawPtr()) || forceRecomputeOffsets == true,
                "Invalid parent xact log record. forceRecomputeOffsets={0}",
                forceRecomputeOffsets);

            if (parentTransactionRecord_ == nullptr)
            {
                parentTransactionRecordOffset_ = 0;
            }
            else
            {
                ASSERT_IFNOT(parentTransactionRecord_->RecordPosition != Constants::InvalidRecordPosition, "Invalid parent xact record position");
                ASSERT_IFNOT(RecordPosition != Constants::InvalidRecordPosition, "Invalid record position");
                
                parentTransactionRecordOffset_ = RecordPosition - parentTransactionRecord_->RecordPosition;
            }
        }

        binaryWriter.Write(parentTransactionRecordOffset_);

        // End Physical Metadata section
        ULONG32 physicalEndPosition = binaryWriter.Position;
        ULONG32 physicalSizeOfSection = physicalEndPosition - physicalStartPosition;
        binaryWriter.Position = physicalStartPosition;
        binaryWriter.Write(physicalSizeOfSection);
        binaryWriter.Position = physicalEndPosition;

        KBuffer::SPtr kBuffer = binaryWriter.GetBuffer(physicalStartPosition);
        operationData.Append(*kBuffer);
    }
}

void TransactionLogRecord::ReadPrivate(
    __in BinaryReader & binaryReader,
    __in bool isPhysicalRead)
{
    // Read Metadata size
    ULONG32 logicalStartingPosition = binaryReader.Position;
    ULONG32 logicalSizeOfSection = 0;
    
    binaryReader.Read(logicalSizeOfSection);
    
    ULONG32 logicalEndPosition = logicalStartingPosition + logicalSizeOfSection;
    LONG64 transactionId;
    
    binaryReader.Read(transactionId);
    transactionBase_ = TransactionBase::CreateTransaction(transactionId, false, GetThisAllocator());

    // Jump to the end of the section ignoring fields that are not understood.
    ASSERT_IFNOT(
        logicalEndPosition >= binaryReader.Position,
        "Could not have read more than section size: {0} {1}", logicalEndPosition, binaryReader.Position);
    
    binaryReader.Position = logicalEndPosition;

    if (isPhysicalRead)
    {
        // Metadata section
        ULONG32 physicalStartPosition = binaryReader.Position;
        ULONG32 physicalSectionSize = 0;
        binaryReader.Read(physicalSectionSize);
        ULONG32 physicalEndPosition = physicalStartPosition + physicalSectionSize;

        binaryReader.Read(parentTransactionRecordOffset_);
        if (parentTransactionRecordOffset_ == 0)
        {
            parentTransactionRecord_ = nullptr;
        }

        ASSERT_IFNOT(
            physicalEndPosition >= binaryReader.Position, 
            "Could not have read more than section size: {0} {1}", physicalEndPosition, binaryReader.Position);

        binaryReader.Position = physicalEndPosition;
    }

    UpdateApproximateDiskSize();
}

std::wstring TransactionLogRecord::ToString() const
{
    std::wstring logRecordString = Constants::CompEndlJSON;

    logRecordString += L"Transaction Id" + Constants::DivisionJSON +
        std::to_wstring(get_TransactionId()) + Constants::Quote;

    return __super::ToString() + logRecordString;
}

void TransactionLogRecord::FreeParentChainProactively()
{
    // To prevent a stack overflow proactively, release the parent references in a loop
    // Using raw pointers to not increment refcount as it is needed for the checks below
    TransactionLogRecord * parent = parentTransactionRecord_.RawPtr();
    TransactionLogRecord * child = parentTransactionRecord_.RawPtr();

    // Step 1- go back all the way to the parent 
    // Stop if parent is INVALID or null
    while (
        parent != nullptr && 
        parent->RecordPosition != Constants::InvalidRecordPosition)
    {
        child = parent;
        parent = parent->ParentTransactionRecord.RawPtr();
    }

    // Step 2 - Free the parent one by one 
    while (parent != parentTransactionRecord_.RawPtr())
    {
        parent = child;
        child = child->ChildTransactionRecord.RawPtr();
        parent->ParentTransactionRecord = nullptr;
    }
}

bool TransactionLogRecord::Test_Equals(__in LogRecord const & other) const
{
    TransactionLogRecord const & otherTxLogRecord = dynamic_cast<TransactionLogRecord const &>(other);

    if (__super::Test_Equals(other))
    {
        return 
            parentTransactionRecordOffset_ == otherTxLogRecord.parentTransactionRecordOffset_ &&
            transactionBase_->TransactionId == otherTxLogRecord.transactionBase_->TransactionId;
    }

    return false;
}
