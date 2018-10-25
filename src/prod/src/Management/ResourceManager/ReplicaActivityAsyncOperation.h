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
		class ReplicaActivityAsyncOperation
			: public Common::TimedAsyncOperation
		{
			using TReceiverContextUPtr = std::unique_ptr<TReceiverContext>;

			DENY_COPY(ReplicaActivityAsyncOperation)

		public:
			ReplicaActivityAsyncOperation(
				Transport::MessageUPtr requestMsg,
				TReceiverContextUPtr receiverContext,
				Common::TimeSpan const & timeout,
				Common::AsyncCallback const & callback,
				Common::AsyncOperationSPtr const & parent);

			static Common::ErrorCode End(
				Common::AsyncOperationSPtr const & asyncOperation,
				__out Transport::MessageUPtr & replyMsg,
				__out TReceiverContextUPtr & receiverContext);

		protected:
			// RequestMsg
			__declspec(property(get = get_RequestMsg)) Transport::Message & RequestMsg;
			Transport::Message & get_RequestMsg() const { return *(this->requestMsg_); }

			// ActivityId
			__declspec(property(get = get_ActivityId)) Common::ActivityId const & ActivityId;
			Common::ActivityId const & get_ActivityId() const { return this->activityId_; }

			// ReceiverContext
			__declspec(property(get = get_ReceiverContext)) TReceiverContext const & ReceiverContext;
			TReceiverContext const & get_ReceiverContext() const { return *(this->receiverContext_); }

			virtual void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override;

			virtual void OnTimeout(Common::AsyncOperationSPtr const & thisSPtr) override;

			virtual void Execute(Common::AsyncOperationSPtr const & thisSPtr) = NULL;

			void SetReply(std::unique_ptr<ServiceModel::ClientServerMessageBody> body);

			void SetReply(Transport::MessageUPtr && replyMsg);

			virtual void Complete(Common::AsyncOperationSPtr const & thisSPtr, Common::ErrorCode const & error);

		private:
			Common::ActivityId const activityId_;
			Transport::MessageUPtr requestMsg_;
			Transport::MessageUPtr replyMsg_;
			TReceiverContextUPtr receiverContext_;
		};

		using IpcReplicaActivityAsyncOperation = ReplicaActivityAsyncOperation<Transport::IpcReceiverContext>;
		using FedereationReplicaActivityAsyncOperation = ReplicaActivityAsyncOperation<Federation::RequestReceiverContext>;
	}
}