// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class ComponentConfig
        : public TextTraceComponent<TraceTaskCodes::Config>
        , public IConfigUpdateSink
    {
        DENY_COPY(ComponentConfig);

    public:
        virtual ~ComponentConfig();

        __declspec(property(get=get_Name)) StringLiteral Name;
        StringLiteral get_Name() const { return name_; }

        virtual void Initialize() {}

        void AddEntry(std::wstring const & section, ConfigEntryBase & entry);

        void GetKeyValues(std::wstring const & section, StringMap & entries) const
        {
            config_.GetKeyValues(section, entries);
        }

        Config & FabricGetConfigStore()
        {
            return config_;
        }

        const Config & FabricGetConfigStore() const
        {
            return config_;
        }

        virtual bool OnUpdate(std::wstring const & section, std::wstring const & key);
        virtual bool CheckUpdate(std::wstring const & section, std::wstring const & key, std::wstring const & value, bool isEncrypted);

        std::wstring const & GetTraceId() const override
        {
            return traceId_;
        }

    protected:
        ComponentConfig(StringLiteral name);
        ComponentConfig(ConfigStoreSPtr const & store, StringLiteral name);

    private:
        class ComponentConfigSection
        {
        public:
            ComponentConfigSection(std::wstring const & name)
                : name_(name)
            {
            }

            __declspec(property(get=get_Name)) std::wstring const & Name;
            std::wstring const & get_Name() const { return name_; }

            void AddEntry(ConfigEntryBase* entry);

            void ClearCachedValue();

            bool OnUpdate(std::wstring const & key);
            bool CheckUpdate(std::wstring const & key, std::wstring const & value, bool isEncrypted);

        private:
            std::wstring name_;
            std::vector<ConfigEntryBase*> entries_;
        };

        typedef std::unique_ptr<ComponentConfigSection> ComponentConfigSectionUPtr;

    protected:
        mutable RwLock configLock_;
        Config config_;
        std::wstring traceId_;

    private:
        StringLiteral name_;
        mutable std::vector<ComponentConfigSectionUPtr> sections_;
    };

