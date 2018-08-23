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
		class ClaimResourceAsyncOperation : public ClaimBasedAsyncOperation<TReceiverContext>
		{
			using TReceiverContextUPtr = std::unique_ptr<TReceiverContext>;

		public:
			ClaimResourceAsyncOperation(
				Management::ResourceManager::Context & context,
				Transport::MessageUPtr requestMsg,
				TReceiverContextUPtr receiverContext,
				Common::TimeSpan const & timeout,
				Common::AsyncCallback const & callback,
				Common::AsyncOperationSPtr const & parent);

		protected:
			virtual void Execute(
				Common::AsyncOperationSPtr const & thisSPtr) override;

		private:
			void ClaimResourceCallback(
				Common::AsyncOperationSPtr const & thisSPtr, 
				Common::AsyncOperationSPtr const & operationSPtr);
		};
	}
}