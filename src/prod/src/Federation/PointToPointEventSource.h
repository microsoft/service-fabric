// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class PointToPointEventSource
    {
    public:
        Common::TraceEventWriter<std::wstring, std::wstring, NodeInstance, Transport::MessageId, bool, uint32> Send;
        Common::TraceEventWriter<std::wstring, std::wstring, NodeInstance, Transport::MessageId, bool, uint32> Receive;

        PointToPointEventSource() :
            Send(
                Common::TraceTaskCodes::P2P,
                4,
                "Send",
                Common::LogLevel::Info,
                "{1} send message {0} to {2} id {3} {4} ({5})",
                "id", "site", "to", "msgId", "isReply", "retry"),
            Receive(
                Common::TraceTaskCodes::P2P,
                5,
                "Receive",
                Common::LogLevel::Info,
                "{1} receive message {0} from {2} id {3} {4} ({5})",
                "id", "site", "from", "msgId", "isReply", "retry")
        {
        }
    };
}
