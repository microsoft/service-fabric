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
			void Execute(Common::AsyncOperationSPtr const & thisSPtr) override;

			void OnCompleted() override;
		private:
			void UnregisterResourcesCallback(
				Common::AsyncOperationSPtr const & thisSPtr,
				Common::AsyncOperationSPtr const & operationSPtr,
				SecretReferencesDescription const & description);

			void RemoveSecretsCallback(
				Common::AsyncOperationSPtr const & thisSPtr,
				Common::AsyncOperationSPtr const & operationSPtr,
				SecretReferencesDescription const & description);
		};
	}
}