// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;


StringLiteral const TraceComponent("ResourceMonitoringAgent");


// {D181F157-5B93-4B6B-9C1A-D22BA72CDA56}
static const GUID CLSID_OpenComAsyncOperationContext =
{ 0xd181f157, 0x5b93, 0x4b6b,{ 0x9c, 0x1a, 0xd2, 0x2b, 0xa7, 0x2c, 0xda, 0x56 } };


class ResourceMonitorServiceInstance::OpenComAsyncOperationContext
    : public ComAsyncOperationContext
{
    DENY_COPY(OpenComAsyncOperationContext)

    COM_INTERFACE_LIST2(
        OpenComAsyncOperationContext,
        IID_IFabricAsyncOperationContext,
        IFabricAsyncOperationContext,
        CLSID_OpenComAsyncOperationContext,
        OpenComAsyncOperationContext)

public:
    explicit OpenComAsyncOperationContext(__in ResourceMonitorServiceInstance & owner)
        : ComAsyncOperationContext(),
        owner_(owner)
    {
    }

    virtual ~OpenComAsyncOperationContext()
    {
    }

    HRESULT STDMETHODCALLTYPE Initialize(
        __in ComponentRootSPtr const & rootSPtr,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
        if (FAILED(hr))
        {
            return hr;
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL)
        {
            return E_POINTER;
        }

        ComPointer<OpenComAsyncOperationContext> thisOperation(context, CLSID_OpenComAsyncOperationContext);
        return thisOperation->Result;
    }

protected:
    void OnStart(__in AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = owner_.AgentSPtr->BeginOpen(
            ResourceMonitorServiceConfig::GetConfig().InstanceOpenTimeout,
            [this](AsyncOperationSPtr const & operation) { this->OnAgentOpened(operation, false); },
            proxySPtr);
        OnAgentOpened(operation, true);
    }

private:
    void OnAgentOpened(AsyncOperationSPtr const & operation, bool expectCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.AgentSPtr->EndOpen(operation);
        TryComplete(operation->Parent, error.ToHResult());
    }

private:
    ResourceMonitorServiceInstance & owner_;
};

// {43FA484D-9962-4052-B1CE-367D69EB6857}
static const GUID CLSID_CloseComAsyncOperationContext =
{ 0x43fa484d, 0x9962, 0x4052,{ 0xb1, 0xce, 0x36, 0x7d, 0x69, 0xeb, 0x68, 0x57 } };


class ResourceMonitorServiceInstance::CloseComAsyncOperationContext
    : public ComAsyncOperationContext
{
    DENY_COPY(CloseComAsyncOperationContext)

    COM_INTERFACE_LIST2(
        CloseComAsyncOperationContext,
        IID_IFabricAsyncOperationContext,
        IFabricAsyncOperationContext,
        CLSID_CloseComAsyncOperationContext,
        CloseComAsyncOperationContext)

public:
    explicit CloseComAsyncOperationContext(__in ResourceMonitorServiceInstance & owner)
        : ComAsyncOperationContext(),
        owner_(owner)
    {
    }

    virtual ~CloseComAsyncOperationContext()
    {
    }

    HRESULT STDMETHODCALLTYPE Initialize(
        __in ComponentRootSPtr const & rootSPtr,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
        if (FAILED(hr))
        {
            return hr;
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL)
        {
            return E_POINTER;
        }

        ComPointer<CloseComAsyncOperationContext> thisOperation(context, CLSID_CloseComAsyncOperationContext);
        return thisOperation->Result;
    }

protected:
    void OnStart(__in AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = owner_.AgentSPtr->BeginClose(
            ResourceMonitorServiceConfig::GetConfig().InstanceCloseTimeout,
            [this](AsyncOperationSPtr const & operation) { this->OnAgentClosed(operation, false); },
            proxySPtr);
        OnAgentClosed(operation, true);
    }

private:
    void OnAgentClosed(AsyncOperationSPtr const & operation, bool expectCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.AgentSPtr->EndClose(operation);
        TryComplete(operation->Parent, error.ToHResult());
    }

private:
    ResourceMonitorServiceInstance & owner_;
};

ResourceMonitorServiceInstance::ResourceMonitorServiceInstance(
    wstring const & serviceName,
    wstring const & serviceTypeName,
    Guid const & partitionId,
    FABRIC_INSTANCE_ID instanceId,
    Common::ComPointer<IFabricRuntime> const & fabricRuntimeCPtr)
    : IFabricStatelessServiceInstance()
     , ComUnknownBase()
     , serviceName_(serviceName)
     , serviceTypeName_(serviceTypeName)
     , partitionId_(partitionId)
     , instanceId_(instanceId)
     , fabricRuntimeCPtr_(fabricRuntimeCPtr)
{
}

ResourceMonitorServiceInstance::~ResourceMonitorServiceInstance()
{
}

