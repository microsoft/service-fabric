// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "DnsResolveOp.h"

/*static*/
void DnsResolveOp::Create(
    __out DnsResolveOp::SPtr& spResolveOp,
    __in KAllocator& allocator,
    __in IDnsTracer& tracer,
    __in INetIoManager& netIoManager,
    __in IDnsParser& dnsParser,
    __in IFabricResolve& fabricResolve,
    __in INetworkParams& networkParams,
    __in const DnsServiceParams& params
)
{
    spResolveOp = _new(TAG, allocator) DnsResolveOp(tracer, netIoManager,dnsParser, fabricResolve, networkParams, params);
    KInvariant(spResolveOp != nullptr);
}

DnsResolveOp::DnsResolveOp(
    __in IDnsTracer& tracer,
    __in INetIoManager& netIoManager,
    __in IDnsParser& dnsParser,
    __in IFabricResolve& fabricResolve,
    __in INetworkParams& networkParams,
    __in const DnsServiceParams& params
) : _tracer(tracer),
_params(params),
_netIoManager(netIoManager),
_dnsParser(dnsParser),
_fabricResolve(fabricResolve),
_networkParams(networkParams),
_arrFabricOps(GetThisAllocator()),
_arrFqdnOps(GetThisAllocator()),
_pendingFabricResolveOps(0),
_pendingFqdnOps(0),
_htAnswers(256, K_DefaultHashFunction, CompareKString, GetThisAllocator())
{
}

DnsResolveOp::~DnsResolveOp()
{
    _tracer.Trace(DnsTraceLevel_Noise, "Destructing DnsResolveOp.");
}

//***************************************
// BEGIN KAsyncContext region
//***************************************

void DnsResolveOp::OnCompleted()
{
    _tracer.Trace(DnsTraceLevel_Noise, "DnsResolveOp OnCompleted Called.");
    _resolveCallback(*_spMessage);
}

void DnsResolveOp::OnReuse()
{
    _tracer.Trace(DnsTraceLevel_Noise, "DnsResolveOp OnReuse Called.");

    _pendingFabricResolveOps = 0;
    _arrFabricOps.Clear();

    _pendingFqdnOps = 0;
    _arrFqdnOps.Clear();

    _htAnswers.Clear();
}

void DnsResolveOp::OnCancel()
{
    _tracer.Trace(DnsTraceLevel_Info,
        "DnsResolveOp activityId {0}, cancelled.",
        WSTR(_activityId));

    TerminateAsync();
}

void DnsResolveOp::OnStart()
{
    ActivateStateMachine();

    ChangeStateAsync(true);
}

//***************************************
// BEGIN StateMachine region
//***************************************

void DnsResolveOp::OnBeforeStateChange(
    __in LPCWSTR fromState,
    __in LPCWSTR toState
)
{
    _tracer.Trace(DnsTraceLevel_Noise,
        "DnsResolveOp activityId {0} change state {1} => {2}",
        WSTR(_activityId), WSTR(fromState), WSTR(toState));
}

void DnsResolveOp::OnStateEnter_Start()
{
    KInvariant(_pendingFabricResolveOps == 0);
    KInvariant(_arrFabricOps.IsEmpty());
    KInvariant(_pendingFqdnOps == 0);
    KInvariant(_arrFqdnOps.IsEmpty());
    KInvariant(_htAnswers.Count() == 0);
}

