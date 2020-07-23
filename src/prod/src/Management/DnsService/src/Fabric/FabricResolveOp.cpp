// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "FabricResolveOp.h"

/*static*/
void FabricResolveOp::Create(
    __out FabricResolveOp::SPtr& spResolveOp,
    __in KAllocator& allocator,
    __in KString& serviceName,
    __in IDnsTracer& tracer,
    __in IFabricData& data,
    __in IDnsCache& cache,
    __in IFabricServiceManagementClient& fabricServiceClient,
    __in IFabricPropertyManagementClient& fabricPropertyClient,
    __in ULONG fabricQueryTimeoutInSeconds,
    __in const KString::SPtr& spStrPartitionPrefix,
    __in const KString::SPtr& spStrPartitionSuffix,
    __in bool fIsPartitionedQueryEnabled
)
{
    spResolveOp = _new(TAG, allocator) FabricResolveOp(serviceName, tracer, data, cache,
        fabricServiceClient, fabricPropertyClient, fabricQueryTimeoutInSeconds,
        spStrPartitionPrefix, spStrPartitionSuffix, fIsPartitionedQueryEnabled);
    KInvariant(spResolveOp != nullptr);
}

FabricResolveOp::FabricResolveOp(
    __in KString& serviceName,
    __in IDnsTracer& tracer,
    __in IFabricData& data,
    __in IDnsCache& cache,
    __in IFabricServiceManagementClient& fabricServiceClient,
    __in IFabricPropertyManagementClient& fabricPropertyClient,
    __in ULONG fabricQueryTimeoutInSeconds,
    __in const KString::SPtr& spStrPartitionPrefix,
    __in const KString::SPtr& spStrPartitionSuffix,
    __in bool fIsPartitionedQueryEnabled
) : _serviceName(serviceName),
_tracer(tracer),
_data(data),
_cache(cache),
_fabricServiceClient(fabricServiceClient),
_fabricPropertyClient(fabricPropertyClient),
_fabricQueryTimeoutInSeconds(fabricQueryTimeoutInSeconds),
_arrResults(GetThisAllocator()),
_nameType(DnsNameTypeUnknown),
_serviceKind(FABRIC_SERVICE_DESCRIPTION_KIND_INVALID),
_partitionKind(FABRIC_PARTITION_SCHEME_INVALID),
_spStrPartitionPrefix(spStrPartitionPrefix),
_spStrPartitionSuffix(spStrPartitionSuffix),
_fIsPartitionedQueryEnabled(fIsPartitionedQueryEnabled)
{
    fabricServiceClient.QueryInterface(
        IID_IInternalFabricServiceManagementClient2,
        _spInternalFabricServiceManagementClient2.VoidInitializationAddress());

    KInvariant(_spInternalFabricServiceManagementClient2 != nullptr);
}

FabricResolveOp::~FabricResolveOp()
{
    _tracer.Trace(DnsTraceLevel_Noise, "Destructing FabricResolveOp.");
}

//***************************************
// BEGIN KAsyncContext region
//***************************************

void FabricResolveOp::OnStart()
{
    ActivateStateMachine();
    ChangeStateAsync(true);
}

void FabricResolveOp::OnCompleted()
{
    _tracer.Trace(DnsTraceLevel_Noise, "FabricResolveOp::OnCompleted called.");
    _callback(*_spQuestion, _arrResults);
}

void FabricResolveOp::OnCancel()
{
    _tracer.Trace(DnsTraceLevel_Noise, "FabricResolveOp::OnCancel called.");
    TerminateAsync();
}

//***************************************
// END KAsyncContext region
//***************************************



//***************************************
// BEGIN StateMachine region
//***************************************

void FabricResolveOp::OnBeforeStateChange(
    __in LPCWSTR fromState,
    __in LPCWSTR toState
)
{
    _tracer.Trace(DnsTraceLevel_Noise,
        "FabricResolveOp activityId {0} change state {1} => {2}",
        WSTR(_activityId), WSTR(fromState), WSTR(toState));
}

void FabricResolveOp::OnStateEnter_Start()
{
}

