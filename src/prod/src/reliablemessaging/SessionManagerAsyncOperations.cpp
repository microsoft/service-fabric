// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

namespace ReliableMessaging
{
	K_FORCE_SHARED_NO_OP_DESTRUCTOR(AsyncCreateOutboundSessionOperation)
	K_FORCE_SHARED_NO_OP_DESTRUCTOR(AsyncRegisterInboundSessionCallbackOperation)
	K_FORCE_SHARED_NO_OP_DESTRUCTOR(AsyncUnregisterInboundSessionCallbackOperation)

	HRESULT AsyncCreateOutboundSessionOperation::StartCreateSession(
					__in ReliableMessagingServicePartitionSPtr &targetServicePartition,
					__in std::wstring const &targetEndpoint,
					__in_opt KAsyncContextBase* const ParentAsyncContext,
					__in_opt KAsyncContextBase::CompletionCallback CallbackPtr)

	{
		PARTITION_TRACE(AsyncCreateOutboundSessionStarted,targetServicePartition);
		creationResult_ = sessionManagerContext_->CreateOutboundSession(targetServicePartition, targetEndpoint, sessionResult_);
		PARTITION_TRACE_WITH_HRESULT(AsyncCreateOutboundSessionCompleted,targetServicePartition,creationResult_.ToHResult());
		Start(ParentAsyncContext, CallbackPtr);
		return S_OK;
	}


	HRESULT AsyncRegisterInboundSessionCallbackOperation::StartRegisterInboundSessionCallback(
				__in IFabricReliableInboundSessionCallback *listenerCallback,
				__in_opt KAsyncContextBase* const ParentAsyncContext,
				__in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
	{
		PARTITION_TRACE(AsyncRegisterInboundSessionCallbackStarted,sessionManagerContext_->OwnerServicePartition);
		registrationResult_ = sessionManagerContext_->RegisterInboundSessionCallback(listenerCallback, sessionListenerEndpointResult_);
		PARTITION_TRACE_WITH_HRESULT(AsyncRegisterInboundSessionCallbackCompleted,sessionManagerContext_->OwnerServicePartition,registrationResult_.ToHResult());
		Start(ParentAsyncContext, CallbackPtr);
		return S_OK;
	}


	HRESULT AsyncUnregisterInboundSessionCallbackOperation::StartUnregisterInboundSessionCallback(
				__in_opt KAsyncContextBase* const ParentAsyncContext,
				__in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
	{
		PARTITION_TRACE(AsyncUnregisterInboundSessionCallbackStarted,sessionManagerContext_->OwnerServicePartition);
		deregistrationResult_ = sessionManagerContext_->UnregisterReliableInboundSessionCallback();
		PARTITION_TRACE_WITH_HRESULT(AsyncUnregisterInboundSessionCallbackCompleted,sessionManagerContext_->OwnerServicePartition,deregistrationResult_.ToHResult());
		Start(ParentAsyncContext, CallbackPtr);
		return S_OK;
	}

	Common::ErrorCode AsyncCreateOutboundSessionOperation::Create(
		__in SessionManagerServiceSPtr const &sessionManagerContext, 
		__out AsyncCreateOutboundSessionOperationSPtr &result)
	{
		result = _new(RMSG_ALLOCATION_TAG, sessionManagerContext->GetThisAllocator())
												AsyncCreateOutboundSessionOperation(sessionManagerContext);
						 						     			 
		KTL_CREATE_CHECK_AND_RETURN(result,AsyncOperationCreateFailed,AsyncCreateOutboundSessionOperation)
	}

	Common::ErrorCode AsyncRegisterInboundSessionCallbackOperation::Create(
		__in SessionManagerServiceSPtr const &sessionManagerContext, 
		__out AsyncRegisterInboundSessionCallbackOperationSPtr &result)
	{
		result = _new(RMSG_ALLOCATION_TAG, sessionManagerContext->GetThisAllocator())
												AsyncRegisterInboundSessionCallbackOperation(sessionManagerContext);
						 						     			 
		KTL_CREATE_CHECK_AND_RETURN(result,AsyncOperationCreateFailed,AsyncRegisterInboundSessionCallbackOperation)
	}

	Common::ErrorCode AsyncUnregisterInboundSessionCallbackOperation::Create(
		__in SessionManagerServiceSPtr const &sessionManagerContext, 
		__out AsyncUnregisterInboundSessionCallbackOperationSPtr &result)
	{
		result = _new(RMSG_ALLOCATION_TAG, sessionManagerContext->GetThisAllocator())
												AsyncUnregisterInboundSessionCallbackOperation(sessionManagerContext);
						 						     			 
		KTL_CREATE_CHECK_AND_RETURN(result,AsyncOperationCreateFailed,AsyncUnregisterInboundSessionCallbackOperation)
	}
}