void DnsResolveOp::OnStateEnter_StartFabricResolveOps()
{
    FabricResolveCompletedCallback fabricResolveCompletedCallback(this, &DnsResolveOp::OnFabricResolveCompleted);
    USHORT flags = _spMessage->Flags();
    DnsFlags::SetResponseCode(flags, DnsFlags::RC_NXDOMAIN);
    _spMessage->SetFlags(flags);

    KArray<IDnsRecord::SPtr>& arrQuestions = _spMessage->Questions();

    _tracer.Trace(DnsTraceLevel_Noise,
        "DnsResolveOp activityId {0}, starting. Question count {1}",
        WSTR(_activityId), arrQuestions.Count());

    // This lock is needed to ensure that all fabric resolve ops are started before
    // results returned by the individual fabric resolve op are processed.
    // Otherwise, it could happen that the first op completes before the second op is started
    // which would cause the _pendingFabricResolveOps count to drop to zero, which would lead 
    // to premature completion of this state machine.
    K_LOCK_BLOCK(_lockFabricResolveOps)
    {
        for (ULONG i = 0; i < arrQuestions.Count(); i++)
        {
            IDnsRecord& question = *arrQuestions[i];

            IFabricResolveOp::SPtr spFabricResolveOp = _fabricResolve.CreateResolveOp(
                _params.FabricQueryTimeoutInSeconds,
                _params.PartitionPrefix,
                _params.PartitionSuffix,
                _params.EnablePartitionedQuery
            );
            if (STATUS_SUCCESS != _arrFabricOps.Append(spFabricResolveOp))
            {
                _tracer.Trace(DnsTraceLevel_Error,
                    "DnsResolveOp activityId {0}, failed to append item to the array. Fatal error.",
                    WSTR(_activityId));
                KInvariant(false);
            }

            _pendingFabricResolveOps++;
            spFabricResolveOp->StartResolve(this/*parent*/, _activityId, question, fabricResolveCompletedCallback);
        }

        ChangeStateAsync(_pendingFabricResolveOps > 0);
    }
}

void DnsResolveOp::OnStateEnter_WaitForFabricResolveOpsToFinish()
{
    // Just wait here, the actual transition will be triggered when
    // _pendingFabricResolveOps drops to zero.
}

void DnsResolveOp::OnStateEnter_ZeroFabricResolveOpsStarted()
{
    ChangeStateAsync(true);
}

void DnsResolveOp::OnStateEnter_ReceivedValidResponse()
{
    ChangeStateAsync(true);
}

void DnsResolveOp::OnStateEnter_ReceivedEmptyResponse()
{
    ChangeStateAsync(true);
}

void DnsResolveOp::OnStateEnter_StartResolveHostnameOps()
{
    DnsResolveFqdnOp::DnsResolveFqdnOpCallback callback(this, &DnsResolveOp::OnDnsResolveFqdnCompleted);

    // This lock is needed to ensure that all fabric fqdn ops are started before
    // results returned by the individual fqdn resolve op are processed.
    // Otherwise, it could happen that the first op completes before the second op is started
    // which would cause the _pendingFqdnOps count to drop to zero, which would lead 
    // to premature completion of this state machine.
    K_LOCK_BLOCK(_lockFqdnOps)
    {
        KString::SPtr spQuestionStr;
        KSharedArray<DnsAnswer::SPtr>::SPtr spAnswers;
        _htAnswers.Reset();
        while (_htAnswers.Next(/*out*/spQuestionStr, /*out*/spAnswers) == STATUS_SUCCESS)
        {
            KArray<DnsAnswer::SPtr>& arr = *spAnswers;
            for (ULONG i = 0; i < arr.Count(); i++)
            {
                DnsAnswer::SPtr spAnswer = arr[i];
                if (spAnswer->IsFqdnHost())
                {
                    DnsResolveFqdnOp::SPtr spQuery;
                    DnsResolveFqdnOp::Create(/*out*/spQuery, GetThisAllocator(), _tracer, _netIoManager, _dnsParser, _networkParams, _params);

                    if (STATUS_SUCCESS != _arrFqdnOps.Append(spQuery))
                    {
                        KInvariant(false);
                    }

                    spQuery->StartResolve(this/*parent*/, _activityId, spAnswer->Host(), callback, spAnswer.RawPtr());
                    _pendingFqdnOps++;
                }
            }
        }

        ChangeStateAsync(_pendingFqdnOps > 0);
    }
}

void DnsResolveOp::OnStateEnter_WaitForResolveHostnameOpsToFinish()
{
    // Just wait here, the actual transition will be triggered when
    // _pendingFqdnOps drops to zero.
}

void DnsResolveOp::OnStateEnter_ZeroResolveHostnameOpsStarted()
{
    ChangeStateAsync(true);
}

void DnsResolveOp::OnStateEnter_ResolveHostnamesFinished()
{
    ChangeStateAsync(true);
}

