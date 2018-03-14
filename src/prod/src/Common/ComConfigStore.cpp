// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

#define TraceType "ComConfigStore"

class ComConfigStore::ConfigUpdateSink : public IConfigUpdateSink
{
    DENY_COPY(ConfigUpdateSink)

public:
    ConfigUpdateSink(ComConfigStore * owner)
        : lock_(),
        owner_(owner),
        traceId_(L"ComConfigStore::ConfigUpdateSink")
    {
    }

    virtual ~ConfigUpdateSink()
    {
    }

    void Detach()
    {
        AcquireWriteLock lock(lock_);
        owner_ = NULL;
    }

    const std::wstring & GetTraceId() const override
    {
        return traceId_;
    }

    virtual bool OnUpdate(std::wstring const & section, std::wstring const & key)
    {
        ComPointer<ComConfigStore> snap;
        {
            AcquireReadLock lock(lock_);
            if ((owner_ != NULL) && (owner_->TryAddRef() != 0u))
            {
                snap.SetNoAddRef(owner_);
                
            }
        }

        if (snap.GetRawPointer() != NULL)
        {
            return snap->OnUpdate(section, key);
        }
        else
        {
            return false;
        }
    }

    virtual bool CheckUpdate(std::wstring const & section, std::wstring const & key, std::wstring const & value, bool isEncrypted)
    {
        AcquireReadLock lock(lock_);
        if ((owner_ != NULL) && (owner_->TryAddRef() != 0u))
        {
            ComPointer<ComConfigStore> snap;
            snap.SetNoAddRef(owner_);
            return snap->CheckUpdate(section, key, value, isEncrypted);
        }

        return false;
    }

private:
    wstring traceId_;
    RwLock lock_;
    ComConfigStore * owner_;
};

ComConfigStore::ComConfigStore(ConfigStoreSPtr const & store, ComPointer<IFabricConfigStoreUpdateHandler> const & updateHandler)
    : store_(store),
    updateHandler_(updateHandler),
    updateSink_()
{
    auto updateSink = make_shared<ConfigUpdateSink>(this);
    store->RegisterForUpdate(L"", updateSink.get());
    updateSink_ = move(updateSink);
}

ComConfigStore::~ComConfigStore()
{
    updateSink_->Detach();
    store_->UnregisterForUpdate((IConfigUpdateSink*)updateSink_.get());
}

ULONG STDMETHODCALLTYPE ComConfigStore::TryAddRef(void)
{
    return this->BaseTryAddRef();
}

HRESULT STDMETHODCALLTYPE ComConfigStore::GetSections( 
    /* [in] */ LPCWSTR partialSectionName,
    /* [retval][out] */ IFabricStringListResult **result)
{
    if (partialSectionName == NULL || result == NULL) 
    { 
        return ComUtility::OnPublicApiReturn(E_POINTER); 
    }

    StringCollection sectionNames;
    store_->GetSections(sectionNames, wstring(partialSectionName));
    return ComStringCollectionResult::ReturnStringCollectionResult(move(sectionNames), result);
}
        
HRESULT STDMETHODCALLTYPE ComConfigStore::GetKeys( 
    /* [in] */ LPCWSTR sectionName,
    /* [in] */ LPCWSTR partialKeyName,
    /* [retval][out] */ IFabricStringListResult **result)
{
    if (sectionName == NULL || partialKeyName == NULL || result == NULL) 
    { 
        return ComUtility::OnPublicApiReturn(E_POINTER); 
    }

    StringCollection KeyNames;
    store_->GetKeys(wstring(sectionName), KeyNames, wstring(partialKeyName));
    return ComStringCollectionResult::ReturnStringCollectionResult(move(KeyNames), result);
}
        
HRESULT STDMETHODCALLTYPE ComConfigStore::ReadString( 
    /* [in] */ LPCWSTR section,
    /* [in] */ LPCWSTR key,
    /* [out] */ BOOLEAN * isEncrypted,
    /* [retval][out] */ IFabricStringResult **result)
{
    if (section == NULL || key == NULL || result == NULL) 
    { 
        return ComUtility::OnPublicApiReturn(E_POINTER); 
    }

    bool isEncryptedValue = false;
    wstring strValue = store_->ReadString(wstring(section), wstring(key), isEncryptedValue);
    *isEncrypted = isEncryptedValue ? TRUE : FALSE;
    return ComStringResult::ReturnStringResult(strValue, result);
}

BOOLEAN STDMETHODCALLTYPE ComConfigStore::get_IgnoreUpdateFailures(void)
{
    if (store_->GetIgnoreUpdateFailures())
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
        
void STDMETHODCALLTYPE ComConfigStore::set_IgnoreUpdateFailures(BOOLEAN value)
{
    store_->SetIgnoreUpdateFailures(value ? true : false);
}


bool ComConfigStore::OnUpdate(std::wstring const & section, std::wstring const & key)
{
    // Using isProcessed default as true. If the updateHandler_ is null, we should consider is processed.
    BOOLEAN isProcessed = TRUE;

    // dispatch across COM boundry to ComProxyConfigStore instead of dispatching to base ConfigStore
    if (updateHandler_)
    {
        auto hr = updateHandler_->OnUpdate(section.c_str(), key.c_str(), &isProcessed);
        if (!SUCCEEDED(hr))
        {
            if (hr == HRESULT_FROM_WIN32(ERROR_PROCESS_ABORTED))
            {
                // This HRESULT code can be returned in a managed process in situations where a 
                // configuration update notification happens at the same time as the process is
                // exiting. In such situations, the CLR is shutting down and refuses to dispatch
                // the configuration update notification to the managed component that has 
                // subscribed to it. In these situations we log a warning, but do not assert.
                Trace.WriteWarning(
                    TraceType,
                    "Failed to dispatch configuration update notification. HRESULT={0}", 
                    hr);
            }
            else
            {
                Assert::CodingError("Failed to dispatch configuration update notification. HRESULT={0}", hr);
            }
        }
    }
    
    return isProcessed ? true : false;
}

bool ComConfigStore::CheckUpdate(wstring const & section, wstring const & key, wstring const & value, bool isEncrypted)
{
    // Using isProcessed default as true. If the updateHandler_ is null, we should consider is processed.
    BOOLEAN result = TRUE;

    // dispatch across COM boundry to ComProxyConfigStore instead of dispatching to base ConfigStore
    if (updateHandler_)
    {
        auto hr = updateHandler_->CheckUpdate(section.c_str(), key.c_str(), value.c_str(), isEncrypted, &result);
        if (!SUCCEEDED(hr))
        {
            if (hr == HRESULT_FROM_WIN32(ERROR_PROCESS_ABORTED))
            {
                // This HRESULT code can be returned in a managed process in situations where a 
                // configuration update notification happens at the same time as the process is
                // exiting. In such situations, the CLR is shutting down and refuses to dispatch
                // the configuration update notification to the managed component that has 
                // subscribed to it. In these situations we log a warning, but do not assert.
                Trace.WriteWarning(
                    TraceType,
                    "Failed to dispatch configuration CheckUpdate notification. HRESULT={0}", 
                    hr);
            }
            else
            {
                Assert::CodingError("Failed to dispatch configuration CheckUpdate notification. HRESULT={0}", hr);
            }
        }
    }
    
    return result ? true : false;
}
