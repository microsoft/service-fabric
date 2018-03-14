// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        class NodeToServiceAsyncOperation
            : public RequestResponseAsyncOperationBase
        {
        public:
            NodeToServiceAsyncOperation(
                Transport::MessageUPtr && message,
                Federation::RequestReceiverContextUPtr && receiverContext,
                Transport::IpcServer & ipcServer,
                Reliability::ServiceAdminClient & serviceAdminClient,
                Hosting2::IHostingSubsystemSPtr hosting,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            ~NodeToServiceAsyncOperation();

            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const & asyncOperation,
                __out Transport::MessageUPtr & reply,
                __out Federation::RequestReceiverContextUPtr & receiverContext);

            __declspec (property(get = get_ReceiverContext)) Federation::RequestReceiverContextUPtr & ReceiverContext;
            Federation::RequestReceiverContextUPtr & get_ReceiverContext() { return receiverContext_; }

            __declspec(property(get = get_ReceivedMessage)) Transport::MessageUPtr const & ReceivedMessage;
            Transport::MessageUPtr const & get_ReceivedMessage() const { return receivedMessage_; }

            __declspec(property(get = get_Reply, put = set_Reply)) Transport::MessageUPtr & Reply;
            Transport::MessageUPtr const & get_Reply() const { return reply_; }
            void set_Reply(Transport::MessageUPtr && value) { reply_ = std::move(value); }

        protected:
            virtual void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override;
            virtual void OnCompleted() override;

        private:
            Common::ErrorCode NodeToServiceAsyncOperation::GetHostId(
                ServiceModel::ServiceTypeIdentifier const & serviceTypeId,
                ServiceModel::ServicePackageVersionInstance const & packageVersionInstance,
                wstring const & applicationName,
                __out wstring& hostId);

            Common::ErrorCode NodeToServiceAsyncOperation::GetHostId(
                ServiceModel::ServiceTypeIdentifier const & serviceTypeId,
                ServiceModel::ServicePackageVersionInstance const & packageVersionInstance,
                wstring const & applicationName,
                ServiceModel::ServicePackageActivationContext const & activationContext,
                __out wstring& hostId);

            void ForwardToService(std::wstring & hostId, Common::AsyncOperationSPtr const & thisSPtr);

            void OnForwardToServiceComplete(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);
            void OnGetServiceDescriptionCompleted(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);

            Federation::RequestReceiverContextUPtr receiverContext_;
            Transport::IpcServer & ipcServer_;
            Hosting2::IHostingSubsystemSPtr hosting_;
            Reliability::ServiceAdminClient & serviceAdminClient_;
        };
    }
}