void DnsResolveOp::OnStateEnter_AggregateResults()
{
    const USHORT ResultLimit = 8;

    USHORT flags = _spMessage->Flags();
    if (_htAnswers.Count() > 0)
    {
        DnsFlags::SetResponseCode(flags, DnsFlags::RC_NOERROR);
    }
    else
    {
        DnsFlags::SetResponseCode(flags, DnsFlags::RC_NXDOMAIN);
    }
    _spMessage->SetFlags(flags);

    KArray<IDnsRecord::SPtr>& arrAnswers = _spMessage->Answers();
    KString::SPtr spQuestionStr;
    KSharedArray<DnsAnswer::SPtr>::SPtr spAnswers;
    _htAnswers.Reset();
    while (_htAnswers.Next(/*out*/spQuestionStr, /*out*/spAnswers) == STATUS_SUCCESS)
    {
        // Hashtables needed to avoid duplicate answers
        KHashTable<KString::SPtr, bool> htTxt(16, K_DefaultHashFunction, CompareKString, GetThisAllocator());
        KHashTable<ULONG, bool> htA(16, K_DefaultHashFunction, GetThisAllocator());
        KHashTable<ULONG, bool> htSrv(16, K_DefaultHashFunction, GetThisAllocator());

        KArray<DnsAnswer::SPtr>& arr = *spAnswers;
        for (ULONG i = 0; i < arr.Count(); i++)
        {
            DnsAnswer& answer = *arr[i];

            IDnsRecord& question = answer.Question();
            KString& strRawUri = answer.RawAnswer();

            if ((question.Type() != DnsRecordTypeTxt) &&
                (question.Type() != DnsRecordTypeA) &&
                (question.Type() != DnsRecordTypeSrv) &&
                (question.Type() != DnsRecordTypeAll))
            {
                continue;
            }

            if ((question.Type() == DnsRecordTypeTxt) || (question.Type() == DnsRecordTypeAll))
            {
                KString::SPtr spKey(&strRawUri);
                if (!htTxt.ContainsKey(spKey) && htTxt.Count() < ResultLimit)
                {
                    IDnsRecord::SPtr spRecord = _dnsParser.CreateRecordTxt(question, strRawUri);

                    if (STATUS_SUCCESS != arrAnswers.Append(spRecord.RawPtr()))
                    {
                        _tracer.Trace(DnsTraceLevel_Error, "DnsResolveOp Failed to append item to the array");
                        KInvariant(false);
                    }

                    NTSTATUS status = htTxt.Put(spKey, true, TRUE);
                    if (status != STATUS_SUCCESS && status != STATUS_OBJECT_NAME_EXISTS)
                    {
                        _tracer.Trace(DnsTraceLevel_Error, "DnsResolveOp Failed to put item to the hashtable");
                        KInvariant(false);
                    }
                }
            }

            if ((question.Type() == DnsRecordTypeA) || (question.Type() == DnsRecordTypeAll))
            {
                ULONG address = answer.HostIpAddress();
                if (address != INADDR_NONE)
                {
                    if (!htA.ContainsKey(address) && htA.Count() < ResultLimit)
                    {
                        IDnsRecord::SPtr spRecord = _dnsParser.CreateRecordA(question, ntohl(address));

                        if (STATUS_SUCCESS != arrAnswers.Append(spRecord.RawPtr()))
                        {
                            KInvariant(false);
                        }

                        NTSTATUS status = htA.Put(address, true, TRUE);
                        if (status != STATUS_SUCCESS && status != STATUS_OBJECT_NAME_EXISTS)
                        {
                            _tracer.Trace(DnsTraceLevel_Error, "DnsResolveOp Failed to put item to the hashtable");
                            KInvariant(false);
                        }
                    }
                }
            }

            if ((question.Type() == DnsRecordTypeSrv) || (question.Type() == DnsRecordTypeAll))
            {
                ULONG port = answer.Port();
                if (port != 0)
                {
                    if (!htSrv.ContainsKey(port) && htSrv.Count() < ResultLimit)
                    {
                        IDnsRecord::SPtr spRecord = _dnsParser.CreateRecordSrv(
                            question,
                            answer.IsFqdnHost() ? answer.Host() : *spQuestionStr,
                            static_cast<USHORT>(port)
                        );

                        if (STATUS_SUCCESS != arrAnswers.Append(spRecord.RawPtr()))
                        {
                            KInvariant(false);
                        }

                        NTSTATUS status = htSrv.Put(port, true, TRUE);
                        if (status != STATUS_SUCCESS && status != STATUS_OBJECT_NAME_EXISTS)
                        {
                            _tracer.Trace(DnsTraceLevel_Error, "DnsResolveOp Failed to put item to the hashtable");
                            KInvariant(false);
                        }
                    }
                }
            }
        }
    }

    ChangeStateAsync(true);
}

