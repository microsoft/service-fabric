// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LoggingReplicatorTests
{
    class TestBackupCallbackHandler
        : public KObject<TestBackupCallbackHandler>
        , public KShared<TestBackupCallbackHandler>
        , public TxnReplicator::IBackupCallbackHandler
    {
        K_FORCE_SHARED(TestBackupCallbackHandler);
        K_SHARED_INTERFACE_IMP(IBackupCallbackHandler);

    public:
        static TestBackupCallbackHandler::SPtr Create(
            __in KString const & externalBackupFolderPath,
            __in KAllocator & allocator);

    public:
        __declspec(property(get = get_BackupInfoArray)) KArray<TxnReplicator::BackupInfo> BackupInfoArray;
        KArray<TxnReplicator::BackupInfo> get_BackupInfoArray() const;

    public: // IBackupCallbackHandler
        ktl::Awaitable<bool> UploadBackupAsync(
            __in TxnReplicator::BackupInfo backupInfo,
            __in ktl::CancellationToken const & cancellationToken) override;

    private: 
        TestBackupCallbackHandler(
            __in KString const & externalBackupFolderPath);

    private:
        KString::CSPtr externalBackupFolderPath_;
        KArray<TxnReplicator::BackupInfo> backupInfoArray_;
    };
}
