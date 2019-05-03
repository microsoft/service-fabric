// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace CentralSecretService
    {
        class ClientRequestAsyncOperation
            : public Common::TimedAsyncOperation
            , public Store::ReplicaActivityTraceComponent<Common::TraceTaskCodes::CentralSecretService>
        {
        public:
            ClientRequestAsyncOperation(
                Management::CentralSecretService::SecretManager & secretManager,
                Management::ResourceManager::IpcResourceManagerService & resourceManager,
                Transport::MessageUPtr requestMsgUPtr,
                Transport::IpcReceiverContextUPtr receiverContextUPtr,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const & asyncOperation,
                __out Transport::MessageUPtr & replyMsgUPtr,
                __out Transport::IpcReceiverContextUPtr & receiverContextUPtr);

        protected:
            using Store::ReplicaActivityTraceComponent<Common::TraceTaskCodes::CentralSecretService>::ActivityId;
            using Store::ReplicaActivityTraceComponent<Common::TraceTaskCodes::CentralSecretService>::get_ActivityId;

            // RequestMsg
            __declspec(property(get = get_RequestMsg)) Transport::Message & RequestMsg;
            Transport::Message & get_RequestMsg() const { return *(this->requestMsgUPtr_); }

            // ReceiverContext
            __declspec(property(get = get_ReceiverContext)) Transport::IpcReceiverContext const & ReceiverContext;
            Transport::IpcReceiverContext const & get_ReceiverContext() const { return *(this->receiverContextUPtr_); }

            __declspec(property(get = get_SecretManager)) Management::CentralSecretService::SecretManager & SecretManager;
            Management::CentralSecretService::SecretManager & get_SecretManager() const { return this->secretManager_; }

            __declspec(property(get = get_ResourceManager)) Management::ResourceManager::IpcResourceManagerService & ResourceManager;
            Management::ResourceManager::IpcResourceManagerService & get_ResourceManager() const { return this->resourceManager_; }

            __declspec(property(get = get_TraceComponent)) Common::StringLiteral const & TraceComponent;
            virtual Common::StringLiteral const & get_TraceComponent() const = NULL;

            void SetReply(std::unique_ptr<ServiceModel::ClientServerMessageBody> body);

            void SetReply(Transport::MessageUPtr && replyMsg);

            virtual void OnTimeout(AsyncOperationSPtr const &) override;
            virtual void OnStart(AsyncOperationSPtr const &) override;
            virtual void OnCompleted() override;
            virtual void OnCancel() override;

            virtual void Execute(Common::AsyncOperationSPtr const & thisSPtr) = NULL;

            void Complete(Common::AsyncOperationSPtr const & thisSPtr, Common::ErrorCode const & errorCode);

        private:
            Management::CentralSecretService::SecretManager & secretManager_;
            Management::ResourceManager::IpcResourceManagerService & resourceManager_;
            Transport::MessageUPtr requestMsgUPtr_;
            Transport::MessageUPtr replyMsgUPtr_;
            Transport::IpcReceiverContextUPtr receiverContextUPtr_;
        };
    }
}
