// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

// ********************************************************************************************************************
// ComFabricRuntime::RegisterStatelessServiceFactoryComAsyncOperationContext Implementation
//

// {1674361B-4A80-43AA-9292-13003AB01B34}
static const GUID CLSID_RegisterStatelessServiceFactoryComAsyncOperationContext =
{ 0x1674361b, 0x4a80, 0x43aa, { 0x92, 0x92, 0x13, 0x0, 0x3a, 0xb0, 0x1b, 0x34 } };

class ComFabricRuntime::RegisterStatelessServiceFactoryComAsyncOperationContext
    : public ComAsyncOperationContext
{
      DENY_COPY(RegisterStatelessServiceFactoryComAsyncOperationContext)

    COM_INTERFACE_LIST2(
        RegisterStatelessServiceFactoryComAsyncOperationContext,
        IID_IFabricAsyncOperationContext,
        IFabricAsyncOperationContext,
        CLSID_RegisterStatelessServiceFactoryComAsyncOperationContext,
        RegisterStatelessServiceFactoryComAsyncOperationContext)

public:
    explicit RegisterStatelessServiceFactoryComAsyncOperationContext(__in ComFabricRuntime & owner) 
        : ComAsyncOperationContext(),
        owner_(owner),
        serviceType_(),
        factory_(),
        timeout_()
    {
    }

    virtual ~RegisterStatelessServiceFactoryComAsyncOperationContext()
    {
    }

    HRESULT STDMETHODCALLTYPE Initialize(
        __in ComponentRootSPtr const & rootSPtr,
        __in LPCWSTR serviceType,
        __in IFabricStatelessServiceFactory *factory,
        __in DWORD timeoutMilliseconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
        if (FAILED(hr)) { return hr; }

        if (serviceType == NULL || factory == NULL) { return E_POINTER; }
        if (!ParameterValidator::IsValid(serviceType).IsSuccess()) { return E_INVALIDARG; }

        serviceType_ = wstring(serviceType);
        factory_.SetAndAddRef(factory);
    
        if (timeoutMilliseconds == INFINITE)
        {
            timeout_ = TimeSpan::MaxValue;
        }
        else if (timeoutMilliseconds == 0)
        {
            timeout_ = HostingConfig::GetConfig().ServiceFactoryRegistrationTimeout;
        }
        else
        {
            timeout_ = TimeSpan::FromMilliseconds(static_cast<double>(timeoutMilliseconds));
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<RegisterStatelessServiceFactoryComAsyncOperationContext> thisOperation(context, CLSID_RegisterStatelessServiceFactoryComAsyncOperationContext);
        return thisOperation->Result;
    }

protected:
    void OnStart(__in AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = owner_.Runtime->FactoryManager->BeginRegisterStatelessServiceFactory(
            serviceType_,
            factory_,
            timeout_,
            [this](AsyncOperationSPtr const & operation){ this->FinishRegisterStatelessServiceFactory(operation, false); },
            proxySPtr);
        FinishRegisterStatelessServiceFactory(operation, true);
    }

private:
    void FinishRegisterStatelessServiceFactory(AsyncOperationSPtr const & operation, bool expectCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.Runtime->FactoryManager->EndRegisterStatelessServiceFactory(operation);
        TryComplete(operation->Parent, error.ToHResult());
    }

private:
    ComFabricRuntime & owner_;
    TimeSpan timeout_;
    wstring serviceType_;
    ComPointer<IFabricStatelessServiceFactory> factory_;
};

// ********************************************************************************************************************
// ComFabricRuntime::RegisterStatefulServiceFactoryComAsyncOperationContext Implementation
//

// {727A535F-A361-4626-84CA-4FDBEB9BC0D4}
static const GUID CLSID_RegisterStatefulServiceFactoryComAsyncOperationContext =
{ 0x727a535f, 0xa361, 0x4626, { 0x84, 0xca, 0x4f, 0xdb, 0xeb, 0x9b, 0xc0, 0xd4 } };

class ComFabricRuntime::RegisterStatefulServiceFactoryComAsyncOperationContext
    : public ComAsyncOperationContext
{
      DENY_COPY(RegisterStatefulServiceFactoryComAsyncOperationContext)

    COM_INTERFACE_LIST2(
        RegisterStatefulServiceFactoryComAsyncOperationContext,
        IID_IFabricAsyncOperationContext,
        IFabricAsyncOperationContext,
        CLSID_RegisterStatefulServiceFactoryComAsyncOperationContext,
        RegisterStatefulServiceFactoryComAsyncOperationContext)

public:
    explicit RegisterStatefulServiceFactoryComAsyncOperationContext(__in ComFabricRuntime & owner) 
        : ComAsyncOperationContext(),
        owner_(owner),
        serviceType_(),
        factory_(),
        timeout_()
    {
    }

    virtual ~RegisterStatefulServiceFactoryComAsyncOperationContext()
    {
    }

    HRESULT STDMETHODCALLTYPE Initialize(
        __in ComponentRootSPtr const & rootSPtr,
        __in LPCWSTR serviceType,
        __in IFabricStatefulServiceFactory *factory,
        __in DWORD timeoutMilliseconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
        if (FAILED(hr)) { return hr; }
        
        if (serviceType == NULL || factory == NULL) { return E_POINTER; }
        if (!ParameterValidator::IsValid(serviceType).IsSuccess()) { return E_INVALIDARG; }

        serviceType_ = wstring(serviceType);
        factory_.SetAndAddRef(factory);
    
        if (timeoutMilliseconds == INFINITE)
        {
            timeout_ = TimeSpan::MaxValue;
        }
        else if (timeoutMilliseconds == 0)
        {
            timeout_ = HostingConfig::GetConfig().ServiceFactoryRegistrationTimeout;
        }
        else
        {
            timeout_ = TimeSpan::FromMilliseconds(static_cast<double>(timeoutMilliseconds));
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<RegisterStatefulServiceFactoryComAsyncOperationContext> thisOperation(context, CLSID_RegisterStatefulServiceFactoryComAsyncOperationContext);
        return thisOperation->Result;
    }

protected:
    void OnStart(__in AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = owner_.Runtime->FactoryManager->BeginRegisterStatefulServiceFactory(
            serviceType_,
            factory_,
            timeout_,
            [this](AsyncOperationSPtr const & operation){ this->FinishRegisterStatefulServiceFactory(operation, false); },
            proxySPtr);
        FinishRegisterStatefulServiceFactory(operation, true);
    }

private:
    void FinishRegisterStatefulServiceFactory(AsyncOperationSPtr const & operation, bool expectCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.Runtime->FactoryManager->EndRegisterStatefulServiceFactory(operation);
        TryComplete(operation->Parent, error.ToHResult());
    }

private:
    ComFabricRuntime & owner_;
    TimeSpan timeout_;
    wstring serviceType_;
    ComPointer<IFabricStatefulServiceFactory> factory_;
};


// ********************************************************************************************************************
// ComFabricRuntime::RegisterServiceGroupFactoryComAsyncOperationContext Implementation
//

// {889B8AFB-D901-45F3-A80E-46C24CB2BDC2}
static const GUID CLSID_RegisterServiceGroupFactoryComAsyncOperationContext =
{ 0x889b8afb, 0xd901, 0x45f3, { 0xa8, 0xe, 0x46, 0xc2, 0x4c, 0xb2, 0xbd, 0xc2 } };

class ComFabricRuntime::RegisterServiceGroupFactoryComAsyncOperationContext
    : public ComAsyncOperationContext
{
      DENY_COPY(RegisterServiceGroupFactoryComAsyncOperationContext)

    COM_INTERFACE_LIST2(
        RegisterServiceGroupFactoryComAsyncOperationContext,
        IID_IFabricAsyncOperationContext,
        IFabricAsyncOperationContext,
        CLSID_RegisterServiceGroupFactoryComAsyncOperationContext,
        RegisterServiceGroupFactoryComAsyncOperationContext)

public:
    explicit RegisterServiceGroupFactoryComAsyncOperationContext(__in ComFabricRuntime & owner) 
        : ComAsyncOperationContext(),
        owner_(owner),
        groupServiceType_(),
        statelessFactory_(),
        statefulFactory_(),
        timeoutHelper_()
    {
    }

    virtual ~RegisterServiceGroupFactoryComAsyncOperationContext()
    {
    }

    HRESULT STDMETHODCALLTYPE Initialize(
        __in ComponentRootSPtr const & rootSPtr,
        __in LPCWSTR groupServiceType,
        __in IFabricServiceGroupFactory * factory,
        __in DWORD timeoutMilliseconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
        if (FAILED(hr)) { return hr; }

        if (groupServiceType == NULL || factory == NULL) { return E_POINTER; }
        if (!ParameterValidator::IsValid(groupServiceType).IsSuccess()) { return E_INVALIDARG; }
        
        groupServiceType_ = wstring(groupServiceType);

        IFabricStatelessServiceFactory * statelessFactory = NULL;
        hr = factory->QueryInterface(IID_InternalStatelessServiceGroupFactory, (void**)&statelessFactory);
        if (SUCCEEDED(hr))
        {
            statelessFactory_.SetNoAddRef(statelessFactory);
        }
        
        IFabricStatefulServiceFactory * statefulFactory = NULL;
        hr = factory->QueryInterface(IID_InternalIStatefulServiceGroupFactory, (void**)&statefulFactory);
        if (SUCCEEDED(hr))
        {
            statefulFactory_.SetNoAddRef(statefulFactory);
        }

        if ((statelessFactory_.GetRawPointer() == NULL) && (statefulFactory_.GetRawPointer() == NULL))
        {
            return E_INVALIDARG;
        }
        if ((statelessFactory_.GetRawPointer() != NULL) && (statefulFactory_.GetRawPointer() != NULL))
        {
            return E_INVALIDARG;
        }

        TimeSpan timeout;
        if (timeoutMilliseconds == INFINITE)
        {
            timeout = TimeSpan::MaxValue;
        }
        else if (timeoutMilliseconds == 0)
        {
            timeout = HostingConfig::GetConfig().ServiceFactoryRegistrationTimeout;
        }
        else
        {
            timeout = TimeSpan::FromMilliseconds(static_cast<double>(timeoutMilliseconds));
        }
        timeoutHelper_ = make_unique<TimeoutHelper>(timeout);

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<RegisterServiceGroupFactoryComAsyncOperationContext> thisOperation(context, CLSID_RegisterServiceGroupFactoryComAsyncOperationContext);
        return thisOperation->Result;
    }

protected:
    void OnStart(__in AsyncOperationSPtr const & proxySPtr)
    {
        if (statelessFactory_.GetRawPointer() != NULL)
        {
            auto operation = owner_.Runtime->FactoryManager->BeginRegisterStatelessServiceFactory(
                groupServiceType_,
                statelessFactory_,
                timeoutHelper_->GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation){ this->FinishRegisterStatelessServiceFactory(operation, false); },
                proxySPtr);
            FinishRegisterStatelessServiceFactory(operation, true);
        }
        else
        {
            ASSERT_IF(statefulFactory_.GetRawPointer() == NULL, "StatefulFactory must be valid.");
            auto operation = owner_.Runtime->FactoryManager->BeginRegisterStatefulServiceFactory(
                groupServiceType_,
                statefulFactory_,
                timeoutHelper_->GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation){ this->FinishRegisterStatefulServiceFactory(operation, false); },
                proxySPtr);
            FinishRegisterStatefulServiceFactory(operation, true);
        }
    }

private:
    void FinishRegisterStatelessServiceFactory(AsyncOperationSPtr const & operation, bool expectCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.Runtime->FactoryManager->EndRegisterStatelessServiceFactory(operation);
        TryComplete(operation->Parent, error.ToHResult());
    }

    void FinishRegisterStatefulServiceFactory(AsyncOperationSPtr const & operation, bool expectCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.Runtime->FactoryManager->EndRegisterStatefulServiceFactory(operation);
        TryComplete(operation->Parent, error.ToHResult());
    }
private:
    ComFabricRuntime & owner_;
    unique_ptr<TimeoutHelper> timeoutHelper_;
    wstring groupServiceType_;
    ComPointer<IFabricStatelessServiceFactory> statelessFactory_;
    ComPointer<IFabricStatefulServiceFactory> statefulFactory_;
};

