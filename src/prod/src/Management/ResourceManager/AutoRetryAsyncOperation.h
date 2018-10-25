// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
	namespace ResourceManager
	{
		template <typename TReceiverContext>
		class AutoRetryAsyncOperation
			: public ReplicaActivityAsyncOperation<TReceiverContext>
		{
			using TReceiverContextUPtr = std::unique_ptr<TReceiverContext>;
			DENY_COPY(AutoRetryAsyncOperation)

		public:
			AutoRetryAsyncOperation(
				Common::TimeSpan const & maxRetryDelay,
				Transport::MessageUPtr requestMsg,
				TReceiverContextUPtr receiverContext,
				Common::TimeSpan const & timeout,
				Common::AsyncCallback const & callback,
				Common::AsyncOperationSPtr const & parent);

		protected:
			virtual bool IsErrorRetryable(Common::ErrorCode const & error);
			virtual void Complete(Common::AsyncOperationSPtr const & thisSPtr, Common::ErrorCode const & error) override;
			virtual void OnTimeout(Common::AsyncOperationSPtr const & thisSPtr) override;
			virtual void OnCancel() override;

		private:
			Common::TimeSpan maxRetryDelay_;
			Common::ErrorCode lastRetryError_;
			Common::atomic_bool keepRetry_;
			Common::TimerSPtr retryTimer_;

			using ReplicaActivityAsyncOperation<TReceiverContext>::TryComplete;
			using ReplicaActivityAsyncOperation<TReceiverContext>::FinishComplete;
			using ReplicaActivityAsyncOperation<TReceiverContext>::TryStartComplete;

			void StopRetry();
			void Retry(Common::AsyncOperationSPtr const & thisSPtr, Common::ErrorCode const & error);
		};

		using IpcAutoRetryAsyncOperation = AutoRetryAsyncOperation<Transport::IpcReceiverContext>;
		using FedereationAutoRetryAsyncOperation = AutoRetryAsyncOperation<Federation::RequestReceiverContext>;
	}
}
