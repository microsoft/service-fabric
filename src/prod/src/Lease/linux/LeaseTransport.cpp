// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <sys/epoll.h>
#include "Common/EventLoop.h"
#include "Transport/IoBuffer.h"
#include "Transport/SendBuffer.h"
#include "Transport/ReceiveBuffer.h"
#include "Transport/IBufferFactory.h"
#include "Transport/LTBufferFactory.h"
#include "Federation/FederationConfig.h"

using namespace Transport;
using namespace Common;
using namespace std;

namespace
{
    const StringLiteral TraceType("LeaseTransport");

    EventLoopPool* eventLoopPool = nullptr;
    INIT_ONCE initPoolOnce = INIT_ONCE_STATIC_INIT;

    BOOL CALLBACK InitEventLoopPool(PINIT_ONCE, PVOID, PVOID*)
    {
        // create a dedicated EventLoopPool for isolation, also, dispatch socket events
        // synchronously to avoid potential delay in thread pool, assuming the number
        // of socket events for lease transport is small and their processing is fast
        eventLoopPool = new EventLoopPool(L"lease");
        return TRUE;
    }

    EventLoopPool* GetEventLoopPool()
    {
        PVOID lpContext = NULL;
        BOOL  bStatus = ::InitOnceExecuteOnce(
            &initPoolOnce,
            InitEventLoopPool,
            nullptr,
            nullptr);

        ASSERT_IF(!bStatus, "Failed to initialize EventLoopPool for LeaseTransport");
        return eventLoopPool; 
    }
}

_Use_decl_annotations_
NTSTATUS TransportCreate(
    PTRANSPORT_LISTEN_ENDPOINT pAddress,
    SecuritySettings const* securitySettings,
    PVOID LeaseAgentContext,
    PLEASE_TRANSPORT & transport)
{
    Invariant(!securitySettings || securitySettings->IsRemotePeer());

    auto listenAddress = TcpTransportUtility::ConstructAddressString(pAddress->Address, pAddress->Port);
    transport = DatagramTransportFactory::CreateTcp(listenAddress, listenAddress, L"lease");
    transport->SetEventLoopPool(GetEventLoopPool());
    transport->SetEventLoopReadDispatch(false);
    transport->SetEventLoopWriteDispatch(false);
    transport->SetBufferFactory(make_unique<LTBufferFactory>());
    transport->DisableListenInstanceMessage();
    transport->DisableThrottle();
    transport->SetConnectionOpenTimeout(Federation::FederationConfig::GetConfig().ConnectionOpenTimeout);

    if (securitySettings)
    {
        ASSERT_IFNOT(
            (securitySettings->SecurityProvider() == SecurityProvider::None) ||
            (securitySettings->SecurityProvider() == SecurityProvider::Ssl),
            "{0} not supported", *securitySettings);

        auto error = transport->SetSecurity(*securitySettings);
        if (!error.IsSuccess()) return error.ToHResult();
    }

    return STATUS_SUCCESS;
}

NTSTATUS TransportListen(PLEASE_TRANSPORT const & transport, __in TRANSPORT_MESSAGE_RECEIVED_CALLBACK callback, __in PVOID state)
{
    transport->SetMessageHandler([callback, state] (MessageUPtr & message, ISendTarget::SPtr const & sender) {
        vector<const_buffer> buffers;
        if (!message->GetBody(buffers))
        {
            LeaseTrace::WriteError(TraceType, "failed to get body buffers from incoming message");
            sender->Reset();
            return;
        }

        size_t totalBytes = 0;
        for(auto const & buffer : buffers)
        {
            totalBytes += buffer.size();
        }

        auto bufUPtr = std::make_unique<BYTE[]>(totalBytes);
        auto buf = bufUPtr.get(); 

        auto ptr = buf;
        for(auto const & buffer : buffers)
        {
            memcpy(ptr, buffer.buf, buffer.len);
            ptr += buffer.len;
        }

        callback(NULL, sender, buf, totalBytes, state);//jc
    });

    return transport->Start().ToHResult();
}

void TransportRelease(PLEASE_TRANSPORT & transport)
{
    transport.reset();
}

VOID TransportAbortConnections(__in PTRANSPORT_SENDTARGET const & Target)
{
    Target->Reset();
}

void TransportClearConnections(__in PLEASE_TRANSPORT const & Transport)
{
    Transport->Test_Reset();
}

static TimeSpan TransportStopWait = TimeSpan::FromSeconds(10);
NTSTATUS TransportClose(_LEASE_AGENT_CONTEXT* leaseAgent)
{
    leaseAgent->Transport->Stop(TransportStopWait);
    leaseAgent->TransportClosed = true;
    return STATUS_SUCCESS;
}

