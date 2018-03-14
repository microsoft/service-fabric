// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "ServiceGroup.Constants.h"
#include "ServiceGroup.Runtime.h"
#include "ServiceGroup.StatelessServiceFactory.h"
#include "ServiceGroup.StatefulServiceFactory.h"

using namespace ServiceGroup;

//
// Constructor/Destructor.
//
CServiceGroupFactoryBuilder::CServiceGroupFactoryBuilder(__in IFabricCodePackageActivationContext * activationContext) :
    activationContext_(NULL)
{
    WriteNoise(
        TraceSubComponentServiceGroupFactoryBuilder, 
        "this={0} - ctor, activationContext={1}", 
        this, 
        (NULL != activationContext)
        );

    if (activationContext != NULL)
    {
        activationContext_ = activationContext;
        activationContext_->AddRef();
    }
}

CServiceGroupFactoryBuilder::~CServiceGroupFactoryBuilder(void)
{
    WriteNoise(TraceSubComponentServiceGroupFactoryBuilder, "this={0} - dtor", this);

    std::map<std::wstring, IFabricStatelessServiceFactory*>::iterator iterateStateless;
    for (iterateStateless = statelessServiceFactories_.begin(); statelessServiceFactories_.end() != iterateStateless; iterateStateless++)
    {
        iterateStateless->second->Release();
    }
    std::map<std::wstring, IFabricStatefulServiceFactory*>::iterator iterateStateful;
    for (iterateStateful = statefulServiceFactories_.begin(); statefulServiceFactories_.end() != iterateStateful; iterateStateful++)
    {
        iterateStateful->second->Release();
    }

    if (activationContext_ != NULL)
    {
        activationContext_->Release();
        activationContext_ = NULL;
    }
}

//
// IFabricServiceGroupFactoryBuilder methods.
//
STDMETHODIMP CServiceGroupFactoryBuilder::AddStatelessServiceFactory(
    __in LPCWSTR memberServiceType, 
    __in IFabricStatelessServiceFactory * factory)
{
    if (NULL == memberServiceType || NULL == factory)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    HRESULT hr = S_OK;

    Common::AcquireWriteLock lock(lock_);

    ServiceGroupCommonEventSource::GetEvents().Info_0_ServiceGroupFactoryBuilder(reinterpret_cast<uintptr_t>(this));

    std::pair<std::map<std::wstring, IFabricStatelessServiceFactory*>::iterator, bool> insert;
    try
    { 
        std::wstring key(memberServiceType);
        auto error = Common::ParameterValidator::IsValid(memberServiceType, Common::ParameterValidator::MinStringSize, Common::ParameterValidator::MaxNameSize);
        if (!error.IsSuccess())
        {
            ServiceGroupCommonEventSource::GetEvents().Error_0_ServiceGroupFactoryBuilder(reinterpret_cast<uintptr_t>(this));
        
            return error.ToHResult();
        }
        if (statefulServiceFactories_.end() != statefulServiceFactories_.find(key))
        {
            ServiceGroupCommonEventSource::GetEvents().Error_1_ServiceGroupFactoryBuilder(
                reinterpret_cast<uintptr_t>(this),
                memberServiceType
                );
    
            return Common::ComUtility::OnPublicApiReturn(E_INVALIDARG);
        }
        insert = statelessServiceFactories_.insert(std::pair<std::wstring, IFabricStatelessServiceFactory*>(key, factory));
    }
    catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
    catch (...) { hr = E_FAIL; }
    if (FAILED(hr))
    {
        ServiceGroupCommonEventSource::GetEvents().Error_2_ServiceGroupFactoryBuilder(
            reinterpret_cast<uintptr_t>(this),
            memberServiceType
            );

        return Common::ComUtility::OnPublicApiReturn(hr);
    }

    if (!insert.second)
    {
        ServiceGroupCommonEventSource::GetEvents().Error_3_ServiceGroupFactoryBuilder(
            reinterpret_cast<uintptr_t>(this),
            memberServiceType
            );
        
        return Common::ComUtility::OnPublicApiReturn(E_INVALIDARG);
    }

    factory->AddRef();

    ServiceGroupCommonEventSource::GetEvents().Info_1_ServiceGroupFactoryBuilder(
        reinterpret_cast<uintptr_t>(this),
        memberServiceType
        );

    return S_OK;
}

STDMETHODIMP CServiceGroupFactoryBuilder::AddStatefulServiceFactory(
    __in LPCWSTR memberServiceType, 
    __in IFabricStatefulServiceFactory * factory)
{
    if (NULL == memberServiceType || NULL == factory)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    HRESULT hr = S_OK;

    Common::AcquireWriteLock lock(lock_);

    ServiceGroupCommonEventSource::GetEvents().Info_2_ServiceGroupFactoryBuilder(reinterpret_cast<uintptr_t>(this));

    std::pair<std::map<std::wstring, IFabricStatefulServiceFactory*>::iterator, bool> insert;
    try
    { 
        std::wstring key(memberServiceType);
        auto error = Common::ParameterValidator::IsValid(memberServiceType, Common::ParameterValidator::MinStringSize, Common::ParameterValidator::MaxNameSize);
        if (!error.IsSuccess())
        {
            ServiceGroupCommonEventSource::GetEvents().Error_0_ServiceGroupFactoryBuilder(reinterpret_cast<uintptr_t>(this));
            
            return error.ToHResult();
        }
        if (statelessServiceFactories_.end() != statelessServiceFactories_.find(key))
        {
            ServiceGroupCommonEventSource::GetEvents().Error_4_ServiceGroupFactoryBuilder(
                reinterpret_cast<uintptr_t>(this),
                memberServiceType
                );
    
            return Common::ComUtility::OnPublicApiReturn(E_INVALIDARG);
        }
        insert = statefulServiceFactories_.insert(std::pair<std::wstring, IFabricStatefulServiceFactory*>(key, factory)); 
    }
    catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
    catch (...) { hr = E_FAIL; }
    if (FAILED(hr))
    {
        ServiceGroupCommonEventSource::GetEvents().Error_2_ServiceGroupFactoryBuilder(
            reinterpret_cast<uintptr_t>(this),
            memberServiceType
            );

        return Common::ComUtility::OnPublicApiReturn(hr);
    }

    if (!insert.second)
    {
        ServiceGroupCommonEventSource::GetEvents().Error_3_ServiceGroupFactoryBuilder(
            reinterpret_cast<uintptr_t>(this),
            memberServiceType
            );

        return Common::ComUtility::OnPublicApiReturn(E_INVALIDARG);
    }

    factory->AddRef();

    ServiceGroupCommonEventSource::GetEvents().Info_3_ServiceGroupFactoryBuilder(
        reinterpret_cast<uintptr_t>(this),
        memberServiceType
        );

    return S_OK;
}

