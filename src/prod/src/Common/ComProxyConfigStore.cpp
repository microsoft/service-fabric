// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

RwLock SingletonDefaultStoreLock;
ConfigStoreSPtr SingletonDefaultStore(nullptr);

class ComProxyConfigStore::ComConfigStoreUpdateHandler : 
    public IFabricConfigStoreUpdateHandler,
    private ComUnknownBase
{
    DENY_COPY(ComConfigStoreUpdateHandler)

    COM_INTERFACE_LIST1(
        ComConfigStoreUpdateHandler,
        IID_IFabricConfigStoreUpdateHandler,
        IFabricConfigStoreUpdateHandler)

public:
    explicit ComConfigStoreUpdateHandler()
        : ComUnknownBase(),
        weak_owner_(),
        lock_()
    {
    }

    virtual ~ComConfigStoreUpdateHandler()
    {
    }

    HRESULT STDMETHODCALLTYPE OnUpdate(
    /* [in] */ LPCWSTR section,
    /* [in] */ LPCWSTR key,
    /* [retval][out] */ BOOLEAN *isProcessed)
    {
        if ((section == NULL) || (key == NULL) || (isProcessed == NULL))
        {
            return ComUtility::OnPublicApiReturn(E_POINTER);
        }

        *isProcessed = FALSE;

        {
            AcquireReadLock lock(lock_);
            auto owner = weak_owner_.lock();
            if (owner)
            {
                auto comProxyConfigStore = static_cast<ComProxyConfigStore*>(owner.get());
                if (comProxyConfigStore->DispatchUpdate(wstring(section), wstring(key)))
                {
                    *isProcessed = TRUE;
                }
            }
        }

        return ComUtility::OnPublicApiReturn(S_OK);
    }

    HRESULT STDMETHODCALLTYPE CheckUpdate(
    /* [in] */ LPCWSTR section,
    /* [in] */ LPCWSTR key,
    /* [in] */ LPCWSTR value,
    /* [in] */ BOOLEAN isEncrypted,
    /* [retval][out] */ BOOLEAN *canUpdate)
    {
        if ((section == NULL) || (key == NULL) || (value == NULL) || (canUpdate == NULL))
        {
            return ComUtility::OnPublicApiReturn(E_POINTER);
        }

        *canUpdate = FALSE;

        {
            AcquireReadLock lock(lock_);
            auto owner = weak_owner_.lock();
            if (owner)
            {
                auto comProxyConfigStore = static_cast<ComProxyConfigStore*>(owner.get());
                if (comProxyConfigStore->DispatchCheckUpdate(wstring(section), wstring(key), wstring(value), (isEncrypted == TRUE)))
                {
                    *canUpdate = TRUE;
                }
            }
        }

        return ComUtility::OnPublicApiReturn(S_OK);
    }

    void set_Owner(ConfigStoreSPtr const & owner)
    {
        AcquireWriteLock lock(lock_);
        weak_owner_ = owner;
    }

private:
    ConfigStoreWPtr weak_owner_;
    RwLock lock_;
};

ComProxyConfigStore::ComProxyConfigStore(ComPointer<IFabricConfigStore2> const & store)
    : ConfigStore(true), // delegate the update policy to actual store
    store_(store)
{
}

ComProxyConfigStore::~ComProxyConfigStore()
{
}
 
bool ComProxyConfigStore::GetIgnoreUpdateFailures() const
{
    return this->store_->get_IgnoreUpdateFailures() ? true : false;
}

void ComProxyConfigStore::SetIgnoreUpdateFailures(bool value)
{
    this->store_->set_IgnoreUpdateFailures(value ? TRUE : FALSE);
}

wstring ComProxyConfigStore::ReadString(
    wstring const & section,
    wstring const & key,
    __out bool & isEncrypted) const
{
    HRESULT hr;
    ComPointer<IFabricStringResult> result;
    BOOLEAN isEncryptedValue = FALSE;

    hr = store_->ReadString(section.c_str(), key.c_str(), &isEncryptedValue, result.InitializationAddress());
    
    if (SUCCEEDED(hr))
    {
        isEncrypted = isEncryptedValue ? true : false;
        return wstring(result->get_String());
    }

    Assert::CodingError(
        "IFabricConfigStore.ReadString failed with HRESULT={2} for Section={0} and Key={1}.", 
        section, 
        key, 
        hr);
}
       
