// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "DnsResolveFqdnOp.h"

/*static*/
void DnsResolveFqdnOp::Create(
    __out DnsResolveFqdnOp::SPtr& spResolveOp,
    __in KAllocator& allocator,
    __in IDnsTracer& tracer,
    __in INetIoManager& netIoManager,
    __in IDnsParser& dnsParser,
    __in INetworkParams& networkParams,
    __in const DnsServiceParams& params
)
{
    spResolveOp = _new(TAG, allocator) DnsResolveFqdnOp(tracer, netIoManager, dnsParser, networkParams, params);
    KInvariant(spResolveOp != nullptr);
}

DnsResolveFqdnOp::DnsResolveFqdnOp(
    __in IDnsTracer& tracer,
    __in INetIoManager& netIoManager,
    __in IDnsParser& dnsParser,
    __in INetworkParams& networkParams,
    __in const DnsServiceParams& params
) : _tracer(tracer),
_params(params),
_netIoManager(netIoManager),
_dnsParser(dnsParser),
_networkParams(networkParams),
_fIsFqdnRelative(false),
_currentSuffixIndex(0),
_context(nullptr)
{
}

DnsResolveFqdnOp::~DnsResolveFqdnOp()
{
    _tracer.Trace(DnsTraceLevel_Noise, "Destructing DnsResolveFqdnOp.");
}

//***************************************
// BEGIN KAsyncContext region
//***************************************

void DnsResolveFqdnOp::OnCompleted()
{
    _tracer.Trace(DnsTraceLevel_Info, "DnsResolveFqdnOp OnCompleted called.");
    _resolveCallback(_spMessage.RawPtr(), _context);
}

void DnsResolveFqdnOp::OnCancel()
{
    _tracer.Trace(DnsTraceLevel_Info,
        "DnsResolveFqdnOp activityId {0}, cancelled.",
        WSTR(_activityId));

    TerminateAsync();
}

void DnsResolveFqdnOp::OnStart()
{
    ActivateStateMachine();

    ChangeStateAsync(true);
}

//***************************************
// BEGIN StateMachine region
//***************************************

void DnsResolveFqdnOp::OnBeforeStateChange(
    __in LPCWSTR fromState,
    __in LPCWSTR toState
)
{
    _tracer.Trace(DnsTraceLevel_Noise,
        "DnsResolveFqdnOp activityId {0} change state {1} => {2}",
        WSTR(_activityId), WSTR(fromState), WSTR(toState));
}

void DnsResolveFqdnOp::OnStateEnter_Start()
{
    _currentSuffixIndex = 0;

    ULONG ndots = 0;
    KString& strQuestion = *_spQuestionStr;
    for (ULONG i = 0; i < strQuestion.Length(); i++)
    {
        if (strQuestion[i] == L'.')
        {
            ndots++;
        }
    }

    _fIsFqdnRelative = (ndots < _params.NDots);

    _spModifiedQuestionStr = KString::Create(*_spQuestionStr, GetThisAllocator());
    if (_spModifiedQuestionStr == nullptr)
    {
        KInvariant(false);
    }
}

void DnsResolveFqdnOp::OnStateEnter_IsFqdnRelative()
{
    ChangeStateAsync(_fIsFqdnRelative);
}

void DnsResolveFqdnOp::OnStateEnter_AppendSuffix()
{
    // It is OK not to append anything, since we still do want to send
    // DNS label as the question to the DNS server if no suffixes are defined.

    if (!_spModifiedQuestionStr->CopyFrom(*_spQuestionStr))
    {
        KInvariant(false);
    }

    const KArray<KString::SPtr>& arrSearchList = _networkParams.DnsSearchList();
    if (_currentSuffixIndex < arrSearchList.Count())
    {
        if (!_spModifiedQuestionStr->Concat(L"."))
        {
            KInvariant(false);
        }

        if (!_spModifiedQuestionStr->Concat(*arrSearchList[_currentSuffixIndex]))
        {
            KInvariant(false);
        }

        _currentSuffixIndex++;
    }

    _spModifiedQuestionStr->SetNullTerminator();

    ChangeStateAsync(true);
}

