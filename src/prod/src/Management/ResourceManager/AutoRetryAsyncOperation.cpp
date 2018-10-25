// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Store;
using namespace Management::ResourceManager;

StringLiteral const TraceComponent("AutoRetryAsyncOperation");

template <typename TReceiverContext>
AutoRetryAsyncOperation<TReceiverContext>::AutoRetryAsyncOperation(
	TimeSpan const & maxRetryDelay,
	MessageUPtr requestMsg,
	TReceiverContextUPtr receiverContext,
	TimeSpan const & timeout,
	AsyncCallback const & callback,
	AsyncOperationSPtr const & parent)
	: ReplicaActivityAsyncOperation<TReceiverContext>(
		move(requestMsg),
		move(receiverContext),
		timeout,
		callback,
		parent)
	, maxRetryDelay_(maxRetryDelay)
	, keepRetry_(true)
	, retryTimer_(nullptr)
	, lastRetryError_(ErrorCode::Success())
{
}

template <typename TReceiverContext>
void AutoRetryAsyncOperation<TReceiverContext>::StopRetry()
{
	this->keepRetry_.store(false);
	this->retryTimer_->Cancel();
}

template <typename TReceiverContext>
void AutoRetryAsyncOperation<TReceiverContext>::OnCancel()
{
	this->StopRetry();
	ReplicaActivityAsyncOperation<TReceiverContext>::OnCancel();
}

template <typename TReceiverContext>
void AutoRetryAsyncOperation<TReceiverContext>::OnTimeout(AsyncOperationSPtr const & thisSPtr)
{
	this->StopRetry();
	ReplicaActivityAsyncOperation<TReceiverContext>::OnTimeout(thisSPtr);
}

template <typename TReceiverContext>
void AutoRetryAsyncOperation<TReceiverContext>::Complete(AsyncOperationSPtr const & thisSPtr, ErrorCode const & error)
{
	ErrorCode finalError(error);

	if (this->IsErrorRetryable(error))
	{
		if ((!thisSPtr->IsCompleted) && (this->keepRetry_.load()))
		{
			this->Retry(thisSPtr, error);
			return;
		}
	}
	else
	{
		// Distinguish non-retryable sequence number check failures (caused by the expected version
		// passed in by the client) from retryable ones (returned by the replicated store).
		// StaleRequest is used to indicate the non-retryable type, but this should get remapped to
		// FABRIC_E_SEQUENCE_NUMBER_CHECK_FAILED (StoreSequenceNumberCheckFailed) just before replying
		// to the client.
		if (error.IsError(ErrorCodeValue::StaleRequest))
		{
			finalError = ErrorCode(ErrorCodeValue::StoreSequenceNumberCheckFailed, wstring(error.Message));
		}
	}

	ReplicaActivityAsyncOperation<TReceiverContext>::Complete(thisSPtr, finalError);
}

template <typename TReceiverContext>
bool AutoRetryAsyncOperation<TReceiverContext>::IsErrorRetryable(ErrorCode const & error)
{
	switch (error.ReadValue())
	{
	case ErrorCodeValue::NoWriteQuorum:
	case ErrorCodeValue::ReconfigurationPending:
		//case ErrorCodeValue::StoreRecordAlreadyExists: // same as StoreWriteConflict
	case ErrorCodeValue::StoreOperationCanceled:
	case ErrorCodeValue::StoreWriteConflict:
	case ErrorCodeValue::StoreSequenceNumberCheckFailed:
		return true;

	default:
		return false;
	}
}

template <typename TReceiverContext>
void AutoRetryAsyncOperation<TReceiverContext>::Retry(AsyncOperationSPtr const & thisSPtr, ErrorCode const & error)
{
	TimeSpan retryDelay = StoreTransaction::GetRandomizedOperationRetryDelay(error, this->maxRetryDelay_);

	this->retryTimer_ =
		Timer::Create(
			TimerTagDefault,
			[this, thisSPtr](TimerSPtr const & timer)
	{
		timer->Cancel();

		if (this->keepRetry_.load())
		{
			this->SetReply(MessageUPtr(nullptr));
			this->Execute(thisSPtr);
		}
	},
			true);
	this->retryTimer_->Change(retryDelay);
}


template class Management::ResourceManager::AutoRetryAsyncOperation<Transport::IpcReceiverContext>;
template class Management::ResourceManager::AutoRetryAsyncOperation<Federation::RequestReceiverContext>;
