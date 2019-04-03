// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Constants.h"
#include "FabricDnsService.h"
#include "DnsServiceConfig.h"
#include "FabricData.h"

/*static*/
void FabricDnsService::Create(
    __out FabricDnsService::SPtr& spService,
    __in KAllocator& allocator,
    __in IDnsTracer& tracer,
    __in LPCWSTR serviceTypeName,
    __in FABRIC_URI serviceName,
    __in ULONG initializationDataLength,
    __in_ecount(initializationDataLength) const byte *initializationData,
    __in FABRIC_PARTITION_ID partitionId,
    __in FABRIC_REPLICA_ID replicaId
)
{
    spService = _new(TAG, allocator) FabricDnsService(tracer, serviceTypeName, serviceName,
        initializationDataLength, initializationData, partitionId, replicaId);
    KInvariant(spService != nullptr);
}

FabricDnsService::FabricDnsService(
    __in IDnsTracer& tracer,
    __in LPCWSTR serviceTypeName,
    __in FABRIC_URI serviceName,
    __in ULONG initializationDataLength,
    __in_ecount(initializationDataLength) const byte* initializationData,
    __in FABRIC_PARTITION_ID partitionId,
    __in FABRIC_REPLICA_ID replicaId
) : _tracer(tracer)
{
    UNREFERENCED_PARAMETER(serviceTypeName);
    UNREFERENCED_PARAMETER(serviceName);
    UNREFERENCED_PARAMETER(initializationDataLength);
    UNREFERENCED_PARAMETER(initializationData);
    UNREFERENCED_PARAMETER(partitionId);
    UNREFERENCED_PARAMETER(replicaId);

    if (STATUS_SUCCESS != KString::Create(/*out*/_spServiceName, GetThisAllocator(), serviceName, TRUE/*appendnull*/))
    {
        KInvariant(false);
    }
}

FabricDnsService::~FabricDnsService()
{
    _tracer.Trace(DnsTraceLevel_Info, "Destructing FabricDnsService.");
}

HRESULT FabricDnsService::BeginOpen(
    __in IFabricStatelessServicePartition* partition,
    __in IFabricAsyncOperationCallback* callback,
    __out IFabricAsyncOperationContext** context
)
{
    UNREFERENCED_PARAMETER(partition);

    _tracer.Trace(DnsTraceLevel_Info, "FabricDnsService BeginOpen called");

    LoadConfig(/*out*/_params);

    HRESULT hr = partition->QueryInterface(IID_IFabricStatelessServicePartition2, _spFabricHealth.VoidInitializationAddress());
    if (S_OK != hr)
    {
        _tracer.Trace(DnsTraceLevel_Error, "Failed to get IID_IFabricStatelessServicePartition2, error 0x{0}", hr);
        return E_FAIL;
    }

    ComPointer<IFabricPropertyManagementClient> spPropertyManagementClient;
    hr = FabricCreateLocalClient(IID_IFabricPropertyManagementClient, /*out*/spPropertyManagementClient.VoidInitializationAddress());
    if (spPropertyManagementClient == nullptr)
    {
        _tracer.Trace(DnsTraceLevel_Error, "Failed to create FabricClient IID_IFabricPropertyManagementClient, error 0x{0}", hr);
        return E_FAIL;
    }

    IDnsCache::SPtr spCache;
    DNS::CreateDnsCache(/*out*/spCache, GetThisAllocator(), _params.MaxCacheSize);

    hr = FabricCreateLocalClient2(this, IID_IFabricServiceManagementClient4, /*out*/_spServiceManagementClient.VoidInitializationAddress());
    if (_spServiceManagementClient == nullptr)
    {
        _tracer.Trace(DnsTraceLevel_Error, "Failed to create FabricClient IID_IFabricServiceManagementClient4, error 0x{0}", hr);
        return E_FAIL;
    }

    IDnsParser::SPtr spParser;
    DNS::CreateDnsParser(/*out*/spParser, GetThisAllocator());

    INetIoManager::SPtr spNetIoManager;
    DNS::CreateNetIoManager(/*out*/spNetIoManager, GetThisAllocator(), &_tracer, _params.NumberOfConcurrentQueries);

    FabricData::SPtr spData;
    FabricData::Create(/*out*/spData, GetThisAllocator(), _tracer);

    CreateFabricResolve(/*out*/_spFabricResolve, GetThisAllocator(),
        _spServiceName, &_tracer, spData.RawPtr(), spCache.RawPtr(),
        _spServiceManagementClient.RawPtr(), spPropertyManagementClient.RawPtr());

    if (!RegisterServiceNotifications())
    {
        return E_FAIL;
    }

    DNS::CreateDnsService(
        /*out*/_spService,
        GetThisAllocator(),
        &_tracer,
        spParser,
        spNetIoManager,
        _spFabricResolve,
        spCache,
        *_spFabricHealth.RawPtr(),
        _params
    );

    DnsServiceCallback closeCompletionCallback(this, &FabricDnsService::OnCloseCompleted);
    if (!_spService->Open(_params.Port, closeCompletionCallback))
    {
        _tracer.Trace(DnsTraceLevel_Error, "Failed to start DnsService");
        return E_FAIL;
    }

    FabricAsyncOperationContext::Create(/*out*/_spOpenOperationContext, GetThisAllocator(), callback);

    ComPointer<IFabricAsyncOperationContext> spOut = _spOpenOperationContext.RawPtr();
    *context = spOut.Detach();

    _spOpenOperationContext->SetCompleted(TRUE);
    callback->Invoke(_spOpenOperationContext.RawPtr());

    return S_OK;
}

