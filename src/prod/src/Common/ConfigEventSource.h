// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    #define CONFIG_STRUCTURED_TRACE(trace_name, trace_id, trace_level, ...) \
        STRUCTURED_TRACE(trace_name, Config, trace_id, trace_level, __VA_ARGS__)

    class ConfigEventSource
    { 
    public:
        DECLARE_STRUCTURED_TRACE(ConfigStoreInitialized, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE(FileConfigStoreFileNotFound, std::wstring);
        DECLARE_STRUCTURED_TRACE(FileConfigStoreFileReadError, std::wstring);
        DECLARE_STRUCTURED_TRACE(FileConfigStoreFileReadSuccess, std::wstring);
        DECLARE_STRUCTURED_TRACE(PackageConfigStoreSettingsLoaded, std::wstring);
        DECLARE_STRUCTURED_TRACE(XmlSettingsStoreSettingsLoaded, std::wstring);
        DECLARE_STRUCTURED_TRACE(ValueUpdated, Common::StringLiteral, std::wstring, std::wstring, bool);
        DECLARE_STRUCTURED_TRACE(RegisteredForUpdate, std::wstring, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE(UnregisteredForUpdate, std::wstring, std::wstring, std::wstring);
    public:
        ConfigEventSource() :
        CONFIG_STRUCTURED_TRACE(
            ConfigStoreInitialized,
            7,
            Info,
            "Configuration store {0} initialized from location {1}.",
            "storeType",
            "storeLocation"),
        CONFIG_STRUCTURED_TRACE(
            FileConfigStoreFileNotFound,
            4,
            Warning,
            "The configuration file {0} doesn't exist. Skip parsing, default config values will be used.",
            "pathName"),
        CONFIG_STRUCTURED_TRACE(
            FileConfigStoreFileReadError,
            5,
            Warning,
            "Error reading configuration file {0}. Skip parsing, default config values will be used.",
            "pathName"),
        CONFIG_STRUCTURED_TRACE(
            FileConfigStoreFileReadSuccess,
            6,
            Info,
            "Read config from file {0}",
            "pathName"),
        CONFIG_STRUCTURED_TRACE(
            PackageConfigStoreSettingsLoaded,
            8,
            Info,
            "Configuration settings loaded from file {0}",
            "settingsFilePath"),
        CONFIG_STRUCTURED_TRACE(
            XmlSettingsStoreSettingsLoaded,
            11,
            Info,
            "XmlSettings file ({0}) loaded",
            "name"),
        CONFIG_STRUCTURED_TRACE(
            ValueUpdated,
            12,
            Info,
            "Loaded property {1} with value {2}. IsPropertyValueEncrypted = {3}.", 
            "id",
            "key",
            "value",
            "isEncrypted"),
        CONFIG_STRUCTURED_TRACE(
            RegisteredForUpdate, 
            13,
            Info,
            "{0} registered for updates on {1}. Subscriber: {2}",
            "id",
            "section",
            "subscriber"),
        CONFIG_STRUCTURED_TRACE(
            UnregisteredForUpdate,
            14,
            Info,
            "{0} unregistered for updates on {1}. Subscriber: {2}",
            "id",
            "section",
            "subscriber")
        {
        }

    public:
        static ConfigEventSource const Events;
    };
}