void DnsResolveOp::OnStateEnter_End()
{
    DeactivateStateMachine();

    for (ULONG i = 0; i < _arrFabricOps.Count(); i++)
    {
        _arrFabricOps[i]->CancelResolve();
    }

    for (ULONG i = 0; i < _arrFqdnOps.Count(); i++)
    {
        _arrFqdnOps[i]->Cancel();
    }

    Complete(STATUS_SUCCESS);
}

//***************************************
// END StateMachine region
//***************************************

void DnsResolveOp::StartResolve(
    __in_opt KAsyncContextBase* const parent,
    __in KStringView& activityId,
    __in IDnsMessage& message,
    __in DnsResolveCallback resolveCallback
)
{
    _activityId = activityId;
    _spMessage = &message;
    _resolveCallback = resolveCallback;

    Start(parent, nullptr/*callback*/);
}

void DnsResolveOp::OnFabricResolveCompleted(
    __in IDnsRecord& question,
    __in KArray<KString::SPtr>& arrResults
)
{
    const ULONG MaxResults = 16;

    K_LOCK_BLOCK(_lockFabricResolveOps)
    {
        // Shuffle the results so we don't allways return the same order.
        // The goal is to help with load balancing.
        Shuffle(arrResults);

        if (!arrResults.IsEmpty())
        {
            KString::SPtr spQuestion(&question.Name());
            KSharedArray<DnsAnswer::SPtr>::SPtr spAnswerArray;
            NTSTATUS status = _htAnswers.Get(spQuestion, /*out*/spAnswerArray);
            if (status != STATUS_SUCCESS)
            {
                spAnswerArray = _new(TAG, GetThisAllocator()) KSharedArray<DnsAnswer::SPtr>();
                status = _htAnswers.Put(spQuestion, spAnswerArray, TRUE/*forceUpdate*/);
                if (status != STATUS_SUCCESS)
                {
                    KInvariant(false);
                }
            }

            for (ULONG i = 0; i < arrResults.Count() && i < MaxResults; i++)
            {
                DnsAnswer::SPtr spAnswer;
                DnsAnswer::Create(/*out*/spAnswer, GetThisAllocator(), question, *arrResults[i]);
                if (STATUS_SUCCESS != spAnswerArray->Append(spAnswer))
                {
                    KInvariant(false);
                }
            }
        }

        KInvariant(_pendingFabricResolveOps > 0);
        _pendingFabricResolveOps--;

        if (_pendingFabricResolveOps == 0)
        {
            ChangeStateAsync(_htAnswers.Count() > 0);
        }
    }
}

void DnsResolveOp::OnDnsResolveFqdnCompleted(
    __in_opt IDnsMessage* pMessage,
    __in_opt PVOID context
)
{
    KInvariant(context != nullptr);

    K_LOCK_BLOCK(_lockFqdnOps)
    {
        ULONG address = INADDR_NONE;
        if (pMessage != nullptr)
        {
            KArray<IDnsRecord::SPtr>& arrAnswers = pMessage->Answers();
            for (ULONG i = 0; i < arrAnswers.Count(); i++)
            {
                IDnsRecord& record = *arrAnswers[i];
                if (record.Type() == DnsRecordTypeA)
                {
                    PVOID pValue = record.ValuePtr();
                    if (pValue != nullptr)
                    {
                        address = *static_cast<ULONG*>(pValue);
                        address = htonl(address);
                        break;
                    }
                }
            }
        }

        DnsAnswer& answer = *reinterpret_cast<DnsAnswer*>(context);
        answer.SetHostIpAddress(address);

        KInvariant(_pendingFqdnOps > 0);
        _pendingFqdnOps--;

        if (_pendingFqdnOps == 0)
        {
            ChangeStateAsync(true);
        }
    }
}

