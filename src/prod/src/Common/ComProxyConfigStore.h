// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class ComProxyConfigStore : public ConfigStore
    {
        DENY_COPY(ComProxyConfigStore)

    public:
        typedef std::function<HRESULT(REFIID, IFabricConfigStoreUpdateHandler *, void **)> GetConfigStoreImpl;

    public:
        explicit ComProxyConfigStore(ComPointer<IFabricConfigStore2> const & store);
        virtual ~ComProxyConfigStore();

        virtual bool GetIgnoreUpdateFailures() const;

        virtual void SetIgnoreUpdateFailures(bool value);

        virtual std::wstring ReadString(
            std::wstring const & section,
            std::wstring const & key,
            __out bool & isEncrypted) const; 
       
        virtual void GetSections(
            Common::StringCollection & sectionNames, 
            std::wstring const & partialName = L"") const;

        virtual void GetKeys(
            std::wstring const & section,
            Common::StringCollection & keyNames, 
            std::wstring const & partialName = L"") const;

        static ConfigStoreSPtr Create();
        static ConfigStoreSPtr Create(GetConfigStoreImpl const & getConfigStore);
        static ErrorCode FabricGetConfigStoreEnvironmentVariable(__out std::wstring & envVarName, __out std::wstring & envVarValue);

    private:
        bool DispatchUpdate(std::wstring const & section, std::wstring const & key);
        bool DispatchCheckUpdate(std::wstring const & section, std::wstring const & key, std::wstring const & value, bool isEncrypted);
    
    private:
        class ComConfigStoreUpdateHandler;
        ComPointer<IFabricConfigStore2> const store_;
    };
}
