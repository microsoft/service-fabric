//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

namespace Store
{
    class MigrationSettings : public Serialization::FabricSerializable
    {
    public:
        MigrationSettings()
            : migrationBatchSize_(StoreConfig::GetConfig().MigrationBatchSize)
            , commitTimeout_(StoreConfig::GetConfig().MigrationCommitTimeout)
            , conflictRetryDelay_(StoreConfig::GetConfig().MigrationConflictRetryDelay)
            , backupDirectory_(StoreConfig::GetConfig().MigrationBackupDirectory)
            , backupConnectionString_(StoreConfig::GetConfig().MigrationBackupConnectionString)
            , certStoreLocation_(Common::X509StoreLocation::LocalMachine)
        {
        }

        __declspec(property(get=get_MigrationBatchSize)) int MigrationBatchSize;
        __declspec(property(get=get_CommitTimeout)) Common::TimeSpan const CommitTimeout;
        __declspec(property(get=get_ConflictRetryDelay)) Common::TimeSpan const ConflictRetryDelay;
        __declspec(property(get=get_BackupDirectory)) std::wstring const & BackupDirectory;
        __declspec(property(get=get_BackupConnectionString)) std::wstring const & BackupConnectionString;
        __declspec(property(get=get_CertStoreLocation)) Common::X509StoreLocation::Enum CertStoreLocation;

        int get_MigrationBatchSize() const { return migrationBatchSize_; }
        Common::TimeSpan const get_CommitTimeout() const { return commitTimeout_; }
        Common::TimeSpan const get_ConflictRetryDelay() const { return conflictRetryDelay_; }
        std::wstring const & get_BackupDirectory() const { return backupDirectory_; }
        std::wstring const & get_BackupConnectionString() const { return backupConnectionString_; }
        Common::X509StoreLocation::Enum get_CertStoreLocation() const { return certStoreLocation_; }

        void SetBatchSize(int value) { migrationBatchSize_ = value; }
        void SetBackupDirectory(std::wstring const & value) { backupDirectory_ = value; }
        void SetBackupConnectionString(std::wstring const & value) { backupConnectionString_ = value; }

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
        {
            w << "batchSize=" << migrationBatchSize_;
            w << " commitTimeout=" << commitTimeout_;
            w << " conflictRetryDelay=" << conflictRetryDelay_;
            w << " backupDirectory=" << backupDirectory_;
            w << " backupConnectionString_=" << backupConnectionString_;
            w << " certStoreLocation=" << certStoreLocation_;
        }

        FABRIC_FIELDS_06(
            migrationBatchSize_, 
            commitTimeout_, 
            conflictRetryDelay_,
            backupDirectory_,
            backupConnectionString_,
            certStoreLocation_);

    private:
        int migrationBatchSize_;
        Common::TimeSpan commitTimeout_;
        Common::TimeSpan conflictRetryDelay_;
        std::wstring backupDirectory_;
        std::wstring backupConnectionString_;
        Common::X509StoreLocation::Enum certStoreLocation_;
    };
}
