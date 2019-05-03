// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"


namespace ReliableMessaging
{
	const ULONG QueuedInboundMessage::LinkOffset = FIELD_OFFSET(QueuedInboundMessage, qlink_);
	const ULONG InboundMessageQueue::DequeueOperation::LinkOffset = FIELD_OFFSET(InboundMessageQueue::DequeueOperation, _Links);

	K_FORCE_SHARED_NO_OP_DESTRUCTOR(QueuedInboundMessage)
	K_FORCE_SHARED_NO_OP_DESTRUCTOR(InboundMessageQueue)

	QueuedInboundMessage::QueuedInboundMessage()
	{
		message_ = nullptr;
		responseTarget_ = nullptr;
	}

	InboundMessageQueue::InboundMessageQueue() : KAsyncQueue<QueuedInboundMessage>(RMSG_ALLOCATION_TAG,QueuedInboundMessage::LinkOffset)
	{}

	QueuedInboundMessage* InboundMessageQueue::TryDequeue()
	{
		QueuedInboundMessage*  queuedItem = nullptr;
		ULONG   queuedItemPriority = 0;

		K_LOCK_BLOCK(_ThisLock)
		{
			if (_IsActive)	// guard against close/abort races
			{
				queuedItem = UnsafeGetNextEnqueuedItem(&queuedItemPriority);
			}

			if (queuedItem != nullptr)
			{
				UnsafeReleaseActivity();
				// release corresponding activity count #1 - see UnsafeEnqueueOrGetWaiter()
			}
		}

		return queuedItem;
	}
}
