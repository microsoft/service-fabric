// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace CentralSecretService
    {
        class RemoveSecretsAsyncOperation : public ClientRequestAsyncOperation
        {
        public:
            RemoveSecretsAsyncOperation(
                Management::CentralSecretService::SecretManager & secretManager,
                Management::ResourceManager::IpcResourceManagerService & resourceManager,
                Transport::MessageUPtr requestMsg,
                Transport::IpcReceiverContextUPtr receiverContext,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

        protected:
            virtual Common::StringLiteral const & get_TraceComponent() const override { return RemoveSecretsAsyncOperation::TraceComponent; }

            void Execute(Common::AsyncOperationSPtr const & thisSPtr) override;

        private:
            static StringLiteral const TraceComponent;

            SecretReferencesDescription description_;

            void CompleteGetSecretsOperation(
                Common::AsyncOperationSPtr const & operationSPtr,
                bool expectedCompletedSynchronously);
            
            void CompleteUnregisterResourcesOperation(
                Common::AsyncOperationSPtr const & operationSPtr,
                bool expectedCompletedSynchronously);

            void CompleteRemoveSecretsOperation(
                Common::AsyncOperationSPtr const & operationSPtr,
                bool expectedCompletedSynchronously);
        };
    }
}