// ********************************************************************************************************************
// ComFabricRuntime Implementation
//

ComFabricRuntime::ComFabricRuntime(FabricRuntimeImplSPtr const & fabricRuntime)
    : IFabricRuntime(),
    ComUnknownBase(),
    fabricRuntime_(fabricRuntime)
{
}

ComFabricRuntime::~ComFabricRuntime()
{   
    fabricRuntime_->Host.UnregisterRuntimeAsync(fabricRuntime_->RuntimeId);
    TraceNoise(
        TraceTaskCodes::Hosting,
        "ComFabricRuntime",
        "ComFabricRuntime::Destructed RuntimeId={0}",
        fabricRuntime_->RuntimeId);
}

HRESULT ComFabricRuntime::BeginRegisterStatelessServiceFactory( 
    /* [in] */ LPCWSTR serviceType,
    /* [in] */ IFabricStatelessServiceFactory *factory,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    ComPointer<IUnknown> rootCPtr;
    HRESULT hr = this->QueryInterface(IID_IUnknown, (void**)rootCPtr.InitializationAddress());
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    auto root = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));

    ComPointer<RegisterStatelessServiceFactoryComAsyncOperationContext> operation 
        = make_com<RegisterStatelessServiceFactoryComAsyncOperationContext>(*this);
    hr = operation->Initialize(
        root,
        serviceType,
        factory,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}
        