void FabricResolveOp::OnStateEnter_IsFabricName()
{
    KString& strQuestion = _spQuestion->Name();

    ULONG pos = 0;
    bool fSuccess = !!strQuestion.Search(FabricSchemeName, /*out*/pos);
    if (fSuccess)
    {
        _spFabricName = &_spQuestion->Name();
    }
    ChangeStateAsync(fSuccess);
}

void FabricResolveOp::OnStateEnter_CacheResolve()
{
    _spFabricName = nullptr;
    _nameType = _cache.Read(*_spStrDnsName, /*out*/_spFabricName);

    bool fSuccess = (_nameType == DnsNameTypeFabric);

    ChangeStateAsync(fSuccess);
}

void FabricResolveOp::OnStateEnter_IsUnknownDnsName()
{
    bool fUnknownDnsName = (_nameType == DnsNameTypeUnknown);
    ChangeStateAsync(fUnknownDnsName);
}

void FabricResolveOp::OnStateEnter_MapDnsNameToFabricName()
{
    // First check if DNS name is valid, this check has to be the same as the check in th CM
    // Best to expose this check as part of the shared .lib
    // Note that the check is purely perf optimization. If the name is not allowed,
    // the get property request is not sent to Naming service.

    _timeGetProperty = KDateTime::Now();

    KString& strQuestion = _spQuestion->Name();

    DNSNAME_STATUS status = DNS::IsDnsNameValid(static_cast<LPCWSTR>(strQuestion));
    if ((status != DNS::DNSNAME_VALID) && (status != DNS::DNSNAME_NON_RFC_NAME))
    {
        ChangeStateAsync(false);
    }

    FabricAsyncOperationCallback callback(this, &FabricResolveOp::OnFabricGetPropertyCompleted);
    OperationCallback::Create(/*out*/_spSync, GetThisAllocator(), callback);

    HRESULT hr = _fabricPropertyClient.BeginGetProperty(
        _serviceName,
        static_cast<LPCWSTR>(*_spStrDnsName),
        _fabricQueryTimeoutInSeconds * 1000,
        static_cast<IFabricAsyncOperationCallback*>(_spSync.RawPtr()),
        _spResolveContext.InitializationAddress()
    );

    if (hr != S_OK)
    {
        _tracer.Trace(DnsTraceLevel_Warning,
            "FabricResolveOp activityId {0}, question {1}, DNS name {2}, FabricClient::BeginGetProperty failed with error {3}",
            WSTR(_activityId), WSTR(strQuestion), WSTR(*_spStrDnsName), hr);

        ChangeStateAsync(false);
    }
}

void FabricResolveOp::OnStateEnter_MapDnsNameToFabricNameSucceeded()
{
    KDuration duration = KDateTime::Now() - _timeGetProperty;

    _tracer.Trace(DnsTraceLevel_Noise,
        "FabricResolveOp activityId {0}, GetProperty succeeded, dns name {1}, duration {2}ms",
        WSTR(_activityId), WSTR(*_spStrDnsName), (LONG)duration.Milliseconds());

    KString& strQuestion = _spQuestion->Name();
    _cache.Put(strQuestion, *_spFabricName);

    ChangeStateAsync(true);
}

void FabricResolveOp::OnStateEnter_MapDnsNameToFabricNameFailed()
{
    KDuration duration = KDateTime::Now() - _timeGetProperty;

    _tracer.Trace(DnsTraceLevel_Noise,
        "FabricResolveOp activityId {0}, GetProperty failed, fabric name {1},  duration {2}ms",
        WSTR(_activityId), WSTR(*_spStrDnsName), (LONG)duration.Milliseconds());

    KString& strQuestion = _spQuestion->Name();
    _cache.MarkNameAsPublic(strQuestion);

    ChangeStateAsync(true);
}

