// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "DnsRemoteQueryOp.h"

/*static*/
void DnsRemoteQueryOp::Create(
    __out DnsRemoteQueryOp::SPtr& spRecurseOp,
    __in KAllocator& allocator,
    __in IDnsTracer& tracer,
    __in INetIoManager& netIoManager,
    __in IDnsParser& dnsParser,
    __in INetworkParams& networkParams,
    __in const DnsServiceParams& params
)
{
    spRecurseOp = _new(TAG, allocator) DnsRemoteQueryOp(tracer, netIoManager, dnsParser, networkParams, params);
    KInvariant(spRecurseOp != nullptr);
}

DnsRemoteQueryOp::DnsRemoteQueryOp(
    __in IDnsTracer& tracer,
    __in INetIoManager& netIoManager,
    __in IDnsParser& dnsParser,
    __in INetworkParams& networkParams,
    __in const DnsServiceParams& params
) : _tracer(tracer),
_params(params),
_dnsParser(dnsParser),
_networkParams(networkParams),
_numberOfBytesInBuffer(0),
_currentDnsServerIndex(0),
_currentDnsServiceIp(INADDR_NONE),
_context(nullptr)
{
    netIoManager.CreateUdpListener(/*out*/_spUdpListener, params.AllowMultipleListeners);
}

DnsRemoteQueryOp::~DnsRemoteQueryOp()
{
    _tracer.Trace(DnsTraceLevel_Noise, "Destructing DnsRemoteQueryOp.");
}

//***************************************
// BEGIN KAsyncContext region
//***************************************

void DnsRemoteQueryOp::OnStart()
{
    ActivateStateMachine();

    ChangeStateAsync(true);
}

void DnsRemoteQueryOp::OnCompleted()
{
    _tracer.Trace(DnsTraceLevel_Info, "DnsRemoteQueryOp OnCompleted called.");
    _callback(*_spBuffer, _numberOfBytesInBuffer, _context);
}

void DnsRemoteQueryOp::OnReuse()
{
    _tracer.Trace(DnsTraceLevel_Noise, "DnsRemoteQueryOp OnReuse called.");
    _context = nullptr;
}

void DnsRemoteQueryOp::OnCancel()
{
    _tracer.Trace(DnsTraceLevel_Info, "DnsRemoteQueryOp OnCancel called.");
    TerminateAsync();
}

//***************************************
// END KAsyncContext region
//***************************************




//***************************************
// BEGIN StateMachine region
//***************************************

void DnsRemoteQueryOp::OnBeforeStateChange(
    __in LPCWSTR fromState,
    __in LPCWSTR toState
)
{
    _tracer.Trace(DnsTraceLevel_Noise, 
        "DnsRemoteQueryOp activityId {0} change state {1} => {2}",
        WSTR(_activityId), WSTR(fromState), WSTR(toState));
}

void DnsRemoteQueryOp::OnStateEnter_Start()
{
}

void DnsRemoteQueryOp::OnStateEnter_InitNetworkParams()
{
    bool fSuccess = false;

    _currentDnsServiceIp = INADDR_NONE;
    _currentDnsServerIndex = 0;
    if (_networkParams.DnsServers().Count() > 0)
    {
        fSuccess = true;
    }

    ChangeStateAsync(fSuccess);
}

void DnsRemoteQueryOp::OnStateEnter_NextDnsServer()
{
    const KArray<ULONG>& arrDnsServers = _networkParams.DnsServers();

    if (_currentDnsServerIndex < arrDnsServers.Count())
    {
        _currentDnsServiceIp = arrDnsServers[_currentDnsServerIndex];
        _currentDnsServerIndex++;
        ChangeStateAsync(true);
        return;
    }

    ChangeStateAsync(false);
}

void DnsRemoteQueryOp::OnStateEnter_TransportOpen()
{
    // Port is dynamically assigned
    USHORT port = 0;

    UdpListenerCallback completionCallback(this, &DnsRemoteQueryOp::OnUdpListenerClosed);
    bool fSuccess = _spUdpListener->StartListener(
        this/*parent*/,
        /*inout*/port,
        completionCallback
    );

    if (!fSuccess)
    {
        _tracer.Trace(DnsTraceLevel_Error,
            "DnsRemoteQueryOp activityId {0}, failed to start the UDP listener",
            WSTR(_activityId));
    }

    ChangeStateAsync(fSuccess);
}

void DnsRemoteQueryOp::OnStateEnter_SendQuery()
{
    if (!_dnsParser.Serialize(/*out*/*_spBuffer, /*out*/_numberOfBytesInBuffer, *_spQueryMessage, _params.TimeToLiveInSeconds))
    {
        ChangeStateAsync(false);
        return;
    }

    ISocketAddress::SPtr spAddress;
    DNS::CreateSocketAddress(/*out*/spAddress, GetThisAllocator(), _currentDnsServiceIp, htons(53));

    // THIS async op guaranteed to be alive until the completion callback is invoked
    // due to THIS being a parent of UdpListener. UdpListener cannot complete if 
    // Read and Write ops are active.
    UdpWriteOpCompletedCallback callback(this, &DnsRemoteQueryOp::OnDnsQueryCompleted);
    _spUdpListener->WriteAsync(*_spBuffer, _numberOfBytesInBuffer, *spAddress, callback);
}

