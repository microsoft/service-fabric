// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

namespace ReliableMessaging
{
	namespace ProtocolAction
	{
		// if you update this update CodeAsString function below as well
		enum Code {
			OpenReliableSessionRequest = 1000,		// Request from sender to open a session
			OpenReliableSessionRequestAck,			// signals that the Open request was received
			OpenReliableSessionResponse,			// signals that the reveiver has completed the open process
			OpenReliableSessionResponseAck,			// signals that response to the Open request was received
			CloseReliableSessionRequest,			// signals that sender has finished sending messages through the session
			CloseReliableSessionRequestAck,			// signals that the Close request was received
			CloseReliableSessionResponse,			// signals that the reveiver has completed the close process
			CloseReliableSessionResponseAck,		// signals that the  response to the Close request was received
			AbortOutboundReliableSessionRequest,	// signals that the receiver has aborted and the corresponding sender should abort
			AbortOutboundReliableSessionRequestAck,	// signals that the sender received the request to abort
			AbortInboundReliableSessionRequest,		// signals that the sender has aborted and the corresponding receiver should abort
			AbortInboundReliableSessionRequestAck,	// signals that the receiver received the request to abort
			SendReliableMessage,					// signals a regular message send in an open session
			SendReliableMessageAck,					// signals a batched acknowledgement reflecting dispatch to receiver queue in order (no gaps)
			Unknown
		};

		Common::StringLiteral CodeAsString(Code code);

		enum Direction {
			ToTargetPartition,
			ToSourcePartition
		};
	};

	enum SessionKind {
		Inbound,
		Outbound
	};
};