VOID TransportAbort(_LEASE_AGENT_CONTEXT* leaseAgent)
{
    leaseAgent->Transport->Stop(TransportStopWait);
}

bool IsTransportClosed(_LEASE_AGENT_CONTEXT const* leaseAgent)
{
    return leaseAgent->TransportClosed;
}

NTSTATUS TransportResolveTarget(__in PLEASE_TRANSPORT const & Transport, __in PTRANSPORT_LISTEN_ENDPOINT Address, __out PTRANSPORT_SENDTARGET & target)
{
    target = Transport->ResolveTarget(TcpTransportUtility::ConstructAddressString(Address->Address, Address->Port));
    return STATUS_SUCCESS;
}

NTSTATUS TransportReleaseTarget(__in PTRANSPORT_SENDTARGET & Target)
{
    Target.reset();
    return STATUS_SUCCESS;
}

NTSTATUS TransportBlockConnection(__in PTRANSPORT_SENDTARGET const & Target, __in BOOLEAN isBlocking)
{
    //LINUXTODO consider use UnreliabableTransport
    if (isBlocking)
    {
        Target->Test_Block();
    }
    else
    {
        Target->Test_Unblock();
    }

    return STATUS_SUCCESS;
}



std::wstring const & TransportListenEndpoint(__in PTRANSPORT_SENDTARGET Target)
{
    return Target->LocalAddress();//jc return &(Transport->InputListenEndpoint());
}

std::wstring const & TransportTargetEndpoint(__in PTRANSPORT_SENDTARGET Target)
{
    return Target->Address();//jc return &(Target->GetAddress());
}

unsigned short TransportTargetSenderListenPort(__in PTRANSPORT_SENDTARGET Target)
{
    return 0;//jc return Target->GetSenderListenPort();
}

PWCHAR TransportIdentifier(__in PLEASE_TRANSPORT const & Transport)
{
    return (PWCHAR)Transport->IdString.c_str();
}

PWCHAR TransportIdentifier(__in PTRANSPORT_LISTEN_ENDPOINT endpoint)
{
    return (PWCHAR) TcpTransportUtility::ConstructAddressString(endpoint->Address, endpoint->Port).c_str();
}

PWCHAR TransportTargetIdentifier(__in PTRANSPORT_SENDTARGET const & Target)
{
    return (PWCHAR)Target->Id().c_str();
}

NTSTATUS ListenEndpointToWString(
    TRANSPORT_LISTEN_ENDPOINT const * endpoint,
    __out_bcount(sizeInBytes) PWCHAR string,
    USHORT sizeInBytes)
{
    auto listenAddress = TcpTransportUtility::ConstructAddressString(endpoint->Address, endpoint->Port);
    auto toCopy = (listenAddress.size() + 1) * sizeof(WCHAR);
    if (sizeInBytes < toCopy) return STATUS_INVALID_PARAMETER;

    memcpy(string, listenAddress.c_str(), toCopy);
    return STATUS_SUCCESS;
}

NTSTATUS TransportSendBuffer(__in PTRANSPORT_SENDTARGET const & pTarget, __in PVOID Buffer, ULONG Size)
{
    return TransportSendBufferNotification(pTarget, Buffer, Size, NULL, NULL);
}

NTSTATUS TransportSendBufferNotification(__in PTRANSPORT_SENDTARGET const & Target, __in PVOID Buffer, ULONG Size, __in_opt TRANSPORT_MESSAGE_SENDING_CALLBACK messageSendingCallback, __in_opt PVOID sendingCallbackState)
{
    if (Target == nullptr)
    {
        ExFreePool(Buffer);
        return STATUS_INVALID_PARAMETER;
    }

    //LINUXTODO, consider derive lease message type from IFabricSerializble,
    //instead of relying on SerializeLeaseMessage and DeserializeLeaseMessage
    vector<const_buffer> bufferList(1, const_buffer(Buffer, Size));
    auto msg = make_unique<Message>(
        bufferList,
        [] (std::vector<Common::const_buffer> const &, void * buffer) { ExFreePool(buffer); },
        Buffer);

    auto error = Target->SendOneWay(move(msg), TimeSpan::MaxValue);
    NTSTATUS status = error.ToHResult();
    if (NT_SUCCESS(status))
    {
        //LINUXTODO consider support sending callback in IDatagramTransport
        if (messageSendingCallback)
        Threadpool::Post([=] { messageSendingCallback(sendingCallbackState, TRUE); });
    }

    return status;
}
