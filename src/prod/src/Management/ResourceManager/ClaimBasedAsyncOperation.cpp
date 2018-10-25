// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;

using namespace Management::ResourceManager;

template <typename TReceiverContext>
ClaimBasedAsyncOperation<TReceiverContext>::ClaimBasedAsyncOperation(
	Management::ResourceManager::Context & executionContext,
	MessageUPtr requestMsg,
	TReceiverContextUPtr receiverContext,
	TimeSpan const & timeout,
	AsyncCallback const & callback,
	AsyncOperationSPtr const & parent)
	: ResourceManagerAsyncOperation<TReceiverContext>(
		executionContext,
		move(requestMsg),
		move(receiverContext),
		timeout,
		callback,
		parent)
{
}

template <typename TReceiverContext>
bool ClaimBasedAsyncOperation<TReceiverContext>::ValidateRequestAndRetrieveClaim(AsyncOperationSPtr const & thisSPtr)
{
	return this->RetrieveClaim(thisSPtr) && this->ValidateRequest(thisSPtr);
}

template <typename TReceiverContext>
bool ClaimBasedAsyncOperation<TReceiverContext>::ValidateRequest(AsyncOperationSPtr const & thisSPtr)
{
	RequestInstanceHeader requestInstanceHeader;
	
	if (!this->RequestMsg.Headers.TryReadFirst<RequestInstanceHeader>(requestInstanceHeader))
	{
		this->Complete(
			thisSPtr,
			ErrorCode(ErrorCodeValue::InvalidMessage,
				wformatString("Missing RequestInstanceHeader in the message. MessageId={0}, TraceId={1}",
					this->RequestMsg.MessageId,
					this->RequestMsg.TraceId())));
		return false;
	}

	for (ResourceIdentifier const & consumerId : this->Claim.ConsumerIds)
	{
		MessageUPtr failureReply;
		wstring key;
		StringWriter keyBuilder(key);

		keyBuilder.Write(this->Claim.ResourceId);
		keyBuilder.Write(consumerId);

		if (!this->ExecutionContext.RequestTracker.TryAcceptRequest(this->ActivityId, key, requestInstanceHeader.Instance, failureReply))
		{
			this->SetReply(move(failureReply));
			this->Complete(thisSPtr,
				ErrorCode(ErrorCodeValue::InvalidMessage,
					wformatString("Request message instance validation failed. MessageId={0}, TraceId={1}",
						this->RequestMsg.MessageId,
						this->RequestMsg.TraceId())));
			return false;
		}
	}

	return true;
}

template <typename TReceiverContext>
bool ClaimBasedAsyncOperation<TReceiverContext>::RetrieveClaim(AsyncOperationSPtr const & thisSPtr)
{
	ErrorCode error(ErrorCodeValue::Success);

	if (!this->RequestMsg.GetBody<Management::ResourceManager::Claim>(this->claim_))
	{
		error = ErrorCode::FromNtStatus(this->RequestMsg.GetStatus());
		this->Complete(thisSPtr, error);
		return false;
	}

	return true;
}

template class Management::ResourceManager::ClaimBasedAsyncOperation<IpcReceiverContext>;
template class Management::ResourceManager::ClaimBasedAsyncOperation<Federation::RequestReceiverContext>;