HRESULT ResourceMonitorServiceInstance::BeginOpen(
    /* [in] */ IFabricStatelessServicePartition *partition,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    UNREFERENCED_PARAMETER(partition);

    Hosting2::ComFabricRuntime * castedRuntimePtr;
    try
    {
#if !defined(PLATFORM_UNIX)
        castedRuntimePtr = dynamic_cast<Hosting2::ComFabricRuntime*>(fabricRuntimeCPtr_.GetRawPointer());
#else
        castedRuntimePtr = (Hosting2::ComFabricRuntime*)(fabricRuntimeCPtr_.GetRawPointer());
#endif
    }
    catch (...)
    {
        Assert::TestAssert("ResourceMonitorService unable to get ComFabricRuntime");
        return ComUtility::OnPublicApiReturn(ErrorCode(ErrorCodeValue::OperationFailed).ToHResult());
    }

    Transport::IpcClient & ipcClient = castedRuntimePtr->Runtime->Host.Client;

    resourceMonitorAgent_ = make_shared<ResourceMonitoringAgent>(castedRuntimePtr, ipcClient);

    // Start opening the ResourceMonitoringAgent
    resourceMonitorAgent_->WriteNoise(TraceComponent, "ResourceMonitorServiceInstance BeginOpen called.");

    ComPointer<IUnknown> rootCPtr;
    HRESULT hr = this->QueryInterface(IID_IUnknown, (void**)rootCPtr.InitializationAddress());
    if (FAILED(hr))
    {
        resourceMonitorAgent_->WriteWarning(
            TraceComponent,
            "ResourceMonitorServiceInstance::BeginOpen Initializing root com operation failed with error {0}.",
            ErrorCode::FromHResult(hr));
        return ComUtility::OnPublicApiReturn(hr);
    }

    auto root = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));

    ComPointer<OpenComAsyncOperationContext> operation = make_com<OpenComAsyncOperationContext>(*this);
    hr = operation->Initialize(root, callback);
    if (FAILED(hr))
    {
        resourceMonitorAgent_->WriteWarning(
            TraceComponent,
            "ResourceMonitorServiceInstance::BeginOpen Initializing OpenComAsyncOperationContext failed with error {0}.",
            ErrorCode::FromHResult(hr));
        return ComUtility::OnPublicApiReturn(hr);
    }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ResourceMonitorServiceInstance::EndOpen(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricStringResult **serviceAddress)
{
    HRESULT hr = OpenComAsyncOperationContext::End(context);
    if (FAILED(hr))
    {
        resourceMonitorAgent_->WriteWarning(
            TraceComponent,
            "ResourceMonitorServiceInstance::EndOpen error when ending operation: {0}.",
            ErrorCode::FromHResult(hr));
        return ComUtility::OnPublicApiReturn(hr);
    }

    // This service does not actually listen to anything, using dummy address.
    hr = ComStringResult::ReturnStringResult(L"127.0.0.1", serviceAddress);
    if (FAILED(hr))
    {
        resourceMonitorAgent_->WriteWarning(
            TraceComponent,
            "ResourceMonitorServiceInstance::EndOpen error when converting address: {0}.",
            ErrorCode::FromHResult(hr));
    }
    else
    {
        resourceMonitorAgent_->WriteNoise(TraceComponent, "ResourceMonitorServiceInstance opened succesfully.");
    }

    return hr;
}

HRESULT ResourceMonitorServiceInstance::BeginClose(
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    // Start opening the ResourceMonitoringAgent
    resourceMonitorAgent_->WriteNoise(TraceComponent, "ResourceMonitorServiceInstance BeginClose called.");

    ComPointer<IUnknown> rootCPtr;
    HRESULT hr = this->QueryInterface(IID_IUnknown, (void**)rootCPtr.InitializationAddress());
    if (FAILED(hr))
    {
        resourceMonitorAgent_->WriteWarning(
            TraceComponent,
            "ResourceMonitorServiceInstance::BeginClose Initializing root com operation failed with error {0}.",
            ErrorCode::FromHResult(hr));
        return ComUtility::OnPublicApiReturn(hr);
    }

    auto root = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));

    ComPointer<CloseComAsyncOperationContext> operation = make_com<CloseComAsyncOperationContext>(*this);
    hr = operation->Initialize(root, callback);
    if (FAILED(hr))
    {
        resourceMonitorAgent_->WriteWarning(
            TraceComponent,
            "ResourceMonitorServiceInstance::BeginClose Initializing OpenComAsyncOperationContext failed with error {0}.",
            ErrorCode::FromHResult(hr));
        return ComUtility::OnPublicApiReturn(hr);
    }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ResourceMonitorServiceInstance::EndClose(/* [in] */ IFabricAsyncOperationContext *context)
{
    HRESULT hr = CloseComAsyncOperationContext::End(context);
    return ComUtility::OnPublicApiReturn(hr);
}

void ResourceMonitorServiceInstance::Abort()
{
    resourceMonitorAgent_->Abort();
}
