// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{

//
// These settings are used by system services both in Fabric.exe and in their own activated hosts.
// See prod\src\ktllogger\KtlLoggerConfig.h for settings used by RA
//

#define SL_GLOBAL_SETTINGS_COUNT 0
#define SL_OVERRIDABLE_STATIC_SETTINGS_COUNT 7
#define SL_OVERRIDABLE_DYNAMIC_SETTINGS_COUNT 0
#define SL_OVERRIDABLE_SETTINGS_COUNT (SL_OVERRIDABLE_STATIC_SETTINGS_COUNT + SL_OVERRIDABLE_DYNAMIC_SETTINGS_COUNT)

#define SL_SETTINGS_COUNT (SL_GLOBAL_SETTINGS_COUNT + SL_OVERRIDABLE_SETTINGS_COUNT)

#define DEFINE_SL_OVERRIDEABLE_CONFIG_PROPERTIES() \
    __declspec(property(get=get_ContainerPath,put=set_ContainerPath)) std::wstring ContainerPath ; \
    std::wstring get_ContainerPath() const; \
    void set_ContainerPath(std::wstring const &); \
    __declspec(property(get=get_ContainerId,put=set_ContainerId)) std::wstring ContainerId ; \
    std::wstring get_ContainerId() const; \
    void set_ContainerId(std::wstring const &); \
    __declspec(property(get=get_LogSize)) int64 LogSize ; \
    int64 get_LogSize() const; \
    __declspec(property(get=get_MaximumNumberStreams)) int MaximumNumberStreams ; \
    int get_MaximumNumberStreams() const; \
    __declspec(property(get=get_MaximumRecordSize)) int MaximumRecordSize ; \
    int get_MaximumRecordSize() const; \
    __declspec(property(get=get_CreateFlags)) uint CreateFlags; \
    uint get_CreateFlags() const; \
    __declspec(property(get=get_EnableHashSharedLogIdFromPath)) bool EnableHashSharedLogIdFromPath; \
    bool get_EnableHashSharedLogIdFromPath() const; \

#define DEFINE_SL_GLOBAL_CONFIG_PROPERTIES() \

#define DEFINE_GET_SL_CONFIG_METHOD() \
    void GetTransactionalReplicatorSharedLogSettingsStructValues(TxnReplicator::SLConfigValues & config) const \
    { \
        config.ContainerPath = this->ContainerPath; \
        config.ContainerId = this->ContainerId; \
        config.LogSize = this->LogSize; \
        config.MaximumNumberStreams = this->MaximumNumberStreams; \
        config.MaximumRecordSize = this->MaximumRecordSize; \
        config.CreateFlags = this->CreateFlags; \
        config.EnableHashSharedLogIdFromPath = this->EnableHashSharedLogIdFromPath; \
    }; \

#define DEFINE_SL_PRIVATE_CONFIG_MEMBERS() \
    std::wstring containerPath_; \
    std::wstring containerId_; \
    int64 logSize_; \
    int maximumNumberStreams_; \
    int maximumRecordSize_; \
    uint createFlags_; \
    bool hashSharedLogIdFromPath_; \

#define DEFINE_SL_PRIVATE_GLOBAL_CONFIG_MEMBERS() \

// Default values for system services
//
#define SL_CONFIG_PROPERTIES(section_name) \
    INTERNAL_CONFIG_ENTRY(std::wstring, section_name, ContainerPath, L"", Common::ConfigEntryUpgradePolicy::Static); \
    INTERNAL_CONFIG_ENTRY(std::wstring, section_name, ContainerId, L"", Common::ConfigEntryUpgradePolicy::Static); \
    INTERNAL_CONFIG_ENTRY(int64, section_name, LogSize, 1 * 1024 * 1024 * 1024, Common::ConfigEntryUpgradePolicy::Static); \
    INTERNAL_CONFIG_ENTRY(int, section_name, MaximumNumberStreams, 24 * 1024, Common::ConfigEntryUpgradePolicy::Static); \
    INTERNAL_CONFIG_ENTRY(int, section_name, MaximumRecordSize, 0, Common::ConfigEntryUpgradePolicy::Static); \
    INTERNAL_CONFIG_ENTRY(uint, section_name, CreateFlags, 0, Common::ConfigEntryUpgradePolicy::Static); \
    INTERNAL_CONFIG_ENTRY(bool, section_name, EnableHashSharedLogIdFromPath, false, Common::ConfigEntryUpgradePolicy::Static); \
    \
    DEFINE_GET_SL_CONFIG_METHOD() \

}