HRESULT ComFabricRuntime::EndRegisterStatelessServiceFactory( 
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(RegisterStatelessServiceFactoryComAsyncOperationContext::End(context));
}

HRESULT ComFabricRuntime::RegisterStatelessServiceFactory( 
    /* [string][in] */ LPCWSTR serviceType,
    /* [in] */ IFabricStatelessServiceFactory *factory)
{
    auto waiter = make_shared<ComAsyncOperationWaiter>();
    ComPointer<IFabricAsyncOperationContext> context;

    HRESULT hr = this->BeginRegisterStatelessServiceFactory(
        serviceType,
        factory,
        0,  // use default from the configuration
        waiter->GetAsyncOperationCallback([this, waiter](IFabricAsyncOperationContext * context)
        {
            auto hr = this->EndRegisterStatelessServiceFactory(context);
            waiter->SetHRESULT(hr);
            waiter->Set();
        }),
        context.InitializationAddress());
    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }
    else
    {
        waiter->WaitOne();
        return ComUtility::OnPublicApiReturn(waiter->GetHRESULT());
    }
}

HRESULT ComFabricRuntime::BeginRegisterStatefulServiceFactory( 
    /* [in] */ LPCWSTR serviceType,
    /* [in] */ IFabricStatefulServiceFactory *factory,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    ComPointer<IUnknown> rootCPtr;
    HRESULT hr = this->QueryInterface(IID_IUnknown, (void**)rootCPtr.InitializationAddress());
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    ComponentRootSPtr root = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));

    ComPointer<RegisterStatefulServiceFactoryComAsyncOperationContext> operation 
        = make_com<RegisterStatefulServiceFactoryComAsyncOperationContext>(*this);
    hr = operation->Initialize(
        root,
        serviceType,
        factory,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}
        
