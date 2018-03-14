// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::LoggingReplicator;
using namespace Data::Utilities;

NTSTATUS BackupLogFileProperties::Create(
    __in KAllocator & allocator,
    __out SPtr & result) noexcept
{
    result = _new(BACKUP_LOG_FILE_PROPERTIES_TAG, allocator) BackupLogFileProperties();
    if (result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(result->Status()))
    {
        return (SPtr(Ktl::Move(result)))->Status();
    }

    return STATUS_SUCCESS;
}

ULONG32 BackupLogFileProperties::get_Count() const { return count_; }
void BackupLogFileProperties::put_Count(__in ULONG32 blocksHandle) { count_ = blocksHandle; }

TxnReplicator::Epoch BackupLogFileProperties::get_IndexingRecordEpoch() const { return indexingRecordEpoch_; }
void BackupLogFileProperties::put_IndexingRecordEpoch(__in TxnReplicator::Epoch epoch) { indexingRecordEpoch_ = epoch; }

FABRIC_SEQUENCE_NUMBER BackupLogFileProperties::get_IndexingRecordLSN() const { return indexingRecordLSN_; }
void BackupLogFileProperties::put_IndexingRecordLSN(__in FABRIC_SEQUENCE_NUMBER LSN) { indexingRecordLSN_ = LSN; }

TxnReplicator::Epoch BackupLogFileProperties::get_LastBackedUpEpoch() const { return lastBackedUpEpoch_; }
void BackupLogFileProperties::put_LastBackedUpEpoch(__in TxnReplicator::Epoch epoch) { lastBackedUpEpoch_ = epoch; }

FABRIC_SEQUENCE_NUMBER BackupLogFileProperties::get_LastBackedUpLSN() const { return lastBackedUpLSN_; }
void BackupLogFileProperties::put_LastBackedUpLSN(__in FABRIC_SEQUENCE_NUMBER LSN) { lastBackedUpLSN_ = LSN; }

BlockHandle::SPtr BackupLogFileProperties::get_RecordsHandle() const { return recordsHandle_.Get(); }
void BackupLogFileProperties::put_RecordsHandle(__in BlockHandle & blocksHandle) { recordsHandle_.Put(&blocksHandle); }

void BackupLogFileProperties::Write(__in BinaryWriter& writer)
{
    // Allow the base class to write first.    
    FileProperties::Write(writer);

    // 'records' - BlockHandle
    writer.Write(RecordsHandlePropertyName);
    VarInt::Write(writer, BlockHandle::SerializedSize());
    RecordsHandle->Write(writer);

    // 'count' - ULONG32
    writer.Write(CountPropertyName);
    VarInt::Write(writer, static_cast<ULONG32>(sizeof(ULONG32)));
    writer.Write(count_);

    // 'indexepoch' - Epoch
    writer.Write(IndexingRecordEpochPropertyName);
    VarInt::Write(writer, static_cast<ULONG32>(sizeof(LONG64) * 2));
    writer.Write(indexingRecordEpoch_.DataLossVersion);
    writer.Write(indexingRecordEpoch_.ConfigurationVersion);

    // 'indexlsn' - FABRIC_SEQUENCE_NUMBER
    writer.Write(IndexingRecordLsnPropertyName);
    VarInt::Write(writer, static_cast<ULONG32>(sizeof(FABRIC_SEQUENCE_NUMBER)));
    writer.Write(indexingRecordLSN_);

    // 'backupepoch' - Epoch
    writer.Write(LastBackedUpEpochPropertyName);
    VarInt::Write(writer, static_cast<ULONG32>(sizeof(LONG64) * 2));
    writer.Write(lastBackedUpEpoch_.DataLossVersion);
    writer.Write(lastBackedUpEpoch_.ConfigurationVersion);

    // 'backuplsn' - FABRIC_SEQUENCE_NUMBER
    writer.Write(LastBackedUpLsnPropertyName);
    VarInt::Write(writer, static_cast<ULONG32>(sizeof(FABRIC_SEQUENCE_NUMBER)));
    writer.Write(lastBackedUpLSN_);
}

void BackupLogFileProperties::ReadProperty(
    __in BinaryReader& reader, 
    __in KStringView const& property, 
    __in ULONG32 valueSize)
{
    // Properties are key-value pairs, so read in corresponding properties
    if (property.Compare(CountPropertyName) == 0)
    {
        reader.Read(count_);
    }
    else if (property.Compare(IndexingRecordEpochPropertyName) == 0)
    {
        LONG64 dataLossNumber = FABRIC_INVALID_SEQUENCE_NUMBER;
        LONG64 configurationNumber = FABRIC_INVALID_SEQUENCE_NUMBER;
        reader.Read(dataLossNumber);
        reader.Read(configurationNumber);
        indexingRecordEpoch_ = TxnReplicator::Epoch(dataLossNumber, configurationNumber);
    }
    else if (property.Compare(IndexingRecordLsnPropertyName) == 0)
    {
        reader.Read(indexingRecordLSN_);
    }
    else if (property.Compare(LastBackedUpEpochPropertyName) == 0)
    {
        LONG64 dataLossNumber = FABRIC_INVALID_SEQUENCE_NUMBER;
        LONG64 configurationNumber = FABRIC_INVALID_SEQUENCE_NUMBER;
        reader.Read(dataLossNumber);
        reader.Read(configurationNumber);
        lastBackedUpEpoch_ = TxnReplicator::Epoch(dataLossNumber, configurationNumber);
    }
    else if (property.Compare(LastBackedUpLsnPropertyName) == 0)
    {
        reader.Read(lastBackedUpLSN_);
    }
    else if (property.Compare(RecordsHandlePropertyName) == 0)
    {
        recordsHandle_.Put(BlockHandle::Read(reader, GetThisAllocator()));
    }
    else
    {
        // If the properties is unknown, just skip it.
        FileProperties::ReadProperty(reader, property, valueSize);
    }
}

BackupLogFileProperties::BackupLogFileProperties()
    : recordsHandle_(nullptr)
{
}

BackupLogFileProperties::~BackupLogFileProperties()
{
}
