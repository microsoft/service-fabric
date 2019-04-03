// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace CentralSecretService
    {
        class GetSecretsAsyncOperation : public ClientRequestAsyncOperation
        {
        public:
            GetSecretsAsyncOperation(
                Management::CentralSecretService::SecretManager & secretManager,
                Management::ResourceManager::IpcResourceManagerService & resourceManager,
                Transport::MessageUPtr requestMsg,
                Transport::IpcReceiverContextUPtr receiverContext,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

        protected:
            virtual Common::StringLiteral const & get_TraceComponent() const override { return GetSecretsAsyncOperation::TraceComponent; }

            void Execute(Common::AsyncOperationSPtr const & thisSPtr) override;

        private:
            static StringLiteral const TraceComponent;

            void CompleteGetSecretsOperation(Common::AsyncOperationSPtr const & operationSPtr, bool expectedCompletedSynchronously);
        };
    }
}
