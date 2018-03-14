// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    namespace TestCommon
    {
        // Test DataLossHandler used for construct TransactionalReplicator
        class TestDataLossHandler :
            public KObject<TestDataLossHandler>,
            public KShared<TestDataLossHandler>,
            public TxnReplicator::IDataLossHandler
        {
            K_FORCE_SHARED(TestDataLossHandler)
            K_SHARED_INTERFACE_IMP(IDataLossHandler)

        public:
            static SPtr Create(
                __in KAllocator & allocator,
                __in_opt KString const * const backupFolder = nullptr,
                __in_opt FABRIC_RESTORE_POLICY restorePolicy = FABRIC_RESTORE_POLICY_SAFE);

        public: // Test APIs
            NTSTATUS Initialize(
                __in TxnReplicator::ITransactionalReplicator & replicator) noexcept;

            void SetBackupPath(
                __in_opt KString const * const replicator) noexcept;

        public:
            ktl::Awaitable<bool> DataLossAsync(
                __in ktl::CancellationToken const & cancellationToken) override;

        private:
            ITransactionalReplicator::SPtr GetTransactionalReplicator() noexcept;
           
            bool IsValidFullBackupExist() const noexcept;

            // Constructor
            TestDataLossHandler(
                __in_opt KString const * const backupFolder,
                __in_opt FABRIC_RESTORE_POLICY restorePolicy);

        private:
            KStringView const FullMetadataFileName = L"backup.metadata";

        private:
            KString::CSPtr backupFolderPath_;
            FABRIC_RESTORE_POLICY restorePolicy_;
            Common::TimeSpan timeout_;

            KWeakRef<TxnReplicator::ITransactionalReplicator>::SPtr transactionalReplicatorWRef_;
        };
    }
}
