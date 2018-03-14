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
    __in ULONG fabricQueryTimeoutInSeconds
)
{
    spResolveOp = _new(TAG, allocator) FabricResolveOp(serviceName, tracer, data, cache,
        fabricServiceClient, fabricPropertyClient, fabricQueryTimeoutInSeconds);
    KInvariant(spResolveOp != nullptr);
}

FabricResolveOp::FabricResolveOp(
    __in KString& serviceName,
    __in IDnsTracer& tracer,
    __in IFabricData& data,
    __in IDnsCache& cache,
    __in IFabricServiceManagementClient& fabricServiceClient,
    __in IFabricPropertyManagementClient& fabricPropertyClient,
    __in ULONG fabricQueryTimeoutInSeconds
) : _serviceName(serviceName),
_tracer(tracer),
_data(data),
_cache(cache),
_fabricServiceClient(fabricServiceClient),
_fabricPropertyClient(fabricPropertyClient),
_fabricQueryTimeoutInSeconds(fabricQueryTimeoutInSeconds),
_arrResults(GetThisAllocator()),
_nameType(DnsNameTypeUnknown)
{
}

FabricResolveOp::~FabricResolveOp()
{
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
    _callback(*_spQuestion, _arrResults);
}

void FabricResolveOp::OnCancel()
{
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
    KString& strQuestion = _spQuestion->Name();
    _spFabricName = nullptr;
    _nameType = _cache.Read(strQuestion, /*out*/_spFabricName);

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
        static_cast<LPCWSTR>(strQuestion),
        _fabricQueryTimeoutInSeconds * 1000,
        static_cast<IFabricAsyncOperationCallback*>(_spSync.RawPtr()),
        _spResolveContext.InitializationAddress()
    );

    if (hr != S_OK)
    {
        _tracer.Trace(DnsTraceLevel_Warning,
            "FabricResolveOp activityId {0}, DNS name {1}, FabricClient::BeginGetProperty failed with error {2}",
            WSTR(_activityId), WSTR(strQuestion), hr);

        ChangeStateAsync(false);
    }
}

void FabricResolveOp::OnStateEnter_MapDnsNameToFabricNameSucceeded()
{
    KString& strQuestion = _spQuestion->Name();
    _cache.Put(strQuestion, *_spFabricName);

    ChangeStateAsync(true);
}

void FabricResolveOp::OnStateEnter_MapDnsNameToFabricNameFailed()
{
    KString& strQuestion = _spQuestion->Name();
    _cache.MarkNameAsPublic(strQuestion);

    ChangeStateAsync(true);
}

void FabricResolveOp::OnStateEnter_ResolveFabricName()
{
    KInvariant(_spFabricName != nullptr);

    KString& strQuestion = _spQuestion->Name();

    _tracer.Trace(DnsTraceLevel_Noise,
        "FabricResolveOp activityId {0} DNS name mapped to fabric name {1} => {2}",
        WSTR(_activityId), WSTR(strQuestion), WSTR(*_spFabricName));

    FabricAsyncOperationCallback callback(this, &FabricResolveOp::OnFabricResolveCompleted);
    OperationCallback::Create(/*out*/_spSync, GetThisAllocator(), callback);

    HRESULT hr = _fabricServiceClient.BeginResolveServicePartition(
        *_spFabricName,
        FABRIC_PARTITION_KEY_TYPE_NONE,
        nullptr,
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
    if (_arrResults.Count() == 0)
    {
        _cache.Remove(*_spFabricName);
    }

    ChangeStateAsync(true);
}

void FabricResolveOp::OnStateEnter_ResolveFabricNameFailed()
{
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
                "FabricResolveOp activityId {0}, DNS name {1}, service name {2}, failed to deserialize FabricClient::EndResolveServicePartition result",
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
