// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "DnsExchangeOp.h"

/*static*/
void DnsExchangeOp::Create(
    __out DnsExchangeOp::SPtr& spExchangeOp,
    __in KAllocator& allocator,
    __in IDnsTracer& tracer,
    __in IDnsParser& dnsParser,
    __in INetIoManager& netIoManager,
    __in IUdpListener& udpServer,
    __in IFabricResolve& fabricResolve,
    __in INetworkParams& networkParams,
    __in const DnsServiceParams& params
)
{
    spExchangeOp = _new(TAG, allocator) DnsExchangeOp(tracer, dnsParser, netIoManager, udpServer, fabricResolve, networkParams, params);
    KInvariant(spExchangeOp != nullptr);
}

DnsExchangeOp::DnsExchangeOp(
    __in IDnsTracer& tracer,
    __in IDnsParser& dnsParser,
    __in INetIoManager& netIoManager,
    __in IUdpListener& udpServer,
    __in IFabricResolve& fabricResolve,
    __in INetworkParams& networkParams,
    __in const DnsServiceParams& params
) : _tracer(tracer),
_dnsParser(dnsParser),
_netIoManager(netIoManager),
_udpServer(udpServer),
_fabricResolve(fabricResolve),
_bytesWrittenToBuffer(0),
_params(params)
{

    DnsResolveOp::Create(/*out*/_spDnsResolveOp, GetThisAllocator(), _tracer, _netIoManager, _dnsParser, _fabricResolve, networkParams, _params);
    DnsRemoteQueryOp::Create(/*out*/_spDnsRemoteQueryOp, GetThisAllocator(), _tracer, _netIoManager, _dnsParser, networkParams, _params);
}

DnsExchangeOp::~DnsExchangeOp()
{
    _tracer.Trace(DnsTraceLevel_Noise, "Destructing DnsExchangeOp.");
}

//***************************************
// BEGIN KAsyncContext region
//***************************************

void DnsExchangeOp::OnStart()
{
    ActivateStateMachine();
    ChangeStateAsync(true);
}

void DnsExchangeOp::OnCancel()
{
    _tracer.Trace(DnsTraceLevel_Info, "DnsExchangeOp OnCancel called.");
    TerminateAsync();
}

void DnsExchangeOp::OnCompleted()
{
    _tracer.Trace(DnsTraceLevel_Info, "DnsExchangeOp OnCompleted Called.");
}

void DnsExchangeOp::OnReuse()
{
    _tracer.Trace(DnsTraceLevel_Noise, "DnsExchangeOp OnReuse called.");
}

//***************************************
// END KAsyncContext region
//***************************************




//***************************************
// BEGIN StateMachine region
//***************************************

void DnsExchangeOp::OnBeforeStateChange(
    __in LPCWSTR fromState,
    __in LPCWSTR toState
)
{
    _tracer.Trace(DnsTraceLevel_Noise, 
        "DnsExchangeOp activityId {0} change state {1} => {2}",
        WSTR(_activityId), WSTR(fromState), WSTR(toState));
}

void DnsExchangeOp::OnStateEnter_Start()
{
    _spDnsResolveOp->Reuse();
    _spDnsRemoteQueryOp->Reuse();
}

void DnsExchangeOp::OnStateEnter_ReadQuestion()
{
    _spAddressFrom = nullptr;
    DNS::CreateSocketAddress(/*out*/_spAddressFrom, GetThisAllocator());

    AcquireActivities();
    UdpReadOpCompletedCallback readOpCallback(this, &DnsExchangeOp::OnUdpReadCompleted);
    _udpServer.ReadAsync(*_spBuffer, *_spAddressFrom, readOpCallback, INFINITE);
}

void DnsExchangeOp::OnStateEnter_ReadQuestionSucceeded()
{
    //Tracing valid resolve requests received by the DNS service
    _tracer.TraceDnsExchangeOpReadQuestion(1);

    ChangeStateAsync(true);
}

