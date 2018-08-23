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
			void Execute(Common::AsyncOperationSPtr const & thisSPtr) override;
		};
	}
}
