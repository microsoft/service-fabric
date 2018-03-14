// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
#define IPC_STRUCTURED_TRACE(trace_name, trace_id, trace_level, ...) \
    STRUCTURED_TRACE(trace_name, IPC, trace_id, trace_level, __VA_ARGS__)

    class IpcEventSource
    {
    public:
        DECLARE_STRUCTURED_TRACE(ServerFailedToSend, std::wstring, std::wstring, uint32);
        DECLARE_STRUCTURED_TRACE(ServerCannotSendToUnknownClient, std::wstring, std::wstring, MessageId, Actor::Trace, std::wstring);
        DECLARE_STRUCTURED_TRACE(ServerCreated, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE(ServerReceivedReconnect, std::wstring, std::wstring, uint32, std::wstring);
        DECLARE_STRUCTURED_TRACE(ServerReceivedBadReconnect, std::wstring, MessageId, std::wstring);
        DECLARE_STRUCTURED_TRACE(ServerFailedToRemoveClient, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE(ServerRemovedClient, std::wstring, std::wstring);

        DECLARE_STRUCTURED_TRACE(ClientCreated, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE(ClientReconnect, std::wstring, MessageId);

        IpcEventSource()
            : IPC_STRUCTURED_TRACE(
                ServerFailedToSend,
                4,
                Warning,
                "failed to send to client {1} in process {2}",
                "id",
                "clientId",
                "clientProcessId")
            , IPC_STRUCTURED_TRACE(
                ServerCannotSendToUnknownClient,
                5,
                Warning,
                "cannot send, client {1} not found, dropping message {2}, Actor = {3}, Action = '{4}'",
                "id",
                "clientId",
                "messageId",
                "actor",
                "action")
            , IPC_STRUCTURED_TRACE(
                ClientReconnect,
                6,
                Info,
                "send {1} to reconnect",
                "id",
                "message")
            , IPC_STRUCTURED_TRACE(
                ServerCreated,
                7,
                Info,
                "created for {1}",
                "id",
                "owner")
            , IPC_STRUCTURED_TRACE(
                ServerReceivedReconnect,
                8,
                Info,
                "received from client {1} in process {2} at {3}",
                "id",
                "clientId",
                "clientProcess",
                "clientAddress")
            , IPC_STRUCTURED_TRACE(
                ServerReceivedBadReconnect,
                9,
                Error,
                "dropping message {1} from {2}",
                "id",
                "message",
                "clientAddress")
            , IPC_STRUCTURED_TRACE(
                ServerFailedToRemoveClient,
                10,
                Warning,
                "client {1} not found",
                "id",
                "message",
                "clientId")
            , IPC_STRUCTURED_TRACE(
                ServerRemovedClient,
                11,
                Info,
                "client {1} removed",
                "id",
                "clientId")
            , IPC_STRUCTURED_TRACE(
                ClientCreated,
                12,
                Info,
                "created for {1}",
                "id",
                "owner")
        {
        }
    };
}
