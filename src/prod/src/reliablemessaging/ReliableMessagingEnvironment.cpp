// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"


namespace ReliableMessaging
{
    KSpinLock ReliableMessagingEnvironment::singletonEnvironmentLock_;
    ReliableMessagingEnvironmentSPtr ReliableMessagingEnvironment::singletonEnvironment_ = nullptr;

    ULONG SessionIdHash(const Common::Guid& key)
    {
        return key.GetHashCode();
    }

    K_FORCE_SHARED_NO_OP_DESTRUCTOR(ReliableMessagingEnvironment)

    ReliableMessagingEnvironment::ReliableMessagingEnvironment(__in KAllocator & allocator) : 
                            sessionManagers_(&ReliableMessagingServicePartition::compare,allocator),
                            sessionManagerInboundSessionRequestFinderIndex_(&ReliableMessagingServicePartition::compareForInt64Find,allocator),
                            sessionProtocolAckWaiters_(&ProtocolAckKey::compare,allocator),
                            sessionOpenResponseWaiters_(&guidCompare,allocator),
                            sessionCloseResponseWaiters_(&guidCompare,allocator),
                            commonReliableOutboundSessionsById_(sessionIdTableInitialSize,&SessionIdHash,ReliableSessionService::KeyOffset,ReliableSessionService::LinkOffset,allocator),
                            commonReliableInboundSessionsById_(sessionIdTableInitialSize,&SessionIdHash,ReliableSessionService::KeyOffset,ReliableSessionService::LinkOffset,allocator),
                            queuedInboundMessagePool_(allocator,QueuedInboundMessage::LinkOffset),
                            asyncSendOperationPool_(allocator,AsyncSendOperation::LinkOffset),
                            asyncReceiveOperationPool_(allocator,AsyncReceiveOperation::LinkOffset),
                            asyncProtocolOperationPool_(allocator,AsyncProtocolOperation::LinkOffset),
                            numberOfMessagesReceived_(0),
                            numberOfMessagesDropped_(0)
    {

        transport_ = _new (RMSG_ALLOCATION_TAG,allocator) ReliableMessagingTransport(KSharedPtr<ReliableMessagingEnvironment>(this));

        if (transport_==nullptr) 
        {
            SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
            return;
        }
        else if (!NT_SUCCESS(transport_->Status()))
        {
            SetConstructorStatus(transport_->Status());
            return;
        }

        Common::ErrorCode result = transport_->Start();

        if (!result.IsSuccess())
        {
            // TODO: there seems to be no clear way to report the actual failure here as an NTSTATUS
            SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
            return;
        }

        result = queuedInboundMessagePool_.Initialize(environmentQueuedInboundMessagePoolInitialCount,environmentQueuedInboundMessagePoolIncrement);

        if (!result.IsSuccess())
        {
            SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
            return;
        }

        result = asyncSendOperationPool_.Initialize(environmentAsyncSendOperationPoolInitialCount,environmentAsyncSendOperationPoolIncrement);

        if (!result.IsSuccess())
        {
            SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
            return;
        }

        result = asyncReceiveOperationPool_.Initialize(environmentAsyncReceiveOperationPoolInitialCount,environmentAsyncReceiveOperationPoolIncrement);

        if (!result.IsSuccess())
        {
            SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
            return;
        }

        result = asyncProtocolOperationPool_.Initialize(environmentAsyncProtocolOperationPoolInitialCount,environmentAsyncProtocolOperationPoolIncrement);

        if (!result.IsSuccess())
        {
            SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
            return;
        }
    }


    ReliableMessagingEnvironmentSPtr ReliableMessagingEnvironment::TryGetsingletonEnvironment()
    {
        // TODO: Revisit after we enable multiple Ktl systems
        KtlSystem *system = &Common::GetSFDefaultKtlSystem();
        if (system == nullptr) { return nullptr; }

        system->SetDefaultSystemThreadPoolUsage(FALSE);

        ReliableMessagingEnvironmentSPtr result;

        K_LOCK_BLOCK(ReliableMessagingEnvironment::singletonEnvironmentLock_)
        {
            if (ReliableMessagingEnvironment::singletonEnvironment_==nullptr)
            {
                ReliableMessagingEnvironment::singletonEnvironment_ = _new (RMSG_ALLOCATION_TAG,system->PagedAllocator()) ReliableMessagingEnvironment(system->PagedAllocator());

                if (ReliableMessagingEnvironment::singletonEnvironment_ == nullptr || !NT_SUCCESS(ReliableMessagingEnvironment::singletonEnvironment_->Status()))
                {
                    ReliableMessagingEnvironment::singletonEnvironment_ = nullptr;
                    return nullptr;
                }
            }

            result = ReliableMessagingEnvironment::singletonEnvironment_;
        }

        return result;
    }

}