void DnsResolveFqdnOp::OnStateEnter_IsFqdnRelativeAndHasMoreSuffixes()
{
    bool fSuccess = _fIsFqdnRelative;
    if (fSuccess)
    {
        const KArray<KString::SPtr>& arrSearchList = _networkParams.DnsSearchList();
        fSuccess = (_currentSuffixIndex < arrSearchList.Count());
    }

    ChangeStateAsync(fSuccess);
}

void DnsResolveFqdnOp::OnStateEnter_SendQuery()
{
    DnsRemoteQueryOp::DnsRemoteQueryOpCallback callback(this, &DnsResolveFqdnOp::OnDnsRemoteQueryCompleted);

    DnsRemoteQueryOp::Create(/*out*/_spQuery, GetThisAllocator(), _tracer, _netIoManager, _dnsParser, _networkParams, _params);

    ULONG bufferSizeInBytes = __min(MAXUSHORT - 1, _params.MaxMessageSizeInKB * 1024);
    KBuffer::SPtr spBuffer;
    if (STATUS_SUCCESS != KBuffer::Create(bufferSizeInBytes, /*out*/spBuffer, GetThisAllocator()))
    {
        _tracer.Trace(DnsTraceLevel_Error, "Failed to allocate buffer");
        KInvariant(false);
    }

    // Schedule state change before the start begins to be sure that
    // response is going to be received by the next state
    ChangeStateAsync(true);

    USHORT id = rand() % MAXUSHORT;
    IDnsMessage::SPtr spMessage = _dnsParser.CreateQuestionMessage(id, *_spModifiedQuestionStr, DnsRecordTypeA);

    _spQuery->StartRemoteQuery(this/*parent*/, _activityId, *spBuffer, *spMessage, callback);
}

void DnsResolveFqdnOp::OnStateEnter_WaitForResponse()
{
    // Just wait here, state is changed in the DnsRemoteQueryOpCallback
}

void DnsResolveFqdnOp::OnStateEnter_ReceivedValidResponse()
{
    ChangeStateAsync(true);
}

void DnsResolveFqdnOp::OnStateEnter_ReceivedEmptyResponse()
{
    ChangeStateAsync(true);
}

void DnsResolveFqdnOp::OnStateEnter_ResolveSucceeded()
{
    ChangeStateAsync(true);
}

void DnsResolveFqdnOp::OnStateEnter_ResolveFailed()
{
    ChangeStateAsync(true);
}

void DnsResolveFqdnOp::OnStateEnter_End()
{
    DeactivateStateMachine();

    Complete(STATUS_SUCCESS);
}


//***************************************
// END StateMachine region
//***************************************


void DnsResolveFqdnOp::StartResolve(
    __in_opt KAsyncContextBase* const parent,
    __in KStringView& activityId,
    __in KString& strQuestion,
    __in DnsResolveFqdnOpCallback resolveCallback,
    __in_opt PVOID context
)
{
    _activityId = activityId;
    _spQuestionStr = &strQuestion;
    _resolveCallback = resolveCallback;
    _context = context;

    Start(parent, nullptr/*callback*/);
}

void DnsResolveFqdnOp::OnDnsRemoteQueryCompleted(
    __in KBuffer& buffer,
    __in ULONG numberOfBytesInBuffer,
    __in_opt PVOID context
)
{
    UNREFERENCED_PARAMETER(context);

    bool fSuccess = false;
    if (_dnsParser.Deserialize(/*out*/_spMessage, buffer, numberOfBytesInBuffer))
    {
        if (_spMessage->Answers().Count() > 0)
        {
            fSuccess = true;
        }
    }

    ChangeStateAsync(fSuccess);
}
