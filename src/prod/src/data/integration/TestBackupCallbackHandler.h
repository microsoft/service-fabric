// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Integration
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

        public: // IBackupCallbackHandler
            ktl::Awaitable<bool> UploadBackupAsync(
                __in TxnReplicator::BackupInfo backupInfo,
                __in ktl::CancellationToken const & cancellationToken) override;

        public:
            void SetBackupCallbackACS(
                __in ktl::AwaitableCompletionSource<void> & acs) noexcept;

        private:
            TestBackupCallbackHandler(
                __in KString const & externalBackupFolderPath);

        private:
            KString::CSPtr externalBackupFolderPath_;
            KArray<TxnReplicator::BackupInfo> backupInfoArray_;
            ktl::AwaitableCompletionSource<void>::SPtr backupCallbackACSSPtr_;
        };
    }
}