void FabricResolveOp::OnStateEnter_GetServiceDescription()
{
    _timeGetServiceDescription = KDateTime::Now();

    FabricAsyncOperationCallback callback(this, &FabricResolveOp::OnFabricGetServiceDescriptionCompleted);
    OperationCallback::Create(/*out*/_spSync, GetThisAllocator(), callback);

    HRESULT hr = _spInternalFabricServiceManagementClient2->BeginGetCachedServiceDescription(
        static_cast<LPCWSTR>(*_spFabricName),
        _fabricQueryTimeoutInSeconds * 1000,
        static_cast<IFabricAsyncOperationCallback*>(_spSync.RawPtr()),
        _spServiceDescriptionContext.InitializationAddress()
    );

    if (hr != S_OK)
    {
        KString& strQuestion = _spQuestion->Name();

        _tracer.Trace(DnsTraceLevel_Warning,
            "FabricResolveOp activityId {0}, fabric name {1}, DNS name {2}, FabricClient::BeginGetCachedServiceDescription failed with error {3}",
            WSTR(_activityId), WSTR(*_spFabricName), WSTR(strQuestion), hr);

        ChangeStateAsync(false);
    }
}

void FabricResolveOp::OnStateEnter_GetServiceDescriptionSucceeded()
{
    KDuration duration = KDateTime::Now() - _timeGetServiceDescription;

    _tracer.Trace(DnsTraceLevel_Noise,
        "FabricResolveOp activityId {0}, GetServiceDescription succeeded, fabric name {1},  duration {2}ms",
        WSTR(_activityId), WSTR(*_spFabricName), (LONG)duration.Milliseconds());

    ChangeStateAsync(true);
}

void FabricResolveOp::OnStateEnter_GetServiceDescriptionFailed()
{
    KDuration duration = KDateTime::Now() - _timeGetServiceDescription;

    _tracer.Trace(DnsTraceLevel_Noise,
        "FabricResolveOp activityId {0}, GetServiceDescription failed, fabric name {1},  duration {2}ms",
        WSTR(_activityId), WSTR(*_spFabricName), (LONG)duration.Milliseconds());

    ChangeStateAsync(true);
}

void FabricResolveOp::OnStateEnter_ResolveFabricName()
{
    KInvariant(_spFabricName != nullptr);

    _timeResolveServicePartition = KDateTime::Now();

    KString& strQuestion = _spQuestion->Name();

    _tracer.Trace(DnsTraceLevel_Noise,
        "FabricResolveOp activityId {0} DNS name mapped to fabric name {1} => {2}",
        WSTR(_activityId), WSTR(strQuestion), WSTR(*_spFabricName));

    FabricAsyncOperationCallback callback(this, &FabricResolveOp::OnFabricResolveCompleted);
    OperationCallback::Create(/*out*/_spSync, GetThisAllocator(), callback);

    FABRIC_PARTITION_KEY_TYPE keyType = FABRIC_PARTITION_KEY_TYPE_NONE;
    if (_partitionKind == FABRIC_PARTITION_SCHEME_NAMED)
    {
        keyType = FABRIC_PARTITION_KEY_TYPE_STRING;
    }
    else if (_partitionKind == FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE)
    {
        keyType = FABRIC_PARTITION_KEY_TYPE_INT64;
    }

    void* partitionKey = nullptr;
    LONGLONG key = 0;
    if (_spStrPartitionName != nullptr && !_spStrPartitionName->IsEmpty())
    {
        if (_partitionKind == FABRIC_PARTITION_SCHEME_NAMED)
        {
            partitionKey = static_cast<PVOID>(*_spStrPartitionName);
        }
        else if (_partitionKind == FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE)
        {
            if (_spStrPartitionName->ToLONGLONG(key))
            {
                partitionKey = &key;
            }
        }
    }

    HRESULT hr = _fabricServiceClient.BeginResolveServicePartition(
        *_spFabricName,
        keyType,
        partitionKey,
        nullptr,
        _fabricQueryTimeoutInSeconds * 1000,
        static_cast<IFabricAsyncOperationCallback*>(_spSync.RawPtr()),
        _spResolveContext.InitializationAddress()
    );

    if (hr != S_OK)
    {
        _tracer.Trace(DnsTraceLevel_Warning,
            "FabricResolveOp activityId {0}, DNS name {1}, service name {2}, FabricClient::BeginResolveServicePartition failed with error {3}",
            WSTR(_activityId), WSTR(strQuestion), WSTR(*_spFabricName), hr);

        ChangeStateAsync(false);
    }
}

