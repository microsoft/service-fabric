// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        class BackupCopyAsyncOperation
            : public RequestResponseAsyncOperationBase
        {
        public:
            BackupCopyAsyncOperation(
                Transport::MessageUPtr && message,
                Transport::IpcReceiverContextUPtr && receiverContext,
                BackupCopierProxySPtr backupCopierProxy,
                bool isUpload,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            ~BackupCopyAsyncOperation();

            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const & asyncOperation,
                __out Transport::MessageUPtr & reply,
                __out Transport::IpcReceiverContextUPtr & receiverContext);

            __declspec (property(get = get_ReceiverContext)) Transport::IpcReceiverContextUPtr & ReceiverContext;
            Transport::IpcReceiverContextUPtr & get_ReceiverContext() { return receiverContext_; }

            __declspec(property(get = get_ReceivedMessage)) Transport::MessageUPtr const & ReceivedMessage;
            Transport::MessageUPtr const & get_ReceivedMessage() const { return receivedMessage_; }

            __declspec(property(get = get_Reply, put = set_Reply)) Transport::MessageUPtr & Reply;
            Transport::MessageUPtr const & get_Reply() const { return reply_; }
            void set_Reply(Transport::MessageUPtr && value) { reply_ = std::move(value); }

        protected:
            virtual void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override;
            virtual void OnCompleted() override;

        private:
            void OnUploadBackupCompleted(
                AsyncOperationSPtr const & uploadBackupOperation,
                BackupStoreType::Enum storeType,
                bool expectedCompletedSynchronously);

            void OnDownloadBackupCompleted(
                AsyncOperationSPtr const & downloadBackupOperation,
                BackupStoreType::Enum storeType,
                int backupLocationIndex,
                size_t backupLocationCount,
                bool expectedCompletedSynchronously);

        private:
            Transport::IpcReceiverContextUPtr receiverContext_;
            BackupCopierProxySPtr backupCopierProxy_;
            int backupFileOrFolderCount_;
            int completedBackupFileOrFolderCount_;
            Common::ExclusiveLock completedBackupCounterLock_;

            Common::ErrorCode lastSeenError_;

            bool isUpload_;
        };
    }
}
