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
		class ResourceManagerAsyncOperation
			: public AutoRetryAsyncOperation<TReceiverContext>
			, public Store::ReplicaActivityTraceComponent<Common::TraceTaskCodes::ResourceManager>
		{
			using TReceiverContextUPtr = std::unique_ptr<TReceiverContext>;

			DENY_COPY(ResourceManagerAsyncOperation)
		public:
			ResourceManagerAsyncOperation(
				Management::ResourceManager::Context & executionContext,
				Transport::MessageUPtr requestMsg,
				TReceiverContextUPtr receiverContext,
				Common::TimeSpan const & timeout,
				Common::AsyncCallback const & callback,
				Common::AsyncOperationSPtr const & parent);

			template<typename TAsyncOperation>
			static Common::AsyncOperationSPtr CreateAndStart(
				__in Management::ResourceManager::Context & context,
				__in Transport::MessageUPtr request,
				__in TReceiverContextUPtr requestContext,
				Common::TimeSpan const & timeout,
				Common::AsyncCallback const & callback,
				Common::AsyncOperationSPtr const & root)
			{
				static_assert(
					is_base_of<ResourceManagerAsyncOperation<TReceiverContext>, TAsyncOperation>::value,
					"TAsyncOperation must be derived from ResourceManagerAsyncOperation<TReceiverContext>.");

				return Common::AsyncOperation::CreateAndStart<TAsyncOperation>(
					context,
					move(request),
					move(requestContext),
					timeout,
					callback,
					root);
			}
		protected:
			using AutoRetryAsyncOperation<TReceiverContext>::ActivityId;
			using AutoRetryAsyncOperation<TReceiverContext>::get_ActivityId;

			// ExecutionContext
			__declspec(property(get = get_ExecutionContext)) ResourceManager::Context & ExecutionContext;
			ResourceManager::Context & get_ExecutionContext() { return this->executionContext_; }

		private:
			ResourceManager::Context & executionContext_;
		};

		template<typename TReceiverContext>
		class CompletedResourceManagerAsyncOperation : public ResourceManagerAsyncOperation<TReceiverContext>
		{
		public:
			CompletedResourceManagerAsyncOperation(
				__in Management::ResourceManager::Context & executionContext,
				__in Common::ErrorCode const & error,
				Common::AsyncCallback const & callback,
				Common::AsyncOperationSPtr const & parent);

			static AsyncOperationSPtr CreateAndStart(
				__in Management::ResourceManager::Context & context,
				__in Common::ErrorCode const & error,
				Common::AsyncCallback const & callback,
				Common::AsyncOperationSPtr const & parent);
		protected:
			virtual void Execute(Common::AsyncOperationSPtr const & thisSPtr) override;

		private:
			Common::ErrorCode completionError_;
		};
	}
}