void FabricResolveOp::OnStateEnter_ResolveFabricNameSucceeded()
{
    KDuration duration = KDateTime::Now() - _timeResolveServicePartition;

    _tracer.Trace(DnsTraceLevel_Noise,
        "FabricResolveOp activityId {0}, ResolveServicePartition succeeded, fabric name {1},  duration {2}ms",
        WSTR(_activityId), WSTR(*_spFabricName), (LONG)duration.Milliseconds());

    if (_arrResults.Count() == 0)
    {
        _cache.Remove(*_spFabricName);
    }

    ChangeStateAsync(true);
}

void FabricResolveOp::OnStateEnter_ResolveFabricNameFailed()
{
    KDuration duration = KDateTime::Now() - _timeResolveServicePartition;

    _tracer.Trace(DnsTraceLevel_Noise,
        "FabricResolveOp activityId {0}, ResolveServicePartition failed, fabric name {1},  duration {2}ms",
        WSTR(_activityId), WSTR(*_spFabricName), (LONG)duration.Milliseconds());

    ChangeStateAsync(true);
}

void FabricResolveOp::OnStateEnter_FabricResolveSucceeded()
{
    ChangeStateAsync(true);
}

void FabricResolveOp::OnStateEnter_FabricResolveFailed()
{
    ChangeStateAsync(true);
}

void FabricResolveOp::OnStateEnter_End()
{
    DeactivateStateMachine();

    if (_spResolveContext != nullptr)
    {
        _spResolveContext->Cancel();
    }

    Complete(STATUS_SUCCESS);
}

//***************************************
// END StateMachine region
//***************************************




void FabricResolveOp::StartResolve(
    __in_opt KAsyncContextBase* const parent,
    __in KStringView& activityId,
    __in IDnsRecord& record,
    __in FabricResolveCompletedCallback callback
)
{
    _activityId = activityId;
    _callback = callback;
    _spQuestion = &record;

    KString& strQuestion = _spQuestion->Name();

    if (_fIsPartitionedQueryEnabled)
    {
        ExtractPartition(strQuestion, GetThisAllocator(),
            _spStrPartitionPrefix, _spStrPartitionSuffix,
            /*out*/_spStrPartitionName, /*out*/_spStrDnsName);
    }
    else
    {
        _spStrDnsName = &strQuestion;
    }

    _tracer.Trace(DnsTraceLevel_Noise,
        "FabricResolveOp activityId {0}, StartResolve question {1}, partition {2}, DNS name {3}",
        WSTR(_activityId), WSTR(strQuestion), WSTR((_spStrPartitionName != nullptr) ? *_spStrPartitionName : L""), WSTR(*_spStrDnsName));

    Start(parent, nullptr/*callback*/);
}

void FabricResolveOp::CancelResolve()
{
    Cancel();
}

void FabricResolveOp::OnFabricGetPropertyCompleted(
    __in IFabricAsyncOperationContext* pResolveContext
)
{
    ComPointer<IFabricPropertyValueResult> spPropertyResult;
    HRESULT hr = _fabricPropertyClient.EndGetProperty(
        pResolveContext,
        spPropertyResult.InitializationAddress()
    );

    KString& strQuestion = _spQuestion->Name();

    if (hr == S_OK)
    {
        if (!_data.DeserializePropertyValue(*spPropertyResult.RawPtr(), /*out*/_spFabricName))
        {
            _tracer.Trace(DnsTraceLevel_Warning,
                "FabricResolveOp activityId {0}, DNS name {1}, failed to deserialize FabricClient::EndGetProperty result.",
                WSTR(_activityId), WSTR(strQuestion));
        }
    }
    else if (hr == FABRIC_E_PROPERTY_DOES_NOT_EXIST)
    {
        _tracer.Trace(DnsTraceLevel_Noise,
            "FabricResolveOp activityId {0}, DNS name {1}, FabricClient::EndGetProperty returned PropertyNotFound",
            WSTR(_activityId), WSTR(strQuestion));
    }
    else
    {
        _tracer.Trace(DnsTraceLevel_Warning,
            "FabricResolveOp activityId {0}, DNS name {1}, FabricClient::EndGetProperty failed with error {2}",
            WSTR(_activityId), WSTR(strQuestion), hr);
    }

    ULONG pos = 0;
    if ((_spFabricName != nullptr) &&
        (_spFabricName->Length() > 0) &&
        (_spFabricName->Search(FabricSchemeName, /*out*/pos)))
    {
        ChangeStateAsync(true);
    }
    else
    {
        ChangeStateAsync(false);
    }
}