HRESULT ComFabricRuntime::EndRegisterStatefulServiceFactory( 
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(RegisterStatefulServiceFactoryComAsyncOperationContext::End(context));
}

HRESULT ComFabricRuntime::RegisterStatefulServiceFactory( 
    /* [string][in] */ LPCWSTR serviceType,
    /* [in] */ IFabricStatefulServiceFactory *factory)
{
    auto waiter = make_shared<ComAsyncOperationWaiter>();
    ComPointer<IFabricAsyncOperationContext> context;

    HRESULT hr = this->BeginRegisterStatefulServiceFactory(
        serviceType,
        factory,
        0, // use default from the configuration
        waiter->GetAsyncOperationCallback([this, waiter](IFabricAsyncOperationContext * context)
        {
            auto hr = this->EndRegisterStatefulServiceFactory(context);
            waiter->SetHRESULT(hr);
            waiter->Set();
        }),
        context.InitializationAddress());
    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }
    else
    {
        waiter->WaitOne();
        return ComUtility::OnPublicApiReturn(waiter->GetHRESULT());
    }
}
 
HRESULT ComFabricRuntime::CreateServiceGroupFactoryBuilder(
    /* [retval][out] */ IFabricServiceGroupFactoryBuilder ** builder)
{
    ComPointer<IFabricCodePackageActivationContext> activationContext;

    auto hr = ApplicationHostContainer::FabricGetActivationContext(IID_IFabricCodePackageActivationContext, activationContext.VoidInitializationAddress());

    if (FAILED(hr))
    {
        // no activation context, we either run in ad-hoc mode and there is not context
        // or this is a CodePackageHost and the CreateServiceGroupFactory should have been detoured
        return ComUtility::OnPublicApiReturn(::CreateServiceGroupFactoryBuilder(nullptr, builder));
    }
    else
    {
        return ComUtility::OnPublicApiReturn(::CreateServiceGroupFactoryBuilder(activationContext.GetRawPointer(), builder));
    }
}

