// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

namespace ReliableMessaging
{

	/***

	this was used for STL maps -- don't need it any more but not delting yet in case it is needed again somewhere given the tricky syntax

	template<class T>
	struct lessthan
		: public std::binary_function<T, T, bool>
	{	// functor for comparison operator
		bool operator()(const T& left, const T& right) const
		{
			return (left.lessThan(right));
		}
	};

	***/



	typedef std::vector<Common::const_buffer> MessageBufferVector;

	class InboundSessionRequestedCallback;
	typedef KSharedPtr<InboundSessionRequestedCallback> InboundSessionRequestedCallbackSPtr;

	class ReliableSessionAbortedCallback;
	typedef KSharedPtr<ReliableSessionAbortedCallback> ReliableSessionAbortedCallbackSPtr;

	class ComFabricReliableSessionMessage;
	typedef KSharedPtr<ComFabricReliableSessionMessage> ComFabricReliableSessionMessageSPtr;

	class ReliableMessagingEnvironment;
	typedef KSharedPtr<ReliableMessagingEnvironment> ReliableMessagingEnvironmentSPtr;

	class AsyncOutboundOpenOperation;
	typedef KSharedPtr<AsyncOutboundOpenOperation> AsyncOutboundOpenOperationSPtr;

	class AsyncOutboundCloseOperation;
	typedef KSharedPtr<AsyncOutboundCloseOperation> AsyncOutboundCloseOperationSPtr;

	class AsyncProtocolOperation;
	typedef KSharedPtr<AsyncProtocolOperation> AsyncProtocolOperationSPtr;

	class AsyncSendOperation;
	typedef KSharedPtr<AsyncSendOperation> AsyncSendOperationSPtr;

	class AsyncReceiveOperation;
	typedef KSharedPtr<AsyncReceiveOperation> AsyncReceiveOperationSPtr;

	class AsyncCreateOutboundSessionOperation;
	typedef KSharedPtr<AsyncCreateOutboundSessionOperation> AsyncCreateOutboundSessionOperationSPtr;

	class AsyncRegisterInboundSessionCallbackOperation;
	typedef KSharedPtr<AsyncRegisterInboundSessionCallbackOperation> AsyncRegisterInboundSessionCallbackOperationSPtr;

	class AsyncUnregisterInboundSessionCallbackOperation;
	typedef KSharedPtr<AsyncUnregisterInboundSessionCallbackOperation> AsyncUnregisterInboundSessionCallbackOperationSPtr;	

	class ReliableMessagingServicePartition;
	typedef KSharedPtr<ReliableMessagingServicePartition> ReliableMessagingServicePartitionSPtr;

	class SessionManagerService;
	typedef KSharedPtr<SessionManagerService> SessionManagerServiceSPtr; 

	class ReliableMessagingTransport;
	typedef KSharedPtr<ReliableMessagingTransport> ReliableMessagingTransportSPtr; 

	class ReliableSessionService;
	typedef KSharedPtr<ReliableSessionService> ReliableSessionServiceSPtr;

	class InboundMessageQueue;
	typedef KSharedPtr<InboundMessageQueue> InboundMessageQueueSPtr;

	typedef KAvlTree<ReliableSessionServiceSPtr,ReliableMessagingServicePartitionSPtr> 
		ReliableSessionServiceOwnerMap;

	typedef HashTableMap<Common::Guid,ReliableSessionService> 
		ReliableSessionServiceIdHashTable;

	typedef KAvlTree<ReliableSessionServiceSPtr,Common::Guid> 
		ReliableSessionServiceIdMap;

	typedef KAvlTree<SessionManagerServiceSPtr,ReliableMessagingServicePartitionSPtr> 
		SessionManagerServiceMap;

	class ProtocolAckKey;
	typedef KAvlTree<AsyncProtocolOperationSPtr,ProtocolAckKey> 
		ReliableProtocolAckKeyMap;

	class QueuedInboundMessage;
	typedef KSharedPtr<QueuedInboundMessage> QueuedInboundMessageSPtr;
}
