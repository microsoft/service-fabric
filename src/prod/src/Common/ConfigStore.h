// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class ConfigStore;
    typedef std::shared_ptr<ConfigStore> ConfigStoreSPtr;
    typedef std::weak_ptr<ConfigStore> ConfigStoreWPtr;

    namespace ConfigStoreType
    {
        enum Enum
        {
            // Unknwon
            Unknown = 0,

            // CFG/INI style settings loaded from FabricConfig.cfg or other CFG files 
            Cfg = 1, 

            // Deployment package settings loaded from Faric.Config.x\Settings.xml
            Package = 2,

            // the settings are in the file that is of the format Settings.xml
            SettingsFile = 3,

            // empty settings when CFG and Package stores are not located
            None = 4,
        };

        std::wstring ToString(Enum const & val);
        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
    };

    // Interface for providers of configuration stores.
    // The providers must allow users to read different type of keys. 
    class ConfigStore
        : public std::enable_shared_from_this<ConfigStore>
        , public Common::TextTraceComponent<Common::TraceTaskCodes::Common>
    {
        DENY_COPY(ConfigStore)

    public:
        ConfigStore();
        ConfigStore(bool ignoreUpdateFailures);
        virtual ~ConfigStore();

        virtual bool GetIgnoreUpdateFailures() const;

        virtual void SetIgnoreUpdateFailures(bool value);

        void RegisterForUpdate(std::wstring const & section, IConfigUpdateSink* sink);

        void UnregisterForUpdate(IConfigUpdateSink const* sink);

        bool CheckUpdate(std::wstring const & section, std::wstring const & key, std::wstring const & value, bool isEncrypted);

        virtual std::wstring ReadString(
            std::wstring const & section,
            std::wstring const & key,
            __out bool & isEncrypted) const = 0;

        virtual void GetSections(
            Common::StringCollection & sectionNames,
            std::wstring const & partialName = L"") const = 0;

        virtual void GetKeys(
            std::wstring const & section,
            Common::StringCollection & keyNames,
            std::wstring const & partialName = L"") const = 0;

    protected:
        bool OnUpdate(std::wstring const & section, std::wstring const & key);

    private:
        static Common::GlobalWString CfgStore_EnvironmentVarName;
        static Common::GlobalWString CfgStore_DefaultFileName;
        static Common::GlobalWString PackageStore_EnvironmentVarName;
        static Common::GlobalWString PackageStore_DefaultFileName;

        bool InvokeOnSubscribers(
            std::wstring const & action,
            std::function<bool(IConfigUpdateSink *)> const & func) const;

    private:
        class UpdateSubscriberMap
        {
        public:
            void Register(std::wstring const & action, IConfigUpdateSink* sink);

            std::vector<std::wstring> UnRegister(IConfigUpdateSink const* sink);

            IConfigUpdateSinkList Get(std::wstring const & section) const;

        private:
            class ConfigUpdateSinkRegistration : public IConfigUpdateSink
            {
            public:
                ConfigUpdateSinkRegistration(IConfigUpdateSink* sink) :
                    sink_(sink)
                {
                }

                const std::wstring & GetTraceId() const override
                {
                    return sink_->GetTraceId();
                }

                bool OnUpdate(std::wstring const & section, std::wstring const & key) override
                {
                    Common::AcquireExclusiveLock grab(lock_);
                    return sink_->OnUpdate(section, key);
                }

                bool CheckUpdate(std::wstring const & section, std::wstring const & key, std::wstring const & value, bool isEncrypted) override
                {
                    Common::AcquireExclusiveLock grab(lock_);
                    return sink_->CheckUpdate(section, key, value, isEncrypted);
                }

                bool operator==(IConfigUpdateSink const * sink) const
                {
                    return sink_ == sink;
                }

                bool operator==(IConfigUpdateSink * sink) const
                {
                    return sink_ == sink;
                }

            private:
                Common::ExclusiveLock lock_;
                IConfigUpdateSink * sink_;
            };

            std::multimap<std::wstring, std::shared_ptr<ConfigUpdateSinkRegistration>> map_;
        };

        mutable RwLock lock_;
        UpdateSubscriberMap updateSubscriberMap_;
        std::wstring traceId_;
        bool ignoreUpdateFailures_;
    };
} // end namespace Common
