// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class EseReplicatedStore
        : public ReplicatedStore
    {
    private:
        // Legacy suffixes < 4.0 CU5.
        // Suffixes were shortened as a hotfix for long RNM WF root paths.
        //
        static Common::GlobalWString LegacyFullCopyDirectorySuffix;
        static Common::GlobalWString LegacyPartialCopyDirectorySuffix;
        static Common::GlobalWString LegacyDropBackupDirectorySuffix;
        
        static Common::GlobalWString FullCopyDirectorySuffix;
        static Common::GlobalWString PartialCopyDirectorySuffix;
        static Common::GlobalWString DropBackupDirectorySuffix;

        static Common::GlobalWString PartialCopyCompletionMarkerFilename;

    private:

        EseReplicatedStore(
            Common::Guid const & partitionId,
            ::FABRIC_REPLICA_ID replicaId,
            Reliability::ReplicationComponent::ReplicatorSettingsUPtr &&,
            Store::ReplicatedStoreSettings const &,
            EseLocalStoreSettings const & ,
            Api::IStoreEventHandlerPtr const &,
            Api::ISecondaryEventHandlerPtr const &,
            Common::ComponentRoot const& root);

        EseReplicatedStore(
            Common::Guid const & partitionId,
            ::FABRIC_REPLICA_ID replicaId,
            Common::ComPointer<IFabricReplicator> &&,
            Store::ReplicatedStoreSettings const &,
            EseLocalStoreSettings const & ,
            Api::IStoreEventHandlerPtr const &,
            Api::ISecondaryEventHandlerPtr const &,
            Common::ComponentRoot const& root);
    public:

        static std::unique_ptr<EseReplicatedStore> Create(
            Common::Guid const & partitionId,
            ::FABRIC_REPLICA_ID replicaId,
            Reliability::ReplicationComponent::ReplicatorSettingsUPtr &&,
            Store::ReplicatedStoreSettings const &,
            EseLocalStoreSettings const & ,
            Api::IStoreEventHandlerPtr const &,
            Api::ISecondaryEventHandlerPtr const &,
            Common::ComponentRoot const&);
                
        static std::unique_ptr<EseReplicatedStore> Create(
            Common::Guid const & partitionId,
            ::FABRIC_REPLICA_ID replicaId,
            Common::ComPointer<IFabricReplicator> &&,
            Store::ReplicatedStoreSettings const &,
            EseLocalStoreSettings const & ,
            Api::IStoreEventHandlerPtr const &,
            Api::ISecondaryEventHandlerPtr const &,
            Common::ComponentRoot const&);

        __declspec(property(get=get_Directory)) std::wstring const & Directory;
        std::wstring const & get_Directory() const { return directory_; }

        __declspec(property(get=get_FileName)) std::wstring const & FileName;
        std::wstring const & get_FileName() const { return settings_.StoreName; }

        virtual ILocalStoreUPtr const & get_LocalStore() const override;

        virtual ~EseReplicatedStore();

        static std::wstring CreateLocalStoreRootDirectory(std::wstring const & path, Common::Guid const & partitionId);
        static std::wstring CreateLocalStoreDatabaseDirectory(std::wstring const & root, FABRIC_REPLICA_ID);

        std::wstring GetLocalStoreRootDirectory() override;
        std::wstring GetFileStreamFullCopyRootDirectory() override;

        virtual void CreateLocalStore() override;
        virtual void ReleaseLocalStore() override;

        virtual ILocalStoreUPtr CreateTemporaryLocalStore() override;
        virtual ILocalStoreUPtr CreateFullCopyLocalStore() override;
        virtual ILocalStoreUPtr CreatePartialCopyLocalStore() override;
        virtual ILocalStoreUPtr CreateRebuildLocalStore() override;

        virtual Common::ErrorCode CreateLocalStoreFromFullCopyData() override;
        virtual Common::ErrorCode MarkPartialCopyLocalStoreComplete() override;
        virtual Common::ErrorCode TryRecoverPartialCopyLocalStore(__out ILocalStoreUPtr &) override;

        virtual Common::ErrorCode DeleteFullCopyDatabases() override;
        virtual Common::ErrorCode DeletePartialCopyDatabases() override;
        virtual Common::ErrorCode BackupAndDeleteDatabase() override;
        virtual void BestEffortDeleteAllCopyDatabases() override;

        virtual Common::ErrorCode PreCopyNotification(std::vector<std::wstring> const &) override;
        virtual Common::ErrorCode PostCopyNotification() override;

        virtual Common::ErrorCode CheckPreOpenLocalStoreHealth(__out bool & reportedHealth) override;
        virtual Common::ErrorCode CheckPostOpenLocalStoreHealth(bool cleanupHealth) override;

        virtual Common::ErrorCode CreateEnumerationByPrimaryIndex(
            ILocalStoreUPtr const &, 
            std::wstring const & typeStart,
            std::wstring const & keyStart,
            __out TransactionSPtr &,
            __out EnumerationSPtr &) override;

        virtual bool ShouldRestartOnWriteError(Common::ErrorCode const &) override;

    private:
        void InitializeDirectoryPath();

        ILocalStoreUPtr CreateLocalStore(
            std::wstring const &,
            bool isMainStore);

        std::wstring GetLegacyFullCopyDirectoryName() const;
        std::wstring GetLegacyPartialCopyDirectoryName() const;
        std::wstring GetLegacyDropBackupDirectoryName() const;

        std::wstring GetFullCopyDirectoryName() const;
        std::wstring GetPartialCopyDirectoryName() const;
        std::wstring GetDropBackupDirectoryName() const;

        std::wstring GetPartialCopyCompletionMarkerFilename() const;

        void BestEffortDeleteDirectory(std::wstring const & directory);
        Common::ErrorCode DeleteDirectory(std::wstring const & directory);
        
        EseLocalStoreSettings settings_;
        std::wstring directoryRoot_;
        std::wstring directory_;

        std::unique_ptr<ILocalStore> localStoreUPtr_;
    };
}
