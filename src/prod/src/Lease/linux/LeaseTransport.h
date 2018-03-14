// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

typedef Transport::IDatagramTransportSPtr PLEASE_TRANSPORT;
typedef Transport::ISendTarget::SPtr PTRANSPORT_SENDTARGET;

typedef void (*TRANSPORT_MESSAGE_RECEIVED_CALLBACK)(PLEASE_TRANSPORT Listener, PTRANSPORT_SENDTARGET const & Target, PVOID Message, ULONG MessageSize, PVOID State);
typedef void (*TRANSPORT_MESSAGE_SENDING_CALLBACK)(PVOID State, BOOLEAN sending);

NTSTATUS TransportCreate(
    __in PTRANSPORT_LISTEN_ENDPOINT pAddress,
    Transport::SecuritySettings const* securitySettings,
    __in PVOID Context,
    __out PLEASE_TRANSPORT & transport);

NTSTATUS TransportListen(PLEASE_TRANSPORT const & transport, __in TRANSPORT_MESSAGE_RECEIVED_CALLBACK callback, __in PVOID state);
void TransportRelease(PLEASE_TRANSPORT & transport);

VOID TransportAbortConnections(__in PTRANSPORT_SENDTARGET const & Target);
void TransportClearConnections(__in PLEASE_TRANSPORT const & Transport);

struct _LEASE_AGENT_CONTEXT;
NTSTATUS TransportClose(_LEASE_AGENT_CONTEXT* leaseAgent);
VOID TransportAbort(_LEASE_AGENT_CONTEXT* pTransport);
bool IsTransportClosed(_LEASE_AGENT_CONTEXT const* pTransport);

NTSTATUS TransportResolveTarget(__in PLEASE_TRANSPORT const & pTransport, __in PTRANSPORT_LISTEN_ENDPOINT address, __out PTRANSPORT_SENDTARGET & ppTarget);
NTSTATUS TransportReleaseTarget(__in PTRANSPORT_SENDTARGET & pTarget);
NTSTATUS TransportBlockConnection(__in PTRANSPORT_SENDTARGET const & Target, __in BOOLEAN isBlocking);

std::wstring const & TransportListenEndpoint(__in PTRANSPORT_SENDTARGET Target);
std::wstring const & TransportTargetEndpoint(__in PTRANSPORT_SENDTARGET Target);
unsigned short TransportTargetSenderListenPort(__in PTRANSPORT_SENDTARGET Target);
PWCHAR TransportIdentifier(__in PLEASE_TRANSPORT const & Transport);
PWCHAR TransportIdentifier(__in PTRANSPORT_LISTEN_ENDPOINT endpoint);
PWCHAR TransportTargetIdentifier(__in PTRANSPORT_SENDTARGET const & Target);

NTSTATUS ListenEndpointToWString(
    TRANSPORT_LISTEN_ENDPOINT const * endpoint,
    __out_bcount(sizeInBytes) PWCHAR string,
    USHORT sizeInBytes);

NTSTATUS TransportSendBuffer(__in PTRANSPORT_SENDTARGET const & pTarget, __in PVOID Buffer, ULONG Size);
NTSTATUS TransportSendBufferNotification(
    __in PTRANSPORT_SENDTARGET const & pTarget,
    __in PVOID Buffer,
    ULONG Size,
    __in_opt TRANSPORT_MESSAGE_SENDING_CALLBACK messageSendingCallback,
    __in_opt PVOID sendingCallbackState);