STDMETHODIMP CServiceGroupFactoryBuilder::RemoveServiceFactory(__in LPCWSTR memberServiceType)
{
    if (NULL == memberServiceType)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    HRESULT hr = S_OK;

    Common::AcquireWriteLock lock(lock_);

    ServiceGroupCommonEventSource::GetEvents().Info_4_ServiceGroupFactoryBuilder(reinterpret_cast<uintptr_t>(this));

    std::map<std::wstring, IFabricStatelessServiceFactory*>::iterator iterateStateless;
    std::map<std::wstring, IFabricStatefulServiceFactory*>::iterator iterateStateful;
    try
    { 
        std::wstring key(memberServiceType);
        iterateStateless = statelessServiceFactories_.find(key);
        if (statelessServiceFactories_.end() != iterateStateless)
        {
            ServiceGroupCommonEventSource::GetEvents().Info_5_ServiceGroupFactoryBuilder(
                reinterpret_cast<uintptr_t>(this),
                memberServiceType
                );

            iterateStateless->second->Release();
            statelessServiceFactories_.erase(iterateStateless);
            return S_OK;
        }
        iterateStateful = statefulServiceFactories_.find(key);
        if (statefulServiceFactories_.end() != iterateStateful)
        {
            ServiceGroupCommonEventSource::GetEvents().Info_6_ServiceGroupFactoryBuilder(
                reinterpret_cast<uintptr_t>(this),
                memberServiceType
                );
    
            iterateStateful->second->Release();
            statefulServiceFactories_.erase(iterateStateful);
            return S_OK;
        }
    }
    catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
    catch (...) { hr = E_FAIL; }
    if (FAILED(hr))
    {
        ServiceGroupCommonEventSource::GetEvents().Error_5_ServiceGroupFactoryBuilder(
            reinterpret_cast<uintptr_t>(this),
            memberServiceType
            );

        return Common::ComUtility::OnPublicApiReturn(hr);
    }
    return Common::ComUtility::OnPublicApiReturn(E_INVALIDARG);
}

STDMETHODIMP CServiceGroupFactoryBuilder::ToServiceGroupFactory(__out IFabricServiceGroupFactory ** factory)
{
    if (NULL == factory)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    HRESULT hr = S_OK;

    Common::AcquireWriteLock lock(lock_);

    ServiceGroupCommonEventSource::GetEvents().Info_7_ServiceGroupFactoryBuilder(reinterpret_cast<uintptr_t>(this));

    if (!statefulServiceFactories_.empty())
    {
        CStatefulServiceGroupFactory* statefulServiceGroupFactory = new (std::nothrow) CStatefulServiceGroupFactory();
        if (NULL == statefulServiceGroupFactory)
        {
            ServiceGroupCommonEventSource::GetEvents().Error_6_ServiceGroupFactoryBuilder(reinterpret_cast<uintptr_t>(this));
    
            return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
        }
        hr = statefulServiceGroupFactory->FinalConstruct(statefulServiceFactories_, statelessServiceFactories_, activationContext_);
        if (FAILED(hr))
        {
            ServiceGroupCommonEventSource::GetEvents().Error_7_ServiceGroupFactoryBuilder(reinterpret_cast<uintptr_t>(this));
    
            statefulServiceGroupFactory->Release();
            return Common::ComUtility::OnPublicApiReturn(hr);
        }
        *factory = statefulServiceGroupFactory;
    }
    else if (!statelessServiceFactories_.empty())
    {
        CStatelessServiceGroupFactory* statelessServiceGroupFactory = new (std::nothrow) CStatelessServiceGroupFactory();
        if (NULL == statelessServiceGroupFactory)
        {
            ServiceGroupCommonEventSource::GetEvents().Error_8_ServiceGroupFactoryBuilder(reinterpret_cast<uintptr_t>(this));
    
            return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
        }
        hr = statelessServiceGroupFactory->FinalConstruct(statelessServiceFactories_);
        if (FAILED(hr))
        {
            ServiceGroupCommonEventSource::GetEvents().Error_9_ServiceGroupFactoryBuilder(reinterpret_cast<uintptr_t>(this));
    
            statelessServiceGroupFactory->Release();
            return Common::ComUtility::OnPublicApiReturn(hr);
        }
        *factory = statelessServiceGroupFactory;
    }
    else
    {
        return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::InvalidOperation);
    }
    return S_OK;;
}