void DnsExchangeOp::OnStateEnter_ReadQuestionFailed()
{
    // Check if this looks like a valid DNS request that failed to be parsed for some reason.
    // If the header looks like the DNS header, send back the response.
    // Otherwise, drop the message.
    USHORT id, flags, qCount, aCount, nsCount, adCount;
    bool fSuccess = _dnsParser.GetHeader(/*out*/id, /*out*/flags, /*out*/qCount, /*out*/aCount,
        /*out*/nsCount, /*out*/adCount, *_spBuffer, _bytesWrittenToBuffer);

    // Valid DNS request has to have at least one question, no answers and the response flag must not be set.
    if (fSuccess)
    {
        if ((qCount == 0) || (aCount != 0) || DnsFlags::IsFlagSet(flags, DnsFlags::RESPONSE))
        {
            fSuccess = false;
        }
    }

    ChangeStateAsync(fSuccess);
}

void DnsExchangeOp::OnStateEnter_FabricResolve()
{
    DnsResolveOp::DnsResolveCallback resolveCallback(this, &DnsExchangeOp::OnDnsResolveCompleted);
    _spDnsResolveOp->StartResolve(this/*parent*/, _activityId, *_spMessage, resolveCallback);
}

void DnsExchangeOp::OnStateEnter_FabricResolveSucceeded()
{
    //Tracing requests successfully resolved by naming
    _tracer.TraceDnsExchangeOpFabricResolve(1);

    IDnsMessage& message = *_spMessage;

    USHORT flags = message.Flags();
    DnsFlags::SetResponseCode(/*out*/flags, DnsFlags::RC_NOERROR);
    DnsFlags::SetFlag(/*out*/flags, DnsFlags::AUTHORITY);
    message.SetFlags(flags);

    ChangeStateAsync(true);
}

void DnsExchangeOp::OnStateEnter_FabricResolveFailed()
{
    ChangeStateAsync(true);
}

void DnsExchangeOp::OnStateEnter_IsRemoteResolveEnabled()
{
    ChangeStateAsync(_params.IsRecursiveQueryEnabled);
}

void DnsExchangeOp::OnStateEnter_RemoteResolve()
{
    IDnsMessage& message = *_spMessage;

    USHORT flags = message.Flags();
    DnsFlags::UnsetFlag(/*out*/flags, DnsFlags::RESPONSE);
    DnsFlags::SetFlag(/*out*/flags, DnsFlags::RECURSION_DESIRED);
    DnsFlags::SetResponseCode(/*out*/flags, DnsFlags::RC_NOERROR);
    message.SetFlags(flags);

    DnsRemoteQueryOp::DnsRemoteQueryOpCallback callback(this, &DnsExchangeOp::OnDnsRemoteQueryCompleted);
    _spDnsRemoteQueryOp->StartRemoteQuery(this/*parent*/, _activityId, *_spBuffer, message, callback);
}

void DnsExchangeOp::OnStateEnter_RemoteResolveSucceeded()
{
    //Tracing requests resolved by public DNS
    _tracer.TraceDnsExchangeOpRemoteResolve(1);

    USHORT id, flags, qCount, aCount, nsCount, adCount;
    bool fSuccess = _dnsParser.GetHeader(/*out*/id, /*out*/flags, /*out*/qCount, /*out*/aCount,
        /*out*/nsCount, /*out*/adCount, *_spBuffer, _bytesWrittenToBuffer);

    if (fSuccess)
    {
        IDnsMessage& message = *_spMessage;
        message.SetFlags(flags);

        KString::SPtr spMessageStr = message.ToString();

        _tracer.Trace(DnsTraceLevel_Info,
            "DnsExchangeOp activityId {0}, processed query recursive, answer count {1}, message (not showing answers) {2}",
            WSTR(_activityId), (ULONG)aCount, WSTR(*spMessageStr));
    }

    ChangeStateAsync(true);
}

