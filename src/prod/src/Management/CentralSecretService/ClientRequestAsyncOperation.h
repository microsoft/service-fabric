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
			: public Management::ResourceManager::IpcAutoRetryAsyncOperation
			, public Store::ReplicaActivityTraceComponent<Common::TraceTaskCodes::CentralSecretService>
		{
		public:
			ClientRequestAsyncOperation(
				Management::CentralSecretService::SecretManager & secretManager,
				Management::ResourceManager::IpcResourceManagerService & resourceManager,
				Transport::MessageUPtr requestMsg,
				Transport::IpcReceiverContextUPtr receiverContext,
				Common::TimeSpan const & timeout,
				Common::AsyncCallback const & callback,
				Common::AsyncOperationSPtr const & parent);

			static Common::ErrorCode End(
				Common::AsyncOperationSPtr const & asyncOperation,
				__out Transport::MessageUPtr & replyMsg,
				__out Transport::IpcReceiverContextUPtr & receiverContext);

		protected:
			using Management::ResourceManager::IpcAutoRetryAsyncOperation::ActivityId;
			using Management::ResourceManager::IpcAutoRetryAsyncOperation::get_ActivityId;

			__declspec(property(get = get_SecretManager)) Management::CentralSecretService::SecretManager & SecretManager;
			Management::CentralSecretService::SecretManager & get_SecretManager() const { return this->secretManager_; }

			__declspec(property(get = get_ResourceManager)) Management::ResourceManager::IpcResourceManagerService & ResourceManager;
			Management::ResourceManager::IpcResourceManagerService & get_ResourceManager() const { return this->resourceManager_; }

		private:
			Management::CentralSecretService::SecretManager & secretManager_;
			Management::ResourceManager::IpcResourceManagerService & resourceManager_;
		};
	}
}
