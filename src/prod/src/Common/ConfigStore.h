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

    /*
        Interface for providers of configuration stores.
        The providers must allow users to read different type of keys. 

        There are three aspects to the windows fabric configuration subsystem:
        1. Identifying the location of the configuration data
        2. Parsing of the configuration data from that location
        3. The in-memory representation of this parsed data 
        4. Associated functionality such as monitoring for updates etc

        The class below is unfortunately combining both of these aspects
        into a single entity which causes code duplication.

        For (1), there are only a few well defined locations. The core of the logic for
        figuring this out lives in FabricEnvironment::GetStoreTypeAndLocation. There are a
        few additional things that are implemented in PackageConfigStore etc.

        For (2), there are the following formats for configuration:
        1. CFG file: which is an INI style settings file. 
           The parsing for such configuration lives in the FileConfigStore

        2. Xml based file: This is an xml file representing configuration (Settings.xml).
           The parsing for this lives in ServiceModel (SettingsXmlConfigReader)

        For (3), there are only two different representations of the configuration data:
        1. ConfigSettings: These classes are a read-only in-memory representation of the data.
        2. FileConfigStore: The file config store has its own representation of the config data.

        The Configuration Store is created once in a module by the FabricGlobals::GetConfigStore method. 
        This method is provided a factory which understands how to create a config store. In production code
        the factory is implemented in ConfigLoader in FabricCommon. This uses the FabricEnvironment to 
        identify the config store and create the appropriate instance. In unit tests that are below the
        FabricCommon the factory is defined in the FabricGlobalsInitializationFixture which simply 
        assumes a .cfg based config store.

        In future, this design should be refactored. There should exist the following components:
        1. ConfigStoreLocation: something which identifies where the configuration lives. 
        2. ConfigSettings: which is the ONLY in-memory representation of configuration data
        3. IConfigReader: which can take a ConfigStoreLocation and return a ConfigSettings
        4. ConfigStore: the current ConfigStore which is the interface for reading config
        5. InMemoryConfigStore: The current ConfigSettingsConfigStore which stores a ConfigSettings and implements ConfigStore
        6. MonitoredInMemoryConfigStore: This keeps a reader, a location and is an in-memory config store providing the same semantics as today
        
        The FileConfigStore should then become simply a config reader for .ini based config files.

        The SettingsXmlConfigReader is already implemented to follow this pattern.
    */
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