void ComProxyConfigStore::GetSections(
    StringCollection & sectionNames, 
    wstring const & partialName) const
{
    HRESULT hr;
    ComPointer<IFabricStringListResult> result;

    hr = store_->GetSections(partialName.c_str(), result.InitializationAddress());
    if (SUCCEEDED(hr))
    {
        ULONG itemCount;
        LPCWSTR *items;
        hr = result->GetStrings(&itemCount, (const LPCWSTR**)&items);
        if (SUCCEEDED(hr))
        {
            for(ULONG i = 0; i < itemCount; i++)
            {
                sectionNames.push_back(wstring(items[i]));
            }
            
            return;
        }
    }

    Assert::CodingError(
        "IFabricConfigStore.GetSections failed with HRESULT={1} for PartialSectionName={0}.", 
        partialName, 
        hr);
}

void ComProxyConfigStore::GetKeys(
    wstring const & section,
    StringCollection & keyNames, 
    wstring const & partialName) const
{
    HRESULT hr;
    ComPointer<IFabricStringListResult> result;

    hr = store_->GetKeys(section.c_str(), partialName.c_str(), result.InitializationAddress());
    if (SUCCEEDED(hr))
    {
        ULONG itemCount;
        LPCWSTR *items;
        hr = result->GetStrings(&itemCount, (const LPCWSTR**)&items);
        if (SUCCEEDED(hr))
        {
            for(ULONG i = 0; i < itemCount; i++)
            {
                keyNames.push_back(wstring(items[i]));
            }

            return;
        }
    }

    Assert::CodingError(
        "IFabricConfigStore.GetKeys failed with HRESULT={1} for PartialSectionName={0}.", 
        partialName, 
        hr);
}

ConfigStoreSPtr ComProxyConfigStore::Create()
{
    if (!SingletonDefaultStore)
    {
        AcquireWriteLock grab(SingletonDefaultStoreLock);
        if (!SingletonDefaultStore)
        {
            SingletonDefaultStore = ComProxyConfigStore::Create(
                [](REFIID riid, IFabricConfigStoreUpdateHandler * updateHandler, void ** configStore)->HRESULT
                {
                    return ::FabricGetConfigStore(riid, updateHandler, configStore);
                });
        }
    }

    return SingletonDefaultStore;
}

ConfigStoreSPtr ComProxyConfigStore::Create(GetConfigStoreImpl const & getConfigStore)
{
    HRESULT hr;
    ComPointer<IFabricConfigStore2> storeCPtr;
    
    auto updateHandler = make_com<ComProxyConfigStore::ComConfigStoreUpdateHandler, IFabricConfigStoreUpdateHandler>();
    hr = getConfigStore(IID_IFabricConfigStore2, updateHandler.GetRawPointer(), storeCPtr.VoidInitializationAddress());
    if (FAILED(hr))
    {
        Assert::CodingError("::FabricGetConfigStore failed with HRESULT={0}", hr);
    }

    auto proxyStore = make_shared<ComProxyConfigStore>(storeCPtr);
    (static_cast<ComProxyConfigStore::ComConfigStoreUpdateHandler*>(updateHandler.GetRawPointer()))->set_Owner(proxyStore);

    return proxyStore;
}

ErrorCode ComProxyConfigStore::FabricGetConfigStoreEnvironmentVariable(__out wstring & envVarName, __out wstring & envVarValue)
{
    ComPointer<IFabricStringResult> envVarNameResult;
    ComPointer<IFabricStringResult> envVarValueResult;

    auto hr = ::FabricGetConfigStoreEnvironmentVariable(
        envVarNameResult.InitializationAddress(), 
        envVarValueResult.InitializationAddress());
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    envVarName = wstring(envVarNameResult->get_String());
    envVarValue = wstring(envVarValueResult->get_String());
    return ErrorCode(ErrorCodeValue::Success);
}

bool ComProxyConfigStore::DispatchUpdate(wstring const & section, wstring const & key)
{
    return ConfigStore::OnUpdate(section, key);
}

bool ComProxyConfigStore::DispatchCheckUpdate(wstring const & section, wstring const & key, wstring const & value, bool isEncrypted)
{
    return ConfigStore::CheckUpdate(section, key, value, isEncrypted);
}
