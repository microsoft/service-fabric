// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::LoggingReplicator;
using namespace Data::Utilities;

NTSTATUS BackupMetadataFileProperties::Create(
    __in KAllocator & allocator,
    __out SPtr & result) noexcept
{
    // Cal KGuid(__in BOOLEAN Zero = TRUE) to constructor the empty KGuid
    KGuid emptyKGuid(TRUE);
    result = _new(BACKUP_METADATA_FILE_PROPERTIES_TAG, allocator) BackupMetadataFileProperties(
        FABRIC_BACKUP_OPTION_INVALID,
        emptyKGuid,
        emptyKGuid,
        emptyKGuid,
        -1,
        TxnReplicator::Epoch::InvalidEpoch(),
        FABRIC_INVALID_SEQUENCE_NUMBER,
        TxnReplicator::Epoch::InvalidEpoch(),
        FABRIC_INVALID_SEQUENCE_NUMBER);
    
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

NTSTATUS BackupMetadataFileProperties::Create(
    __in FABRIC_BACKUP_OPTION backupOption,
    __in KGuid const & parentBackupId,
    __in KGuid const & backupId,
    __in KGuid const & partitionId,
    __in FABRIC_REPLICA_ID replicaId,
    __in TxnReplicator::Epoch const & startingEpoch,
    __in FABRIC_SEQUENCE_NUMBER startingLSN,
    __in TxnReplicator::Epoch const & backupEpoch,
    __in FABRIC_SEQUENCE_NUMBER backupLSN,
    __in KAllocator & allocator,
    __out SPtr & result) noexcept
{
    result = _new(BACKUP_METADATA_FILE_PROPERTIES_TAG, allocator) BackupMetadataFileProperties(
        backupOption,
        parentBackupId,
        backupId,
        partitionId,
        replicaId,
        startingEpoch,
        startingLSN,
        backupEpoch,
        backupLSN);

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

// Write the properties to the buffer
void BackupMetadataFileProperties::Write(__in BinaryWriter& writer)
{
    // Allow the base class to write first.    
    FileProperties::Write(writer);

    // 'option' - LONG32
    writer.Write(BackupOptionPropertyName);
    VarInt::Write(writer, static_cast<LONG32>(sizeof(LONG32)));
    writer.Write(static_cast<LONG32>(backupOption_) - backupOptionEnumOffset);

    // 'parentbackupid' - KGuid
    writer.Write(ParentBackupIdPropertyName);
    // Guid 16 bytes
    ULONG32 length = 16;
    VarInt::Write(writer, length);
    writer.Write(parentBackupId_);

    // 'backupid' - KGuid
    writer.Write(BackupIdPropertyName);
    VarInt::Write(writer, length);
    writer.Write(backupId_);

    // 'partitionid' - KGuid
    writer.Write(PartitionIdPropertyName);
    VarInt::Write(writer, length);
    writer.Write(partitionId_);

    // 'replicaid' - LONG64
    writer.Write(ReplicaIdPropertyName);
    VarInt::Write(writer, static_cast<ULONG32>(sizeof(LONG64)));
    writer.Write(replicaId_);

    // 'epoch' - Epoch
    writer.Write(StartingEpochPropertyName);
    VarInt::Write(writer, static_cast<ULONG32>(sizeof(LONG64) * 2));
    writer.Write(startingEpoch_.DataLossVersion);
    writer.Write(startingEpoch_.ConfigurationVersion);

    // 'lsn' - FABRIC_SEQUENCE_NUMBER
    writer.Write(StartLSNPropertyName);
    VarInt::Write(writer, static_cast<ULONG32>(sizeof(FABRIC_SEQUENCE_NUMBER)));
    writer.Write(startingLSN_);

    // 'epoch' - Epoch
    writer.Write(BackupEpochPropertyName);
    VarInt::Write(writer, static_cast<ULONG32>(sizeof(LONG64) * 2));
    writer.Write(backupEpoch_.DataLossVersion);
    writer.Write(backupEpoch_.ConfigurationVersion);

    // 'lsn' - FABRIC_SEQUENCE_NUMBER
    writer.Write(BackupLSNPropertyName);
    VarInt::Write(writer, static_cast<ULONG32>(sizeof(FABRIC_SEQUENCE_NUMBER)));
    writer.Write(backupLSN_);
}

// Read the properties from the buffer
void BackupMetadataFileProperties::ReadProperty(
    __in BinaryReader & reader, 
    __in KStringView const & propertyName,
    __in ULONG32 valueSize)
{
    // Properties are key-value pairs, so read in corresponding properties
    if (propertyName.Compare(BackupOptionPropertyName) == 0)
    {
        LONG32 backupOption;
        reader.Read(backupOption);

        backupOption_ = static_cast<FABRIC_BACKUP_OPTION>(backupOption + backupOptionEnumOffset);
    }
    else if (propertyName.Compare(ParentBackupIdPropertyName) == 0)
    {
        reader.Read(parentBackupId_);
    }
    else if (propertyName.Compare(BackupIdPropertyName) == 0)
    {
        reader.Read(backupId_);
    }
    else if (propertyName.Compare(PartitionIdPropertyName) == 0)
    {
        reader.Read(partitionId_);
    }
    else if (propertyName.Compare(ReplicaIdPropertyName) == 0)
    {
        reader.Read(replicaId_);
    }
    else if (propertyName.Compare(StartingEpochPropertyName) == 0)
    {
        LONG64 dataLossNumber = FABRIC_INVALID_SEQUENCE_NUMBER;
        LONG64 configurationNumber = FABRIC_INVALID_SEQUENCE_NUMBER;
        reader.Read(dataLossNumber);
        reader.Read(configurationNumber);
        startingEpoch_ = TxnReplicator::Epoch(dataLossNumber, configurationNumber);
    }
    else if (propertyName.Compare(StartLSNPropertyName) == 0)
    {
        reader.Read(startingLSN_);
    }
    else if (propertyName.Compare(BackupEpochPropertyName) == 0)
    {
        LONG64 dataLossNumber = FABRIC_INVALID_SEQUENCE_NUMBER;
        LONG64 configurationNumber = FABRIC_INVALID_SEQUENCE_NUMBER;
        reader.Read(dataLossNumber);
        reader.Read(configurationNumber);
        backupEpoch_ = TxnReplicator::Epoch(dataLossNumber, configurationNumber);
    }
    else if (propertyName.Compare(BackupLSNPropertyName) == 0)
    {
        reader.Read(backupLSN_);
    }
    else
    {
        // If the properties is unknown, just skip it.
        FileProperties::ReadProperty(reader, propertyName, valueSize);
    }
}

TxnReplicator::Epoch BackupMetadataFileProperties::get_BackupEpoch() const { return backupEpoch_; }
void BackupMetadataFileProperties::put_BackupEpoch(__in TxnReplicator::Epoch const & epoch) { backupEpoch_ = epoch; }

KGuid BackupMetadataFileProperties::get_BackupId() const { return backupId_; }
void BackupMetadataFileProperties::put_BackupId(__in KGuid const & backupId) { backupId_ = backupId; }

FABRIC_SEQUENCE_NUMBER BackupMetadataFileProperties::get_BackupLSN() const { return backupLSN_; }
void BackupMetadataFileProperties::put_BackupLSN(__in FABRIC_SEQUENCE_NUMBER backupLSN) { backupLSN_ = backupLSN; }

FABRIC_BACKUP_OPTION BackupMetadataFileProperties::get_BackupOption() const { return backupOption_; }
void BackupMetadataFileProperties::put_BackupOption(__in FABRIC_BACKUP_OPTION backupOption) { backupOption_ = backupOption; }

KGuid BackupMetadataFileProperties::get_ParentBackupId() const { return parentBackupId_; }
void BackupMetadataFileProperties::put_ParentBackupId(__in KGuid const & parentBackupId) { parentBackupId_ = parentBackupId; }

KGuid BackupMetadataFileProperties::get_PartitionId() const { return partitionId_; }
void BackupMetadataFileProperties::put_PartitionId(__in KGuid const & partitionId) { partitionId_ = partitionId; }

FABRIC_REPLICA_ID BackupMetadataFileProperties::get_ReplicaId() const { return replicaId_; }
void BackupMetadataFileProperties::put_ReplicaId(__in FABRIC_REPLICA_ID replicaId) { replicaId_ = replicaId; }

TxnReplicator::Epoch BackupMetadataFileProperties::get_StartingEpoch() const { return startingEpoch_; }
void BackupMetadataFileProperties::put_StartingEpoch(__in TxnReplicator::Epoch const & epoch) { startingEpoch_ = epoch; }

FABRIC_SEQUENCE_NUMBER BackupMetadataFileProperties::get_StartingLSN() const { return startingLSN_; }
void BackupMetadataFileProperties::put_StartingLSN(__in FABRIC_SEQUENCE_NUMBER startingLSN) { startingLSN_ = startingLSN; }

BackupMetadataFileProperties::BackupMetadataFileProperties(
    __in FABRIC_BACKUP_OPTION backupOption,
    __in KGuid const & parentBackupId,
    __in KGuid const & backupId,
    __in KGuid const & partitionId,
    __in FABRIC_REPLICA_ID replicaId,
    __in TxnReplicator::Epoch const & startingEpoch,
    __in FABRIC_SEQUENCE_NUMBER startingLSN,
    __in TxnReplicator::Epoch const & backupEpoch,
    __in FABRIC_SEQUENCE_NUMBER backupLSN) noexcept
    : backupOption_(backupOption)
    , parentBackupId_(parentBackupId)
    , backupId_(backupId)
    , partitionId_(partitionId)
    , replicaId_(replicaId)
    , startingEpoch_(startingEpoch)
    , startingLSN_(startingLSN)
    , backupEpoch_(backupEpoch)
    , backupLSN_(backupLSN)
{
}

BackupMetadataFileProperties::~BackupMetadataFileProperties()
{
}