void FabricResolveOp::OnFabricGetServiceDescriptionCompleted(
    __in IFabricAsyncOperationContext* pContext
)
{
    KString& strQuestion = _spQuestion->Name();

    ComPointer<IFabricServiceDescriptionResult> spResult;
    HRESULT hr = _spInternalFabricServiceManagementClient2->EndGetCachedServiceDescription(
        pContext, 
        spResult.InitializationAddress());

    if (hr != S_OK)
    {
        _tracer.Trace(DnsTraceLevel_Warning,
            "FabricResolveOp activityId {0}, DNS name {1}, service name {2}, FabricClient::EndGetCachedServiceDescription failed with error {3}",
            WSTR(_activityId), WSTR(strQuestion), WSTR(*_spFabricName), hr);

        ChangeStateAsync(false);
        return;
    }

    const FABRIC_SERVICE_DESCRIPTION* pDesc = spResult->get_Description();
    _serviceKind = pDesc->Kind;

    _partitionKind = FABRIC_PARTITION_SCHEME_INVALID;
    void* partitionSchemeDescription = nullptr;

    if (FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL == _serviceKind)
    {
        FABRIC_STATEFUL_SERVICE_DESCRIPTION* p = (FABRIC_STATEFUL_SERVICE_DESCRIPTION*)pDesc->Value;
        _partitionKind = p->PartitionScheme;
        partitionSchemeDescription = p->PartitionSchemeDescription;
    }
    else if (FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS == _serviceKind)
    {
        FABRIC_STATELESS_SERVICE_DESCRIPTION* p = (FABRIC_STATELESS_SERVICE_DESCRIPTION*)pDesc->Value;
        _partitionKind = p->PartitionScheme;
        partitionSchemeDescription = p->PartitionSchemeDescription;
    }
    else
    {
        _tracer.Trace(DnsTraceLevel_Warning,
            "FabricResolveOp activityId {0}, DNS name {1}, service name {2}, unknown service description type {3}",
            WSTR(_activityId), WSTR(strQuestion), WSTR(*_spFabricName), (ULONG)_serviceKind);

        ChangeStateAsync(false);
        return;
    }

    if (_spStrPartitionName == nullptr)
    {
        if (FABRIC_PARTITION_SCHEME_NAMED == _partitionKind)
        {
            FABRIC_NAMED_PARTITION_SCHEME_DESCRIPTION* pPartitionDesc =
                static_cast<FABRIC_NAMED_PARTITION_SCHEME_DESCRIPTION*>(partitionSchemeDescription);

            LONG id = rand() % pPartitionDesc->PartitionCount;
            KString::Create(/*out*/_spStrPartitionName, GetThisAllocator(), pPartitionDesc->Names[id]);

            _tracer.Trace(DnsTraceLevel_Noise,
                "FabricResolveOp activityId {0}, DNS name {1}, service name {2}, choosing random named partition {3}",
                WSTR(_activityId), WSTR(strQuestion), WSTR(*_spFabricName), WSTR(*_spStrPartitionName));
        }
        else if (FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE == _partitionKind)
        {
            FABRIC_UNIFORM_INT64_RANGE_PARTITION_SCHEME_DESCRIPTION* pPartitionDesc =
                static_cast<FABRIC_UNIFORM_INT64_RANGE_PARTITION_SCHEME_DESCRIPTION*>(partitionSchemeDescription);

            KLocalString<128> strKey;
            strKey.FromLONGLONG(pPartitionDesc->HighKey);
            KString::Create(/*out*/_spStrPartitionName, GetThisAllocator(), strKey);

            _tracer.Trace(DnsTraceLevel_Noise,
                "FabricResolveOp activityId {0}, DNS name {1}, service name {2}, choosing random int64 partition {3}",
                WSTR(_activityId), WSTR(strQuestion), WSTR(*_spFabricName), WSTR(*_spStrPartitionName));
        }
    }

    ChangeStateAsync(true);
}