void DnsExchangeOp::OnStateEnter_RemoteResolveFailed()
{
    ChangeStateAsync(true);
}

void DnsExchangeOp::OnStateEnter_SerializeFabricAnswer()
{
    IDnsMessage& message = *_spMessage;

    USHORT flags = message.Flags();
    DnsFlags::SetFlag(/*out*/flags, DnsFlags::RESPONSE);
    _params.IsRecursiveQueryEnabled ? DnsFlags::SetFlag(/*out*/flags, DnsFlags::RECURSION_AVAILABLE) :
        DnsFlags::UnsetFlag(/*out*/flags, DnsFlags::RECURSION_AVAILABLE);
    message.SetFlags(flags);

    KString::SPtr spMessageStr = message.ToString();

    bool fSuccess = _dnsParser.Serialize(*_spBuffer, /*out*/_bytesWrittenToBuffer, message, _params.TimeToLiveInSeconds);
    if (!fSuccess)
    {
        _tracer.Trace(DnsTraceLevel_Warning,
            "DnsExchangeOp activityId {0}, failed to serialize answer for dns query {1}",
            WSTR(_activityId), WSTR(*spMessageStr));
    }
    else
    {
        _tracer.Trace(DnsTraceLevel_Info,
            "DnsExchangeOp activityId {0}, processed query {1}",
            WSTR(_activityId), WSTR(*spMessageStr));
    }

    ChangeStateAsync(fSuccess);
}

void DnsExchangeOp::OnStateEnter_WriteAnswers()
{
    AcquireActivities();
    UdpWriteOpCompletedCallback callback(this, &DnsExchangeOp::OnUdpWriteCompleted);
    _udpServer.WriteAsync(*_spBuffer, _bytesWrittenToBuffer, *_spAddressFrom, callback);
}

void DnsExchangeOp::OnStateEnter_CreateInternalErrorAnswer()
{
    IDnsMessage& requestMessage = *_spMessage;

    USHORT flags = requestMessage.Flags();
    DnsFlags::SetFlag(/*out*/flags, DnsFlags::RESPONSE);
    _params.IsRecursiveQueryEnabled ? DnsFlags::SetFlag(/*out*/flags, DnsFlags::RECURSION_AVAILABLE) :
        DnsFlags::UnsetFlag(/*out*/flags, DnsFlags::RECURSION_AVAILABLE);
    DnsFlags::UnsetFlag(/*out*/flags, DnsFlags::TRUNCATION);
    DnsFlags::SetResponseCode(/*out*/flags, DnsFlags::RC_SERVFAIL);

    requestMessage.SetFlags(flags);

    ChangeStateAsync(true);
}

void DnsExchangeOp::OnStateEnter_CreateBadRequestAnswer()
{
    USHORT id, flags, qCount, aCount, nsCount, adCount;
    if (!_dnsParser.GetHeader(/*out*/id, /*out*/flags, /*out*/qCount, /*out*/aCount, /*out*/nsCount, /*out*/adCount,
        *_spBuffer, _bytesWrittenToBuffer))
    {
        KInvariant(false);
    }

    DnsFlags::SetFlag(/*out*/flags, DnsFlags::RESPONSE);
    DnsFlags::UnsetFlag(/*out*/flags, DnsFlags::TRUNCATION);
    DnsFlags::SetResponseCode(/*out*/flags, DnsFlags::RC_FORMERR);

    _dnsParser.SetHeader(*_spBuffer, id, flags, qCount, aCount, nsCount, adCount);

    _tracer.Trace(DnsTraceLevel_Info,
        "DnsExchangeOp activityId {0}, sent FORMERR answer.", WSTR(_activityId));

    ChangeStateAsync(true);
}

void DnsExchangeOp::OnStateEnter_DropMessage()
{
    _tracer.Trace(DnsTraceLevel_Info,
        "DnsExchangeOp activityId {0}, dropped query",
        WSTR(_activityId));

    ChangeStateAsync(true);
}

