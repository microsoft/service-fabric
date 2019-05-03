// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

namespace ReliableMessaging
{
    class ReliableMessagingEnvironment : public KObject<ReliableMessagingEnvironment>, public KShared<ReliableMessagingEnvironment>
    {
        K_FORCE_SHARED(ReliableMessagingEnvironment);

    public:

        ReliableMessagingEnvironment(__in KAllocator &);

        QueuedInboundMessageSPtr GetQueuedInboundMessage()
        {
            QueuedInboundMessageSPtr message = nullptr;
            GetPooledResource(message);
            return message;
        }

        void ReturnQueuedInboundMessage(QueuedInboundMessageSPtr& message)
        {
            ReturnPooledResource(message);
        }

        void GetPooledResource(__out QueuedInboundMessageSPtr& message)
        {
            message = queuedInboundMessagePool_.GetItemGrowIfNeeded();
        }

        void ReturnPooledResource(QueuedInboundMessageSPtr& message)
        {
            queuedInboundMessagePool_.ReturnItem(message);
        }

        void GetPooledResource(__out AsyncSendOperationSPtr& sendOp)
        {
            sendOp = asyncSendOperationPool_.GetItemGrowIfNeeded();
        }

        void ReturnPooledResource(AsyncSendOperationSPtr& sendOp)
        {
            asyncSendOperationPool_.ReturnItem(sendOp);
        }

        void GetPooledResource(__out AsyncReceiveOperationSPtr& receiveOp)
        {
            receiveOp = asyncReceiveOperationPool_.GetItemGrowIfNeeded();
        }

        void ReturnPooledResource(AsyncReceiveOperationSPtr& receiveOp)
        {
            asyncReceiveOperationPool_.ReturnItem(receiveOp);
        }
        
        void GetPooledResource(__out AsyncProtocolOperationSPtr& protocolOp)
        {
            protocolOp = asyncProtocolOperationPool_.GetItemGrowIfNeeded();
        }

        void ReturnPooledResource(AsyncProtocolOperationSPtr& protocolOp)
        {
            asyncProtocolOperationPool_.ReturnItem(protocolOp);
        }
        

        static ReliableMessagingEnvironmentSPtr TryGetsingletonEnvironment();

        KSpinLock sessionManagerCommonLock_;
        KSpinLock sessionServiceCommonLock_;
        KSpinLock sendAckWaitersLock_;
        ReliableProtocolAckKeyMap sessionProtocolAckWaiters_;
        ReliableSessionServiceIdMap sessionOpenResponseWaiters_;
        ReliableSessionServiceIdMap sessionCloseResponseWaiters_;
        ReliableSessionServiceIdHashTable commonReliableInboundSessionsById_;
        ReliableSessionServiceIdHashTable commonReliableOutboundSessionsById_;
        SessionManagerServiceMap sessionManagers_;
        SessionManagerServiceMap sessionManagerInboundSessionRequestFinderIndex_;
        ReliableMessagingTransportSPtr transport_;
        ResourcePool<QueuedInboundMessage> queuedInboundMessagePool_;
        ResourcePool<AsyncSendOperation> asyncSendOperationPool_;
        ResourcePool<AsyncReceiveOperation> asyncReceiveOperationPool_;
        ResourcePool<AsyncProtocolOperation> asyncProtocolOperationPool_;

        volatile unsigned __int64 numberOfMessagesReceived_;
        volatile unsigned __int64 numberOfMessagesDropped_;


        static KSpinLock singletonEnvironmentLock_;
        static ReliableMessagingEnvironmentSPtr singletonEnvironment_;    
    };
}