void FabricResolveOp::OnFabricResolveCompleted(
    __in IFabricAsyncOperationContext* pResolveContext
)
{
    KInvariant(_spQuestion != nullptr);
    KInvariant(_spFabricName != nullptr);

    KString& strQuestion = _spQuestion->Name();

    ComPointer<IFabricResolvedServicePartitionResult> spResolveResult;
    HRESULT hr = _fabricServiceClient.EndResolveServicePartition(
        pResolveContext,
        spResolveResult.InitializationAddress()
    );

    bool fSuccess = false;
    if (hr == S_OK)
    {
        KInvariant(spResolveResult != nullptr);
        fSuccess = _data.DeserializeServiceEndpoints(*spResolveResult.RawPtr(), /*out*/_arrResults);
        if (!fSuccess)
        {
            _tracer.Trace(DnsTraceLevel_Warning,
                "FabricResolveOp activityId {0}, DNS name {2}, service name {3}, failed to deserialize FabricClient::EndResolveServicePartition result",
                WSTR(_activityId), WSTR(strQuestion), WSTR(*_spFabricName));
        }
    }
    else
    {
        _tracer.Trace(DnsTraceLevel_Noise,
            "FabricResolveOp activityId {0}, DNS name {1}, service name {2}, FabricClient::EndResolveServicePartition failed with error {3}",
            WSTR(_activityId), WSTR(strQuestion), WSTR(*_spFabricName), hr);
    }

    ChangeStateAsync(fSuccess);
}

/*static*/
void FabricResolveOp::ExtractPartition(
    __in KString& strQuestion,
    __in KAllocator& allocator,
    __in const KString::SPtr& spStrPartitionPrefix,
    __in const KString::SPtr& spStrPartitionSuffix,
    __out KString::SPtr& spStrPartition,
    __out KString::SPtr& spStrDnsName
)
{
    spStrDnsName = &strQuestion;

    ULONG ppl = 0;
    if (spStrPartitionPrefix != nullptr)
    {
        ppl = spStrPartitionPrefix->Length();
    }

    // If there is no prefix, we treat the whole question as the DNS name. 
    if (ppl == 0)
    {
        return;
    }

    ULONG psl = 0;
    if (spStrPartitionSuffix != nullptr)
    {
        psl = spStrPartitionSuffix->Length();
    }

    // If partition exists, it has to be in the first label, so let's start by finding the dot "."
    KStringView strDot(L".");
    ULONG labelEnd = 0;
    KStringView strLabel = strQuestion;
    if (strQuestion.Search(strDot, /*out*/labelEnd))
    {
        strLabel = strQuestion.LeftString(labelEnd);
    }

    // Label has to end with suffix, if present.
    ULONG suffixPos = strLabel.Length();
    if (psl > 0)
    {
        if (!strLabel.RSearch(*spStrPartitionSuffix, /*out*/suffixPos) || (suffixPos != (strLabel.Length() - psl)))
        {
            return;
        }
    }

    KStringView strLabelMinusSuffix = strLabel.LeftString(strLabel.Length() - psl);

    // Find first occurrence of prefix in the remaining label from right.
    ULONG prefixPos = 0;
    if (!strLabelMinusSuffix.RSearch(*spStrPartitionPrefix, /*out*/prefixPos))
    {
        return;
    }

    // No service DNS name or partition name, use whole question as the DNS name.
    if ((prefixPos == 0) || (suffixPos == (prefixPos + ppl)))
    {
        return;
    }

    KStringView strPartition = strLabel.SubString((prefixPos + ppl), (suffixPos - prefixPos - ppl));
    if (!NT_SUCCESS(KString::Create(/*out*/spStrPartition, allocator, strPartition)))
    {
        KInvariant(false);
    }

    KStringView strDnsName = strLabelMinusSuffix.LeftString(prefixPos);
    if (!NT_SUCCESS(KString::Create(/*out*/spStrDnsName, allocator, strDnsName)))
    {
        KInvariant(false);
    }

    if ((labelEnd != 0) && !spStrDnsName->Concat(strQuestion.RightString(strQuestion.Length() - labelEnd)))
    {
        KInvariant(false);
    }
}
