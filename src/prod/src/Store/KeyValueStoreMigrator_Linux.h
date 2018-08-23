//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

namespace Store
{
    class KeyValueStoreMigrator 
    {
    public:

        static Common::ErrorCode CreateForTStoreMigration(
            Common::ComponentRoot const &,
            Store::PartitionedReplicaId const &,
            __in IReplicatedStore &, 
            TSReplicatedStoreSettingsUPtr &&, 
            std::unique_ptr<MigrationInitData> &&,
            __out std::shared_ptr<KeyValueStoreMigrator> &)
        {
            return Common::ErrorCodeValue::NotImplemented;
        }

        bool TryUpdateInitializationData(std::unique_ptr<MigrationInitData> &&) { return false; }

        Common::AsyncOperationSPtr BeginMigration(Common::AsyncCallback const & callback, Common::AsyncOperationSPtr const & parent)
        {
            return Common::AsyncOperation::CreateAndStart<CompletedAsyncOperation>(Common::ErrorCodeValue::NotImplemented, callback, parent);
        }

        Common::ErrorCode EndMigration(Common::AsyncOperationSPtr const & operation)
        {
            return Common::CompletedAsyncOperation::End(operation);
        }

        MigrationState::Enum GetMigrationState() const { return MigrationState::Inactive; }

        MigrationPhase::Enum GetMigrationPhase() const { return MigrationPhase::Inactive; }

        void Cancel() { };
    };

    typedef std::shared_ptr<KeyValueStoreMigrator> KeyValueStoreMigratorSPtr;
}