HRESULT FabricDnsService::EndOpen(
    __in IFabricAsyncOperationContext* context,
    __out IFabricStringResult** serviceAddress
)
{
    _tracer.Trace(DnsTraceLevel_Info, "FabricDnsService EndOpen called");

    UNREFERENCED_PARAMETER(context);

    LPCWSTR wszIpAddress = L"127.0.0.1";
    ComPointer<IFabricNodeContextResult2> spContext;
    if (S_OK == FabricGetNodeContext(/*out*/spContext.VoidInitializationAddress()))
    {
        const FABRIC_NODE_CONTEXT* pFabricNodeContext = spContext->get_NodeContext();
        if (pFabricNodeContext != nullptr)
        {
            wszIpAddress = pFabricNodeContext->IPAddressOrFQDN;
        }
    }

    std::wstring endpoint = Common::wformatString("{0}:{1}", wszIpAddress, _params.Port);

    KString::SPtr spJson = KString::Create(GetThisAllocator());
    FabricData::SerializeServiceEndpoints(endpoint.c_str(), /*out*/*spJson);

    FabricStringResult::SPtr spResult;
    FabricStringResult::Create(/*out*/spResult, GetThisAllocator(), *spJson);

    *serviceAddress = spResult.Detach();

    return S_OK;
}

HRESULT FabricDnsService::BeginClose(
    __in IFabricAsyncOperationCallback* callback,
    __out IFabricAsyncOperationContext** context
)
{
    _tracer.Trace(DnsTraceLevel_Info, "FabricDnsService BeginClose called");

    FabricAsyncOperationContext::Create(/*out*/_spCloseOperationContext, GetThisAllocator(), callback);
    FabricAsyncOperationContext::SPtr spOut = _spCloseOperationContext;
    *context = spOut.Detach();

    UnegisterServiceNotifications();

    _spService->CloseAsync();
    _spServiceManagementClient = nullptr;

    // If the close time out config has been enabled, waiting for the event to be signaled
    // or shutdown the process after the configured timeout.
    if (_params.EnableOnCloseTimeout)
    {
        const ULONG timeoutInMs = _params.OnCloseTimeoutInSeconds * 1000;
        if (!_closeCompletedEvent.WaitUntilSet(timeoutInMs))
        {
            _tracer.Trace(DnsTraceLevel_Error, "FabricDnsService BeginClose failed to complete after {0} ms, killing the process", timeoutInMs);
            KInvariant(false);
        }
    }

    return S_OK;
}

HRESULT FabricDnsService::EndClose(
    __in IFabricAsyncOperationContext* context
)
{
    _tracer.Trace(DnsTraceLevel_Info, "FabricDnsService EndClose called");

    UNREFERENCED_PARAMETER(context);

    return S_OK;
}

void FabricDnsService::Abort(void)
{
    _tracer.Trace(DnsTraceLevel_Info, "FabricDnsService Abort called");

    UnegisterServiceNotifications();

    _spService->CloseAsync();
    _spServiceManagementClient = nullptr;

    const ULONG timeoutInMs = 3 * 1000;
    if (!_closeCompletedEvent.WaitUntilSet(timeoutInMs))
    {
        _tracer.Trace(DnsTraceLevel_Error, "FabricDnsService Abort failed to complete after {0} ms, killing the process", timeoutInMs);
        KInvariant(false);
    }
}

HRESULT FabricDnsService::OnNotification(
    __in IFabricServiceNotification* pNotification
)
{
    const FABRIC_SERVICE_NOTIFICATION* pData = pNotification->get_Notification();
    if (pData == nullptr)
    {
        return S_OK;
    }

    KString::SPtr spServiceName = KString::Create(pData->ServiceName, GetThisAllocator());
    bool fServiceIsDeleted = (pData->EndpointCount == 0);

    _spFabricResolve->NotifyServiceChanged(*spServiceName, fServiceIsDeleted);

    return S_OK;
}

void FabricDnsService::OnCloseCompleted(
    __in NTSTATUS status
)
{
    UNREFERENCED_PARAMETER(status);

    _tracer.Trace(DnsTraceLevel_Info, "FabricDnsService OnCloseCompleted called");

    _closeCompletedEvent.SetEvent();

    if (_spCloseOperationContext != nullptr)
    {
        ComPointer<IFabricAsyncOperationCallback> spCallback;
        if (S_OK != _spCloseOperationContext->get_Callback(spCallback.InitializationAddress()))
        {
            KInvariant(false);
        }

        _tracer.Trace(DnsTraceLevel_Info, "FabricDnsService OnCloseCompleted invoking close completed callback.");

        _spCloseOperationContext->SetCompleted(TRUE);
        spCallback->Invoke(_spCloseOperationContext.RawPtr());
    }
}

