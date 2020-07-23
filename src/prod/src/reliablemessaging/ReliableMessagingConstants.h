// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

namespace ReliableMessaging
{
	/* start global configuration constants */

	static const LONG transportMessageDequeueWaitersInitialCount = 20;
	static const LONG environmentQueuedInboundMessagePoolInitialCount = 2000;
	static const LONG environmentQueuedInboundMessagePoolIncrement = 100;
	static const LONG environmentAsyncSendOperationPoolInitialCount = 2000;
	static const LONG environmentAsyncSendOperationPoolIncrement = 100;
	static const LONG environmentAsyncReceiveOperationPoolInitialCount = 2000;
	static const LONG environmentAsyncReceiveOperationPoolIncrement = 100;
	static const LONG environmentAsyncProtocolOperationPoolInitialCount = 10;
	static const LONG environmentAsyncProtocolOperationPoolIncrement = 10;
	static const LONG sessionAsyncSendOperationQuota = 2000;
	static const LONG sessionQueuedInboundMessageQuota = 2000;
	static const LONG sessionAsyncReceiveOperationQuota = 2000;
	static const ULONG sessionIdTableInitialSize = 1000;
	static const ULONG defaultContentMessageRetryIntervalInMilliSeconds = 200;
	static const ULONG defaultProtocolMessageRetryIntervalInMilliSeconds = 100;
	static const ULONG ackIntervalInMilliSeconds = 100;
	static const ULONG maxMessageAckBatchSize = 100;
	static const ULONG ackBatchSizeToWindowSizeFactor = 4;
	static const ULONG maxSendWindowSize = maxMessageAckBatchSize * ackBatchSizeToWindowSizeFactor;
	static const ULONG sendDelayForAckInMilliSeconds = 10;
	static const int maxRetryCountForAbortMessage = 3;

	/* end global configuration constants */
}
