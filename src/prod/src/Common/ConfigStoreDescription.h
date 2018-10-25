//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

namespace Common
{
    // Represents a loaded config store
    class ConfigStoreDescription
    {
    public:
        ConfigStoreDescription(
            ConfigStoreSPtr const & store,
            std::wstring const & storeEnvironmentVariableName,
            std::wstring const & storeEnvironmentVariableValue) :
            store_(store),
            storeEnvironmentVariableName_(storeEnvironmentVariableName),
            storeEnvironmentVariableValue_(storeEnvironmentVariableValue)
        {
        }

        __declspec(property(get = get_Store)) Common::ConfigStoreSPtr const & Store;
        Common::ConfigStoreSPtr const & get_Store() const { return store_; }

        __declspec(property(get = get_StoreEnvironmentVariableName)) std::wstring const & StoreEnvironmentVariableName;
        std::wstring const & get_StoreEnvironmentVariableName() const { return storeEnvironmentVariableName_; }

        __declspec(property(get = get_StoreEnvironmentVariableValue)) std::wstring const & StoreEnvironmentVariableValue;
        std::wstring const & get_StoreEnvironmentVariableValue() const { return storeEnvironmentVariableValue_; }

    private:
        ConfigStoreSPtr store_;
        std::wstring storeEnvironmentVariableName_;
        std::wstring storeEnvironmentVariableValue_;
    };

    typedef std::unique_ptr<ConfigStoreDescription> ConfigStoreDescriptionUPtr;
}