void DnsRemoteQueryOp::OnStateEnter_StartReceiveResponseOp()
{
    const ULONG timeoutInMs = _params.RecursiveQueryTimeoutInSeconds * 1000;

    // Start read op
    ISocketAddress::SPtr spAddress;
    DNS::CreateSocketAddress(/*out*/spAddress, GetThisAllocator());
    
    // THIS async op guaranteed to be alive until the completion callback is invoked
    // due to THIS being a parent of UdpListener. UdpListener cannot complete if 
    // Read and Write ops are active.
    UdpReadOpCompletedCallback callback(this, &DnsRemoteQueryOp::OnRemoteDnsQueryCompleted);
    _spUdpListener->ReadAsync(*_spBuffer, *spAddress, callback, timeoutInMs);
}

void DnsRemoteQueryOp::OnStateEnter_SuccessfullyReceivedResponse()
{
    ChangeStateAsync(true);
}

void DnsRemoteQueryOp::OnStateEnter_FailedToGetResponse()
{
    _numberOfBytesInBuffer = 0;
    ChangeStateAsync(true);
}

void DnsRemoteQueryOp::OnStateEnter_TransportClose()
{
    if (!_spUdpListener->CloseAsync())
    {
        // If the listener is already closed, just change state
        ChangeStateAsync(true);
    }
}

void DnsRemoteQueryOp::OnStateEnter_ShouldTryNextDnsServer()
{
    KInvariant(_spBuffer != nullptr);

    // Next DNS server should be queried only if the current DNS server was
    // unable to answer due to a timeout or internal failure.
    // We also don't support truncated responses since we only support UDP protocol.

    USHORT id, flags, qCount, aCount, nsCount, adCount;
    bool fSuccess = _dnsParser.GetHeader(/*out*/id, /*out*/flags, /*out*/qCount, /*out*/aCount, /*out*/nsCount, /*out*/adCount,
        *_spBuffer, _numberOfBytesInBuffer);

    if (fSuccess)
    {
        USHORT rc = DnsFlags::GetResponseCode(flags);
        fSuccess = DnsFlags::IsSuccessResponseCode(rc);
    }

    if (fSuccess)
    {
        bool fIsTruncated = DnsFlags::IsFlagSet(flags, DnsFlags::TRUNCATION);
        if (fIsTruncated)
        {
            // This is a success, but we want to return failure so secondary DNS would be contacted.
            _numberOfBytesInBuffer = 0;
        }
    }

    _tracer.Trace(DnsTraceLevel_Noise,
        "DnsRemoteQueryOp activityId {0}, query completed, server {1}, flags {2}, number of answers {3}",
        WSTR(_activityId), _currentDnsServiceIp, (ULONG)flags, (ULONG)aCount);

    ChangeStateAsync(!fSuccess);
}

void DnsRemoteQueryOp::OnStateEnter_RemoteQuerySucceeded()
{
    ChangeStateAsync(true);
}

void DnsRemoteQueryOp::OnStateEnter_RemoteQueryFailed()
{
    _numberOfBytesInBuffer = 0;
    ChangeStateAsync(true);
}

void DnsRemoteQueryOp::OnStateEnter_End()
{
    DeactivateStateMachine();

    Complete(STATUS_SUCCESS);
}

//***************************************
// END StateMachine region
//***************************************





void DnsRemoteQueryOp::StartRemoteQuery(
    __in_opt KAsyncContextBase* const parent,
    __in KStringView& activityId,
    __in KBuffer& buffer,
    __in IDnsMessage& message,
    __in DnsRemoteQueryOpCallback callback,
    __in_opt PVOID context
)
{
    _activityId = activityId;
    _callback = callback;
    _spBuffer = &buffer;
    _numberOfBytesInBuffer = 0;
    _spQueryMessage = &message;
    _context = context;

    _tracer.Trace(DnsTraceLevel_Noise, "DnsRemoteQueryOp activityId {0} started", WSTR(_activityId));

    Start(parent, nullptr/*callback*/);
}

void DnsRemoteQueryOp::OnUdpListenerClosed(
    __in NTSTATUS status
)
{
    bool fSuccess = (status == STATUS_SUCCESS);
    if (!fSuccess)
    {
        _tracer.Trace(DnsTraceLevel_Info, "DnsRemoteQueryOp activityId {0} UDP listener closed, status {1}",
            WSTR(_activityId), status);
    }

    ChangeStateAsync(fSuccess);
}

void DnsRemoteQueryOp::OnDnsQueryCompleted(
    __in NTSTATUS status,
    __in ULONG bytesSent
)
{
    bool fSuccess = (status == STATUS_SUCCESS);
    if (!fSuccess)
    {
        _tracer.Trace(DnsTraceLevel_Info, 
            "DnsRemoteQueryOp activityId {0}, dns query failed, status {1}, bytesSent {2}",
            WSTR(_activityId), status, bytesSent);
    }
    ChangeStateAsync(fSuccess);
}

void DnsRemoteQueryOp::OnRemoteDnsQueryCompleted(
    __in NTSTATUS status,
    __in KBuffer& buffer,
    __in ULONG bytesRead
)
{
    UNREFERENCED_PARAMETER(status);
    UNREFERENCED_PARAMETER(buffer);

    _numberOfBytesInBuffer = bytesRead;

    ChangeStateAsync(true);
}
