// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
	namespace ResourceManager
	{
		template<typename TReceiverContext>
		class ClaimBasedAsyncOperation : public ResourceManagerAsyncOperation<TReceiverContext>
		{
			using TReceiverContextUPtr = std::unique_ptr<TReceiverContext>;

		public:
			ClaimBasedAsyncOperation(
				Management::ResourceManager::Context & executionContext,
				Transport::MessageUPtr requestMsg,
				TReceiverContextUPtr receiverContext,
				Common::TimeSpan const & timeout,
				Common::AsyncCallback const & callback,
				Common::AsyncOperationSPtr const & parent);
		protected:
			__declspec(property(get = get_Claim)) Management::ResourceManager::Claim const & Claim;
			Management::ResourceManager::Claim const & get_Claim() const { return this->claim_; }

			bool ValidateRequestAndRetrieveClaim(Common::AsyncOperationSPtr const & thisSPtr);
		private:
			Management::ResourceManager::Claim claim_;

			bool RetrieveClaim(Common::AsyncOperationSPtr const & thisSPtr);
			bool ValidateRequest(Common::AsyncOperationSPtr const & thisSPtr);
		};
	}
}