void DnsExchangeOp::OnStateEnter_End()
{
    DeactivateStateMachine();

    _spDnsResolveOp->Cancel();
    _spDnsRemoteQueryOp->Cancel();

    Complete(STATUS_SUCCESS);
}

//***************************************
// END StateMachine region
//***************************************




void DnsExchangeOp::StartExchange(
    __in_opt KAsyncContextBase* const parent,
    __in_opt KAsyncContextBase::CompletionCallback callbackPtr
)
{
    KGuid aid;
    aid.CreateNew();

    _activityId.FromGUID(aid);
    _activityId.SetNullTerminator();

    ULONG bufferSizeInBytes = __min(MAXUSHORT - 1, _params.MaxMessageSizeInKB * 1024);
    if (STATUS_SUCCESS != KBuffer::Create(bufferSizeInBytes, /*out*/_spBuffer, GetThisAllocator()))
    {
        _tracer.Trace(DnsTraceLevel_Error, "Failed to allocate buffer");
        KInvariant(false);
    }

    _tracer.Trace(DnsTraceLevel_Noise, "New DnsExchangeOp activityId {0} bufferSize {1} IsRecursiveQueryEnabled {2}",
        WSTR(_activityId), bufferSizeInBytes, (BOOLEAN)_params.IsRecursiveQueryEnabled);

    Start(parent, callbackPtr);
}

void DnsExchangeOp::OnUdpReadCompleted(
    __in NTSTATUS status,
    __in KBuffer& buffer,
    __in ULONG bytesRead
)
{
    IDnsMessage::SPtr spMessage;

    if (bytesRead == 0 || status != STATUS_SUCCESS)
    {
        _tracer.Trace(DnsTraceLevel_Info,
            "DnsExchangeOp activityId {0} UDP read failed, status {1}, bytesRead {2}",
            WSTR(_activityId), status, bytesRead);

        ChangeStateAsync(false);
    }
    else if (!_dnsParser.Deserialize(/*out*/spMessage, buffer, bytesRead))
    {

        _tracer.Trace(DnsTraceLevel_Info,
            "DnsExchangeOp activityId {0} UDP read failed, unable to deserialize the message, status {1}, bytesRead {2}",
            WSTR(_activityId), status, bytesRead);

        ChangeStateAsync(false);
    }
    else
    {
        _spMessage = spMessage;

        _tracer.Trace(DnsTraceLevel_Noise,
            "DnsExchangeOp activityId {0} UDP read succeeded, status {1}, bytesRead {2}",
            WSTR(_activityId), status, bytesRead);

        ChangeStateAsync(true);
    }

    ReleaseActivities();
}

void DnsExchangeOp::OnDnsResolveCompleted(
    __in IDnsMessage& message
)
{
    USHORT flags = message.Flags();
    bool fSuccess = (DnsFlags::GetResponseCode(flags) == DnsFlags::RC_NOERROR);

    ChangeStateAsync(fSuccess);
}

void DnsExchangeOp::OnDnsRemoteQueryCompleted(
    __in KBuffer& buffer,
    __in ULONG numberOfBytesInBuffer,
    __in_opt PVOID context
)
{
    UNREFERENCED_PARAMETER(buffer);
    UNREFERENCED_PARAMETER(context);
    _bytesWrittenToBuffer = numberOfBytesInBuffer;
    (_bytesWrittenToBuffer > 0) ? ChangeStateAsync(true) : ChangeStateAsync(false);
}

void DnsExchangeOp::OnUdpWriteCompleted(
    __in NTSTATUS status,
    __in ULONG bytesSent
)
{
    if ((status != STATUS_SUCCESS) || (bytesSent == 0))
    {
        _tracer.Trace(DnsTraceLevel_Info, 
            "DnsExchangeOp activityId {0} UDP write completed, status {1}, bytesSent {2}",
            WSTR(_activityId), status, bytesSent);
    }

    ChangeStateAsync(true);

    ReleaseActivities();
}
