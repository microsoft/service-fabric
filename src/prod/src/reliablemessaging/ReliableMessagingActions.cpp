// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace ReliableMessaging
{
	namespace ProtocolAction
	{
		Common::StringLiteral CodeAsString(Code code)
		{
			switch (code)
			{
			case OpenReliableSessionRequest: return "OpenReliableSessionRequest";
			case OpenReliableSessionRequestAck: return "OpenReliableSessionRequestAck";
			case OpenReliableSessionResponse: return "OpenReliableSessionResponse";
			case OpenReliableSessionResponseAck: return "OpenReliableSessionResponseAck";
			case CloseReliableSessionRequest: return "CloseReliableSessionRequest";
			case CloseReliableSessionRequestAck: return "CloseReliableSessionRequestAck";
			case CloseReliableSessionResponse: return "CloseReliableSessionResponse";
			case CloseReliableSessionResponseAck: return "CloseReliableSessionResponseAck";
			case AbortOutboundReliableSessionRequest: return "AbortOutboundReliableSessionRequest";
			case AbortOutboundReliableSessionRequestAck: return "AbortOutboundReliableSessionRequestAck";
			case AbortInboundReliableSessionRequest: return "AbortInboundReliableSessionRequest";
			case AbortInboundReliableSessionRequestAck: return "AbortInboundReliableSessionRequest";
			case SendReliableMessage: return "SendReliableMessage";
			case SendReliableMessageAck: return "SendReliableMessageAck";
			default: Common::Assert::CodingError("ProtocolAction::CodeAsString called with unknown code {0}", (int)code);
				break;
			}
		}
	};
};