HRESULT ComFabricRuntime::BeginRegisterServiceGroupFactory( 
    /* [in] */ LPCWSTR groupServiceType,
    /* [in] */ IFabricServiceGroupFactory *factory,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    ComPointer<IUnknown> rootCPtr;
    HRESULT hr = this->QueryInterface(IID_IUnknown, (void**)rootCPtr.InitializationAddress());
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    ComponentRootSPtr root = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));

    ComPointer<RegisterServiceGroupFactoryComAsyncOperationContext> operation 
        = make_com<RegisterServiceGroupFactoryComAsyncOperationContext>(*this);
    hr = operation->Initialize(
        root,
        groupServiceType,
        factory,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}
        
HRESULT ComFabricRuntime::EndRegisterServiceGroupFactory( 
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(RegisterServiceGroupFactoryComAsyncOperationContext::End(context));
}

HRESULT ComFabricRuntime::RegisterServiceGroupFactory(
    /* [string][in] */ LPCWSTR groupServiceType,
    /* [retval][out] */ IFabricServiceGroupFactory * factory)
{
    auto waiter = make_shared<ComAsyncOperationWaiter>();
    ComPointer<IFabricAsyncOperationContext> context;

    HRESULT hr = this->BeginRegisterServiceGroupFactory(
        groupServiceType,
        factory,
        0, // use default from the configuration,
        waiter->GetAsyncOperationCallback([this, waiter](IFabricAsyncOperationContext * context)
        {
            auto hr = this->EndRegisterServiceGroupFactory(context);
            waiter->SetHRESULT(hr);
            waiter->Set();
        }),
        context.InitializationAddress());
    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }
    else
    {
        waiter->WaitOne();
        return ComUtility::OnPublicApiReturn(waiter->GetHRESULT());
    }
}
