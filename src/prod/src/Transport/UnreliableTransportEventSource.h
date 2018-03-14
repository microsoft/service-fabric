// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
#define UNRELIABLE_TRANSPORT_STRUCTURED_TRACE(trace_name, trace_id, trace_level, ...) \
    STRUCTURED_TRACE(trace_name, UnreliableTransport, trace_id, trace_level, __VA_ARGS__)

    class UnreliableTransportEventSource
    {
    public:
        DECLARE_STRUCTURED_TRACE(Drop, std::wstring, std::wstring, Transport::MessageId);
        DECLARE_STRUCTURED_TRACE(Delay, std::wstring, std::wstring, Transport::MessageId, Common::TimeSpan);

        static UnreliableTransportEventSource const Trace;

        UnreliableTransportEventSource()
            : UNRELIABLE_TRANSPORT_STRUCTURED_TRACE(
                Drop,
                4,
                Info,
                "dropping message {1} {2}",
                "id",
                "messageAction",
                "messageId"),
            UNRELIABLE_TRANSPORT_STRUCTURED_TRACE(
                Delay,
                5,
                Info,
                "delaying message {1} {2} by {3}",
                "id",
                "messageAction",
                "messageId",
                "delay")
        {
        }
    };
}