// Clang doesn't concatenate as MSVC does, 
// so L#property_name becomes L "PropertyName", not L"PropertyName"
#define COMPONENT_STR_CONCAT(x , y) x ## y
#define PUBLIC_CONFIG_ENTRY(type, section_name, property_name, default_value, upgrade_policy, ...)   \
public: \
    __declspec(property(get=get_##property_name##Entry)) Common::ConfigEntry<type> & property_name##Entry;  \
    inline Common::ConfigEntry<type> & get_##property_name##Entry() const   \
    {   \
        if (property_name##_.HasValue == false) \
        {   \
            Common::AcquireWriteLock grab(configLock_);   \
            if (property_name##_.HasValue == false) \
            {   \
                property_name##_.Load(this, section_name, COMPONENT_STR_CONCAT(L, #property_name), default_value, upgrade_policy, ##__VA_ARGS__);    \
            }   \
        }   \
        return property_name##_;    \
    }   \
    __declspec(property(get=get_##property_name, put=set_##property_name)) type property_name;  \
    inline type get_##property_name() const \
    {   \
__pragma(warning(push)) \
__pragma(warning(disable: 4127)) \
        if (upgrade_policy == Common::ConfigEntryUpgradePolicy::Dynamic && (Common::is_blittable<type>::value == false || sizeof(type) > sizeof(void*))) \
__pragma(warning(pop))  \
        {   \
            Common::AcquireWriteLock grab(configLock_);   \
            if (property_name##_.HasValue == false) \
            {   \
                property_name##_.Load(this, section_name, COMPONENT_STR_CONCAT(L, #property_name), default_value, upgrade_policy, ##__VA_ARGS__);    \
            }                                                                                               \
            return property_name##_.GetValue(); \
        }   \
        else    \
        {   \
            return get_##property_name##Entry().GetValue(); \
        }   \
    }   \
    inline void set_##property_name(type const & value) \
    {   \
    Common::AcquireWriteLock grab(configLock_);   \
    property_name##_.SetValue(this, section_name, COMPONENT_STR_CONCAT(L, #property_name), value, default_value, upgrade_policy, ##__VA_ARGS__); \
    }   \
private:    \
    mutable Common::ConfigEntry<type> property_name##_;

#define INTERNAL_CONFIG_ENTRY(type, section_name, property_name, default_value, upgrade_policy, ...)   \
    PUBLIC_CONFIG_ENTRY(type, section_name, property_name, default_value, upgrade_policy, ##__VA_ARGS__)

#define PRIVATE_CONFIG_ENTRY(type, section_name, property_name, default_value, upgrade_policy, ...)   \
    PUBLIC_CONFIG_ENTRY(type, section_name, property_name, default_value, upgrade_policy, ##__VA_ARGS__)

#define TEST_CONFIG_ENTRY(type, section_name, property_name, default_value, upgrade_policy, ...)   \
    INTERNAL_CONFIG_ENTRY(type, section_name, property_name, default_value, upgrade_policy, ##__VA_ARGS__)

#define DEPRECATED_CONFIG_ENTRY(type, section_name, property_name, default_value, upgrade_policy, ...)   \
    PUBLIC_CONFIG_ENTRY(type, section_name, property_name, default_value, upgrade_policy, ##__VA_ARGS__)

#define PUBLIC_CONFIG_GROUP(type, section_name, property_name, upgrade_policy)  \
public: \
    __declspec(property(get=get_##property_name##Entry)) Common::ConfigGroup<type> & property_name##Entry;  \
    inline Common::ConfigGroup<type> & get_##property_name##Entry() const \
    {   \
        if (property_name##_.HasValue == false) \
        { \
            Common::AcquireWriteLock grab(configLock_); \
            if (property_name##_.HasValue == false) \
            { \
                property_name##_.Load(this, section_name, upgrade_policy); \
            } \
            return property_name##_; \
        } \
        return property_name##_; \
    } \
    __declspec(property(get=get_##property_name, put=set_##property_name)) type property_name; \
    inline type const & get_##property_name() const \
    { \
__pragma(warning(push)) \
__pragma(warning(disable: 4127)) \
        if (upgrade_policy == Common::ConfigEntryUpgradePolicy::Dynamic) \
__pragma(warning(pop)) \
        { \
            Common::AcquireWriteLock grab(configLock_); \
            if (property_name##_.HasValue == false) \
            { \
                property_name##_.Load(this, section_name, upgrade_policy); \
            } \
            return property_name##_.GetValue(); \
        } \
        else \
        { \
            return get_##property_name##Entry().GetValue(); \
        } \
    } \
    inline void set_##property_name(type const & value) \
    {   \
        Common::AcquireWriteLock grab(configLock_);   \
        property_name##_.SetValue(this, section_name, value, upgrade_policy);  \
    }   \
private:    \
    mutable Common::ConfigGroup<type> property_name##_;

#define INTERNAL_CONFIG_GROUP(type, section_name, property_name, upgrade_policy)  \
    PUBLIC_CONFIG_GROUP(type, section_name, property_name, upgrade_policy)

#define TEST_CONFIG_GROUP(type, section_name, property_name, upgrade_policy)  \
    INTERNAL_CONFIG_GROUP(type, section_name, property_name, upgrade_policy)

#define DECLARE_COMPONENT_CONFIG(TConfig, Name)   \
    DENY_COPY(TConfig)  \
public: \
    TConfig(Common::ConfigStoreSPtr const & store) : Common::ComponentConfig(store, Name)   \
    {   \
    Initialize();   \
    }   \
    TConfig() : Common::ComponentConfig(Name)   \
    {   \
    Initialize();   \
    }   \
    virtual ~TConfig() \
    { \
    config_.UnregisterForUpdate(this); \
    }

#define DECLARE_SINGLETON_COMPONENT_CONFIG(TConfig, Name)   \
    DENY_COPY(TConfig)  \
public: \
    TConfig(Common::ConfigStoreSPtr const & store) : Common::ComponentConfig(store, Name)   \
    {   \
    Initialize();   \
    }   \
    TConfig() : Common::ComponentConfig(Name)   \
    {   \
    Initialize();   \
    }   \
    virtual ~TConfig() \
    { \
    config_.UnregisterForUpdate(this); \
    } \
    static TConfig & GetConfig();   \
    static void Test_Reset();   \
private:    \
    static BOOL CALLBACK InitConfigFunction(PINIT_ONCE, PVOID, PVOID *);    \
    static INIT_ONCE initOnceConfig_;   \
    static Common::Global<TConfig> singletonConfig_;

#define DEFINE_SINGLETON_COMPONENT_CONFIG(TConfig)  \
    INIT_ONCE TConfig::initOnceConfig_; \
    Global<TConfig> TConfig::singletonConfig_;  \
    BOOL CALLBACK TConfig::InitConfigFunction(PINIT_ONCE, PVOID, PVOID *)   \
    {   \
    singletonConfig_ = Common::make_global<TConfig>();  \
    return TRUE;    \
    }   \
    TConfig & TConfig::GetConfig()  \
    {   \
    PVOID lpContext = NULL; \
    BOOL result = ::InitOnceExecuteOnce(&TConfig::initOnceConfig_, TConfig::InitConfigFunction, NULL, &lpContext);  \
    ASSERT_IF(!result, "Failed to initialize {0} singleton", L ## #TConfig);    \
    return *(TConfig::singletonConfig_);    \
    }   \
    void TConfig::Test_Reset()  \
    {   \
    singletonConfig_ = Common::make_global<TConfig>();  \
    }
}
