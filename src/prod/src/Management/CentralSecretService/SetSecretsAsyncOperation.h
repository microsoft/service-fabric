// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace CentralSecretService
    {
        class SetSecretsAsyncOperation : public ClientRequestAsyncOperation
        {
        public:
            SetSecretsAsyncOperation(
                Management::CentralSecretService::SecretManager & secretManager,
                Management::ResourceManager::IpcResourceManagerService & resourceManager,
                Transport::MessageUPtr requestMsg,
                Transport::IpcReceiverContextUPtr receiverContext,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

        protected:
            virtual Common::StringLiteral const & get_TraceComponent() const override { return SetSecretsAsyncOperation::TraceComponent; }

            void Execute(Common::AsyncOperationSPtr const & thisSPtr) override;

        private:
            static StringLiteral const TraceComponent;

            std::vector<Secret> secrets_;

            void CompleteRegisterResourcesOperation(
                Common::AsyncOperationSPtr const & operationSPtr,
                bool expectedCompletedSynchronously);

            void CompleteSetSecretsOperation(
                Common::AsyncOperationSPtr const & operationSPtr,
                bool expectedCompletedSynchronously);
        };
    }
}