// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

using namespace Transport;

/*
	This file contains the implementation of the private KAsyncQueue message pump mechanisms for KMessagingTransportService
	The functionality here uses NTSTATUS in line with KTL which is the base for this mechanism
*/


namespace ReliableMessaging
{
	// Needed for unknown reasons to get K_FORCE_SHARED to compile properly below
	using ::_delete;

	/*
	QueuedInboundMessage is designed to be queued into KAsyncQueue
	*/
	class QueuedInboundMessage sealed : public KObject<QueuedInboundMessage>, public KShared<QueuedInboundMessage>
	{

		K_FORCE_SHARED(QueuedInboundMessage);

	public:

		static const ULONG LinkOffset;
       
		__declspec(property(get=get_responseTarget)) ISendTarget::SPtr const & ResponseTarget;
        ISendTarget::SPtr const & get_responseTarget() const { return responseTarget_; }

		__declspec(property(get=get_sessionHeader)) ReliableSessionHeader const & SessionHeader;
        ReliableSessionHeader const & get_sessionHeader() const { return sessionHeader_; }

		__declspec(property(get=get_isCloseMessage)) bool IsCloseMessage;
        bool get_isCloseMessage() const { return isCloseMessage_; }

		// Create a blank poolable QueuedInboundMessage
		static Common::ErrorCode Create(
				__in KAllocator &allocator, 
				__out QueuedInboundMessage::SPtr& result)
		{		
			result = _new(RMSG_ALLOCATION_TAG, allocator) QueuedInboundMessage();
			KTL_CREATE_CHECK_AND_RETURN(result,KtlObjectCreateFailed,QueuedInboundMessage)
		}

		void Reuse()
		{
			message_ = nullptr;
			responseTarget_ = nullptr;
			memset(&sessionHeader_,0,sizeof(ReliableSessionHeader));
		}

		// TODO: we need a consistent policy for when to track and use responseTarget and when to ignore it
		void Initialize(
				__in MessageUPtr &&message, 
				__in ISendTarget::SPtr const & responseTarget,
				__in ReliableSessionHeader const &sessionHeader
				)
		{
			message_ = std::move(message);
			responseTarget_ = responseTarget;
			sessionHeader_ = sessionHeader;
			isCloseMessage_ = false;
		}


		void InitializeClosureMessage(
				__in ReliableSessionHeader const &sessionHeader
				)
		{
			message_ = nullptr;
			responseTarget_ = nullptr;
			sessionHeader_ = sessionHeader;
			isCloseMessage_ = true;
		}


		// Create a pre-initialized QueuedInboundMessage
		static Common::ErrorCode Create(
				__in MessageUPtr &&message, 
				__in ISendTarget::SPtr const & responseTarget,
				__out QueuedInboundMessage::SPtr& result)
		{

				
			result = _new(RMSG_ALLOCATION_TAG, Common::GetSFDefaultPagedKAllocator())
													QueuedInboundMessage(std::move(message),responseTarget);

			KTL_CREATE_CHECK_AND_RETURN(result,KtlObjectCreateFailed,QueuedInboundMessage)
		}

		QueuedInboundMessage(
				MessageUPtr &&message, 
				ISendTarget::SPtr const & responseTarget,
				bool isCloseMessage=false
			) : message_(std::move(message)), 
			    responseTarget_(responseTarget), 
			    isCloseMessage_(isCloseMessage)
		{}

		void GetMessageUPtr(__out MessageUPtr &message)
		{
			if (message_ == nullptr) 
			{
				Common::Assert::CodingError("QueuedInboundMessage has no message when dequeued");
			}
			else
			{
				message = std::move(message_);
				message_ = nullptr;
			}
		}


	private:

		KListEntry qlink_;
		MessageUPtr message_;
		bool isCloseMessage_;
		ISendTarget::SPtr responseTarget_;
		ReliableSessionHeader sessionHeader_;
	};


	/*
	InboundMessageQueue supports synchronous Enqueue and asynchronous Dequeue: the latter through the DequeueOperation class
	*/

	class InboundMessageQueue sealed : public KAsyncQueue<QueuedInboundMessage>
	{
		K_FORCE_SHARED(InboundMessageQueue);

	public:

		static Common::ErrorCode Create(__out InboundMessageQueueSPtr& result)
		{
			result = _new(RMSG_ALLOCATION_TAG, Common::GetSFDefaultPagedKAllocator())
											InboundMessageQueue();

			KTL_CREATE_CHECK_AND_RETURN(result,KtlObjectCreateFailed,InboundMessageQueue)
		}

		// Asynchronous Dequeue support for the InboundMessageQueue via specialization of DequeueOperation
		class DequeueOperation : public KAsyncQueue<QueuedInboundMessage>::DequeueOperation
		{
		public:

			static const ULONG LinkOffset;
 
		private:

			friend class InboundMessageQueue;
			
			// for Transport level listener dequeue operations
			DequeueOperation(InboundMessageQueue& Owner, ULONG const OwnerVersion) :
				KAsyncQueue<QueuedInboundMessage>::DequeueOperation(Owner, OwnerVersion), 
				receiveOp_(nullptr)
			{}

			// for Session level dequeue operations with an outstanding receive as context
			DequeueOperation(const AsyncReceiveOperationSPtr &receiveOp, InboundMessageQueue& Owner, ULONG const OwnerVersion) :
				KAsyncQueue<QueuedInboundMessage>::DequeueOperation(Owner, OwnerVersion), 
				receiveOp_(receiveOp)
			{}

			void OnReuse() override
			{
				receiveOp_ = nullptr;
				__super::OnReuse();
			}

			// async context when used for regular session receive operation
			AsyncReceiveOperationSPtr receiveOp_;

		}; // End definition of specialized DequeueOperation for InboundMessageQueue

		// InboundMessageQueue::DequeueOperation factory method for transport level listener
		Common::ErrorCode CreateDequeueOperation(
			__out InboundMessageQueue::DequeueOperation::SPtr& result)
		{
			result = _new(_AllocationTag, GetThisAllocator()) InboundMessageQueue::DequeueOperation(*this, _ThisVersion);

			KTL_CREATE_CHECK_AND_RETURN(result,KtlObjectCreateFailed,DequeueOperationForTransport)
		}

		// InboundMessageQueue::DequeueOperation factory method for session level outstanding receive operations
		// TODO: we are keeping receiveOp in the DequeueOperation to keep it alive--need to revisit lifetime management interaction via COM refcounting
		Common::ErrorCode  CreateDequeueOperation(
			__in const AsyncReceiveOperationSPtr &receiveOp,	// receive operation context
			__out InboundMessageQueue::DequeueOperation::SPtr& result)
		{
			result = _new(_AllocationTag, GetThisAllocator()) InboundMessageQueue::DequeueOperation(receiveOp,*this, _ThisVersion);

			KTL_CREATE_CHECK_AND_RETURN(result,KtlObjectCreateFailed,DequeueOperationForSession)
		}

		QueuedInboundMessage* TryDequeue();


	private:

		// Hide some of the KAsyncQueue public interface
		using KAsyncQueue::Create;
		using KAsyncQueue::CancelEnqueue;

	};
}
