// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    namespace StoreBackupType
    {
        enum Enum : ULONG
        {
            None = 0,
            Full = 1,
            Incremental = 2
        };

        void WriteToTextWriter(__in Common::TextWriter & w, Enum const & e);
    };

    class BackupRestoreData : public Serialization::FabricSerializable
    {
    public:
        static Common::GlobalWString FileName;
        
        BackupRestoreData()
            : partitionInfo_()
            , backupType_(StoreBackupType::None)
            , backupChainGuid_(Common::Guid::Empty())
            , backupIndex_(0)
			, backupFiles_()
            , replicaId_(0)
            , timeStampUtc_(Common::DateTime::Now())
        {
        }
        
        explicit BackupRestoreData(
            ServiceModel::ServicePartitionInformation const & partitionInfo,
            ::FABRIC_REPLICA_ID replicaId,
            StoreBackupType::Enum backupType,
            Common::Guid backupChainGuid,
            ULONG backupIndex,
			std::unordered_set<std::wstring> && backupFiles)
            : partitionInfo_(partitionInfo)
            , backupType_(backupType)
            , backupChainGuid_(backupChainGuid)
            , backupIndex_(backupIndex)
			, backupFiles_(std::move(backupFiles))
            , replicaId_(replicaId)
            , timeStampUtc_(Common::DateTime::Now())
        {
        }
        
        __declspec(property(get=get_PartitionInfo)) ServiceModel::ServicePartitionInformation const & PartitionInfo;
        ServiceModel::ServicePartitionInformation const & get_PartitionInfo() const { return partitionInfo_; }

        __declspec(property(get = get_BackupType)) StoreBackupType::Enum const & BackupType;
        StoreBackupType::Enum const & get_BackupType() const { return backupType_; }

        __declspec(property(get = get_BackupIndex)) ULONG const & BackupIndex;
        ULONG const & get_BackupIndex() const { return backupIndex_; }

        __declspec(property(get = get_BackupChainGuid)) Common::Guid const & BackupChainGuid;
        Common::Guid const & get_BackupChainGuid() const { return backupChainGuid_; }

        __declspec(property(get = get_ReplicaId)) ::FABRIC_REPLICA_ID const & ReplicaId;
        ::FABRIC_REPLICA_ID const & get_ReplicaId() const { return replicaId_; }

        __declspec(property(get = get_TimeStampUtc)) Common::DateTime const & TimeStampUtc;
        Common::DateTime const & get_TimeStampUtc() const { return timeStampUtc_; }

		__declspec(property(get = get_BackupFiles)) std::unordered_set<std::wstring> const & BackupFiles;
		std::unordered_set<std::wstring> const & get_BackupFiles() const { return backupFiles_; }
        
        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

		FABRIC_FIELDS_07(partitionInfo_, backupType_, backupIndex_, backupChainGuid_, replicaId_, timeStampUtc_, backupFiles_);

    private:
        ServiceModel::ServicePartitionInformation partitionInfo_;
        StoreBackupType::Enum backupType_;
        Common::Guid backupChainGuid_;
        ULONG backupIndex_;
		std::unordered_set<std::wstring> backupFiles_;

        // For debugging purposes
        ::FABRIC_REPLICA_ID replicaId_;
        Common::DateTime timeStampUtc_;
    };
}