void FabricDnsService::LoadConfig(
    __out DnsServiceParams& params
)
{
    params.Port = static_cast<USHORT>(DnsServiceConfig::GetConfig().DnsPort);
    params.NumberOfConcurrentQueries = DnsServiceConfig::GetConfig().NumberOfConcurrentQueries;
    params.MaxMessageSizeInKB = DnsServiceConfig::GetConfig().MaxMessageSizeInKB;
    params.MaxCacheSize = DnsServiceConfig::GetConfig().MaxCacheSize;
    params.IsRecursiveQueryEnabled = DnsServiceConfig::GetConfig().IsRecursiveQueryEnabled;
    params.NDots = (ULONG)DnsServiceConfig::GetConfig().NDots;
    params.SetAsPreferredDns = DnsServiceConfig::GetConfig().SetAsPreferredDns;
    params.FabricQueryTimeoutInSeconds = (ULONG)DnsServiceConfig::GetConfig().FabricQueryTimeout.TotalSeconds();
    params.RecursiveQueryTimeoutInSeconds = (ULONG)DnsServiceConfig::GetConfig().RecursiveQueryTimeout.TotalSeconds();
    params.TimeToLiveInSeconds = max((ULONG)1, (ULONG)DnsServiceConfig::GetConfig().TimeToLive.TotalSeconds());
    params.AllowMultipleListeners = DnsServiceConfig::GetConfig().AllowMultipleListeners;
    params.EnablePartitionedQuery = DnsServiceConfig::GetConfig().EnablePartitionedQuery;
    params.EnableOnCloseTimeout = DnsServiceConfig::GetConfig().EnableOnCloseTimeout;
    params.OnCloseTimeoutInSeconds = (ULONG)DnsServiceConfig::GetConfig().OnCloseTimeoutInSeconds.TotalSeconds();

    if (!DnsServiceConfig::GetConfig().PartitionPrefix.empty())
    {
        params.PartitionPrefix = KString::Create(DnsServiceConfig::GetConfig().PartitionPrefix.c_str(), GetThisAllocator());
    }
    if (!DnsServiceConfig::GetConfig().PartitionSuffix.empty())
    {
        params.PartitionSuffix = KString::Create(DnsServiceConfig::GetConfig().PartitionSuffix.c_str(), GetThisAllocator());
    }
}

bool FabricDnsService::RegisterServiceNotifications()
{
    HRESULT hr = S_OK;

    FABRIC_SERVICE_NOTIFICATION_FILTER_DESCRIPTION description = { 0 };
    description.Name = FabricSchemeName;
    description.Flags = FABRIC_SERVICE_NOTIFICATION_FILTER_FLAGS_NAME_PREFIX;

    const ULONG waitInMs = 10000;

    FabricAsyncOperationCallback::SPtr spSync;
    FabricAsyncOperationCallback::Create(/*out*/spSync, GetThisAllocator());
    ComPointer<IFabricAsyncOperationContext> spContext;
    if (S_OK != (hr = _spServiceManagementClient->BeginRegisterServiceNotificationFilter(
        &description,
        waitInMs,
        spSync.RawPtr(),
        spContext.InitializationAddress())))
    {
        _tracer.Trace(DnsTraceLevel_Warning, "Failed to register for service notifications, error 0x{0}", hr);
        return false;
    }

    if (!spSync->Wait(waitInMs))
    {
        _tracer.Trace(DnsTraceLevel_Warning, "Failed to register for service notifications due to a timeout");
        return false;
    }

    _spServiceManagementClient->EndRegisterServiceNotificationFilter(spContext.RawPtr(), /*out*/&_filterId);

    return true;
}

void FabricDnsService::UnegisterServiceNotifications()
{
    const ULONG waitInMs = 10000;

    HRESULT hr = S_OK;
    FabricAsyncOperationCallback::SPtr spSync;
    FabricAsyncOperationCallback::Create(/*out*/spSync, GetThisAllocator());
    ComPointer<IFabricAsyncOperationContext> spContext;
    if (S_OK != (hr = _spServiceManagementClient->BeginUnregisterServiceNotificationFilter(
        _filterId,
        waitInMs,
        spSync.RawPtr(),
        spContext.InitializationAddress())))
    {
        _tracer.Trace(DnsTraceLevel_Warning, "Failed to unregister for service notifications, error 0x{0}", hr);
        return;
    }

    if (!spSync->Wait(waitInMs))
    {
        _tracer.Trace(DnsTraceLevel_Warning, "Failed to unregister for service notifications due to a timeout");
        return;
    }

    _spServiceManagementClient->EndUnregisterServiceNotificationFilter(spContext.RawPtr());
}
