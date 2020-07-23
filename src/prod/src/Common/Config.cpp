// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Common
{
    using namespace std;
    using Common::Environment;
    using Common::StringUtility;

    volatile LONG Config::assertAndTracingConfigInitialized = 0;

    shared_ptr<Config> Config::Create(HMODULE dllModule)
    {
        shared_ptr<Config> result(new Config(dllModule));
        return result;
    }

    Config::Config(ConfigStoreSPtr const & store)
        : store_(store)
    {
        try
        {
            InitAssertAndTracingConfig();
        }
        catch(...)
        {
            store_ = nullptr;
            throw;
        } 
    }

    Config::Config(HMODULE dllModule)
        : store_()
    {
        UNREFERENCED_PARAMETER(dllModule);

        try
        {
            store_ = InitConfigStore();
            InitAssertAndTracingConfig();
        }
        catch(...)
        {
            store_ = nullptr;
            throw;
        }
    }

    Config::~Config()
    {
    }

    void Config::GetKeyValues(std::wstring const & section, StringMap & entries) const
    {
        StringCollection keys;
        GetKeys(section, keys);

        for (std::wstring const & key : keys)
        {                
            bool isEncrypted;
            entries[key] = ReadString(section, key, isEncrypted);
        }
    }

    void Config::InitAssertAndTracingConfig()
    {
        if (InterlockedIncrement(&assertAndTracingConfigInitialized) == 1)
        {
            Assert::LoadConfiguration(*this);
            TraceProvider::LoadConfiguration(*this);
        }
    }

    ConfigStoreSPtr Config::InitConfigStore()
    {
        ConfigStoreSPtr store = defaultStore_;

        if (!store)
        {
            store = ComProxyConfigStore::Create();
        }

        return store;
    }

    ConfigStoreSPtr Config::defaultStore_ = nullptr;

    void Config::SetConfigStore(ConfigStoreSPtr const & store)
    {
        defaultStore_ = store;
    }
}
