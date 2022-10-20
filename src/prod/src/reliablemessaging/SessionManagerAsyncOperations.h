// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

namespace ReliableMessaging
{
	// Needed for unknown reasons to get K_FORCE_SHARED to compile properly below
	using ::_delete;

	class AsyncCreateOutboundSessionOperation : public SessionAsyncOperationContext
	{
		K_FORCE_SHARED(AsyncCreateOutboundSessionOperation);

	public:

		static Common::ErrorCode Create(
			__in SessionManagerServiceSPtr const &sessionManagerContext, 
			__out AsyncCreateOutboundSessionOperationSPtr &result);

		HRESULT StartCreateSession(
				__in ReliableMessagingServicePartitionSPtr &targetServicePartition,
				__in std::wstring const &targetEndpoint,
				__in_opt KAsyncContextBase* const ParentAsyncContext,
				__in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

		ReliableSessionServiceSPtr GetResult()
		{
			return sessionResult_;
		}

	private:

		void OnStart() override
		{
			Complete(creationResult_);
		}

		AsyncCreateOutboundSessionOperation(SessionManagerServiceSPtr sessionManagerContext) 
							  : sessionManagerContext_(sessionManagerContext) 
							  											   
		{}

		Common::ErrorCode creationResult_;
		SessionManagerServiceSPtr sessionManagerContext_;
		ReliableSessionServiceSPtr sessionResult_;
	};


	class AsyncRegisterInboundSessionCallbackOperation : public SessionAsyncOperationContext
	{
		friend class SessionManagerService;
		K_FORCE_SHARED(AsyncRegisterInboundSessionCallbackOperation);

	public:

		static Common::ErrorCode Create(
			__in SessionManagerServiceSPtr const &sessionManagerContext, 
			__out AsyncRegisterInboundSessionCallbackOperationSPtr &result);

		HRESULT StartRegisterInboundSessionCallback(
					__in IFabricReliableInboundSessionCallback *listenerCallback,
					__in_opt KAsyncContextBase* const ParentAsyncContext,
					__in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

		std::wstring GetResult()
		{
			return sessionListenerEndpointResult_;
		}


	private:

		void OnStart() override
		{
			Complete(registrationResult_);
		}

		AsyncRegisterInboundSessionCallbackOperation(SessionManagerServiceSPtr sessionManagerContext) 
							  : sessionManagerContext_(sessionManagerContext) 
							  											   
		{}

		Common::ErrorCode registrationResult_;
		SessionManagerServiceSPtr sessionManagerContext_;
		std::wstring sessionListenerEndpointResult_;
	};

	class AsyncUnregisterInboundSessionCallbackOperation : public SessionAsyncOperationContext
	{
		K_FORCE_SHARED(AsyncUnregisterInboundSessionCallbackOperation);

	public:

		static Common::ErrorCode Create(
			__in SessionManagerServiceSPtr const &sessionManagerContext, 
			__out AsyncUnregisterInboundSessionCallbackOperationSPtr &result);

		HRESULT StartUnregisterInboundSessionCallback(
				__in_opt KAsyncContextBase* const ParentAsyncContext,
				__in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

	private:

		void OnStart() override
		{
			Complete(deregistrationResult_);
		}

		AsyncUnregisterInboundSessionCallbackOperation(SessionManagerServiceSPtr sessionManagerContext) 
							  : sessionManagerContext_(sessionManagerContext) 
							  											   
		{}

		Common::ErrorCode deregistrationResult_;
		SessionManagerServiceSPtr sessionManagerContext_;
	};
}