/*static*/
void DnsResolveOp::Shuffle(
    __inout KArray<KString::SPtr>& arr
)
{
    ULONG count = arr.Count();
    if (count < 2)
    {
        return;
    }

    for (ULONG i = count - 1; i > 0; i--)
    {
        // Pick a random index from [0, i]
        int j = rand() % (i + 1);

        // Swap i and j
        KString::SPtr spTmp = arr[i];
        arr[i] = arr[j];
        arr[j] = spTmp;
    }
}

/*static*/
void DnsResolveOp::DnsAnswer::Create(
    __out DnsResolveOp::DnsAnswer::SPtr& spAnswer,
    __in KAllocator& allocator,
    __in IDnsRecord& question,
    __in KString& rawAnswer
)
{
    spAnswer = _new(TAG, allocator) DnsResolveOp::DnsAnswer(question, rawAnswer);
    KInvariant(spAnswer != nullptr);
}

DnsResolveOp::DnsAnswer::DnsAnswer(
    __in IDnsRecord& question,
    __in KString& rawAnswer
) : _fIsFqdnHost(false),
_spQuestion(&question),
_spRawAnswer(&rawAnswer),
_hostIpAddress(INADDR_NONE),
_port(0)
{
    KStringView strHost;
    FindHostAndPort(*_spRawAnswer, /*out*/strHost, /*out*/_port);
    if (!strHost.IsEmpty())
    {
        _spHostStr = KString::Create(strHost, GetThisAllocator());

        ULONG address = INADDR_NONE;
        KStringView strLocalhost(L"localhost");
        KStringView strLocalhostIP(L"127.0.0.1");
        KStringView strConvert = (strLocalhost.CompareNoCase(*_spHostStr) == 0) ? strLocalhostIP : *_spHostStr;

        IN_ADDR addr;
#if !defined(PLATFORM_UNIX)
        int err = InetPton(AF_INET, strConvert, &addr);
        if (1 == err)
        {
            address = addr.S_un.S_addr;
        }
#else
        CHAR tempAscii[INET_ADDRSTRLEN];
        ULONG length = __min(strConvert.Length(), ARRAYSIZE(tempAscii) - 1);
        strConvert.CopyToAnsi(tempAscii, length);
        tempAscii[length] = '\0';
        int err = inet_pton(AF_INET, tempAscii, &addr);
        if (1 == err)
        {
            address = addr.s_addr;
        }
#endif

        _hostIpAddress = address;
    }
    else
    {
        _spHostStr = KString::Create(L" ", GetThisAllocator());
    }

    _fIsFqdnHost = (!_spHostStr->IsEmpty() && (_hostIpAddress == INADDR_NONE));

    KInvariant(_spHostStr != nullptr);
}

DnsResolveOp::DnsAnswer::~DnsAnswer()
{
}

/*static*/
void DnsResolveOp::DnsAnswer::FindHostAndPort(
    __in KStringView strRawUri,
    __out KStringView& strHost,
    __out ULONG& port
)
{
    KStringView strDS(L"//");
    ULONG hostStartPos = 0;
    if (!strRawUri.Search(strDS, /*out*/hostStartPos))
    {
        hostStartPos = 0;
    }
    else
    {
        hostStartPos += strDS.Length();
    }

    ULONG hostEndPos = 0;
    KStringView strC(L":");
    if (!strRawUri.Search(strC, /*out*/hostEndPos, hostStartPos))
    {
        if (!strRawUri.Search(KStringView(L"/"), /*out*/hostEndPos, hostStartPos))
        {
            hostEndPos = strRawUri.Length();
        }
    }
    else
    {
        // Port probably exists
        //
        ULONG portStartPos = hostEndPos + strC.Length();
        ULONG portEndPos = portStartPos;
        for (portEndPos = portStartPos; portEndPos < strRawUri.Length(); portEndPos++)
        {
            if (strRawUri[portEndPos] < '0' || strRawUri[portEndPos] > '9')
            {
                break;
            }
        }

        KStringView strPort = strRawUri.SubString(portStartPos, portEndPos - portStartPos);
        if (!strPort.ToULONG(/*out*/port))
        {
            port = 0;
        }
    }

    strHost = strRawUri.SubString(hostStartPos, hostEndPos - hostStartPos);
}
