// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    namespace TestCommon
    {
        class TestRestoreAsyncOperation :
            public Common::AsyncOperation,
            public Common::TextTraceComponent<Common::TraceTaskCodes::TestSession>
        {
            DENY_COPY(TestRestoreAsyncOperation);

        public:
            TestRestoreAsyncOperation(
                __in TxnReplicator::ITransactionalReplicator & transactionalReplicator,
                __in KString const & backupFolderPath,
                __in FABRIC_RESTORE_POLICY restorePolicy,
                __in Common::AsyncCallback const & callback,
                __in Common::AsyncOperationSPtr const & parent)
                : Common::AsyncOperation(callback, parent, true)
                , txnReplicator_(transactionalReplicator)
                , backupFolderPath_(&backupFolderPath)
                , restorePolicy_(restorePolicy)
            {
            }

            virtual ~TestRestoreAsyncOperation()
            {
            }

            static Common::ErrorCode End(
                __in Common::AsyncOperationSPtr const &);

        protected:
            void OnStart(
                __in Common::AsyncOperationSPtr const & proxySPtr) override;

        private:
            void DoRestore(__in
                Common::AsyncOperationSPtr const & operation);

            TxnReplicator::ITransactionalReplicator & txnReplicator_;
            KString::CSPtr backupFolderPath_;
            FABRIC_RESTORE_POLICY restorePolicy_;
        };
    }
}
