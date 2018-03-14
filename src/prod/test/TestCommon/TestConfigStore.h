// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TestCommon
{
    class TestConfigStore : public Common::ConfigStore
    {
        class UpdateSink : Common::IConfigUpdateSink
        {
        public:
            UpdateSink()
                : store_(nullptr), traceId_(L"TestConfigStore::UpdateSink")
            {
            }

            ~UpdateSink()
            {
                if (store_)
                {
                    store_->store_->UnregisterForUpdate(this);
                }
            }

            void Register(TestConfigStore & store)
            {
                store_ = &store;
                store_->store_->RegisterForUpdate(L"", this);
            }

            bool OnUpdate(std::wstring const & section, std::wstring const & key)
            {
                return store_->OnUpdate(section, key);
            }

            bool CheckUpdate(std::wstring const & section, std::wstring const & key, std::wstring const & value, bool isEncrypted)
            {
                return store_->CheckUpdate(section, key, value, isEncrypted);
            }

            const std::wstring & GetTraceId() const
            {
                return traceId_;
            }

        private:
            TestConfigStore* store_;
            std::wstring traceId_;
        };

        DENY_COPY(TestConfigStore)

    public:
        TestConfigStore(Common::ConfigStoreSPtr const & store);

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

        bool UpdateConfig(Common::StringCollection const & settings);
        bool SetConfig(Common::StringCollection const & settings);
        bool RemoveSection(std::wstring const & section);

    private:
        Common::ConfigStoreSPtr store_;
        Common::ConfigSettingsConfigStore overrides_;
        std::set<std::wstring> resetSections_;
        UpdateSink sink_;
        mutable Common::RwLock lock_;

        bool Set(Common::StringCollection const & settings, bool update);
    };
};
