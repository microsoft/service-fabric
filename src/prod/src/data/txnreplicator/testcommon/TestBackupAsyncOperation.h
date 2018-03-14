// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    namespace TestCommon
    {
        class TestBackupAsyncOperation :
            public Common::AsyncOperation,
            public Common::TextTraceComponent<Common::TraceTaskCodes::TestSession>
        {
            DENY_COPY(TestBackupAsyncOperation);

        public:
            TestBackupAsyncOperation(
                __in TxnReplicator::ITransactionalReplicator & transactionalReplicator,
                __in IBackupCallbackHandler & backupCallbackHandler,
                __in FABRIC_BACKUP_OPTION backupOption,
                __in Api::IStorePostBackupHandlerPtr const & postBackupHandler,
                __in Common::AsyncCallback const & callback,
                __in Common::AsyncOperationSPtr const & parent)
                : Common::AsyncOperation(callback, parent, true)
                , txnReplicator_(transactionalReplicator)
                , backupCallbackHandler_(&backupCallbackHandler)
                , backupOption_(backupOption)
                , postBackupHandler_(postBackupHandler)
            {
            }

            virtual ~TestBackupAsyncOperation()
            {
            }

            static Common::ErrorCode End(__in Common::AsyncOperationSPtr const &);

        protected:
            void OnStart(__in Common::AsyncOperationSPtr const & proxySPtr) override;
            void OnCompleted() override;

        private:
            ktl::Task DoBackup(__in Common::AsyncOperationSPtr const & operation) noexcept;

        private:
            TxnReplicator::ITransactionalReplicator & txnReplicator_;
            TxnReplicator::IBackupCallbackHandler::SPtr backupCallbackHandler_;
            FABRIC_BACKUP_OPTION backupOption_;
            Api::IStorePostBackupHandlerPtr postBackupHandler_;
        };
    }
}
