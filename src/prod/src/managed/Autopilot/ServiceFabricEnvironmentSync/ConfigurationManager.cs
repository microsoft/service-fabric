// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Autopilot
{
    using System;

    using Microsoft.Search.Autopilot;

    internal static class ConfigurationManager
    {
        // The interval between certificate manager checks local environment and ensures all environment certificates are present.
        public static int CertificateManagerRunIntervalSeconds { get; private set; }

        // When certificate manager fails to check local environment and ensure all environment certificates are present, the interval between retries.
        // Certificate manager would keep retrying until a successful run while reporting health reflecting the status.        
        public static int CertificateManagerRetryIntervalSeconds { get; private set; }

        // Whether to enable Secret Store client side persistent caching to increase decryption performance and relability in case of Secret Store being unavailable.
        public static bool EnableSecretStorePersistentCaching { get; private set; }

        // The minimal log level for ServiceFabricEnvironmentSync logging.
        public static LogLevel LogLevel { get; private set; }

        /*
         * TODO: currently AP safe config deployment is not supported to update configurations. 
         * The only way to update the configurations is via an application upgrade.
         */
        public static bool GetCurrentConfigurations()
        {
            IConfiguration configuration = APConfiguration.GetConfiguration();

            CertificateManagerRunIntervalSeconds = configuration.GetInt32Value(StringConstants.CertificateManagerConfigurationSectionName, "CertificateManagerRunIntervalSeconds", 3600);

            CertificateManagerRetryIntervalSeconds = configuration.GetInt32Value(StringConstants.CertificateManagerConfigurationSectionName, "CertificateManagerRetryIntervalSeconds", 300);

            EnableSecretStorePersistentCaching = configuration.GetBoolValue(StringConstants.CertificateManagerConfigurationSectionName, "EnableSecretStoreClientSidePersistentCaching", true);

            string logLevelString = configuration.GetStringValue(StringConstants.EnvironmentSyncConfigurationSectionName, "LogLevel", "Info");

            LogLevel logLevel;
            if (!Enum.TryParse<LogLevel>(logLevelString, out logLevel))
            {
                TextLogger.LogError("Log level is invalid : {0}.", logLevelString);

                return false;
            }

            LogLevel = logLevel;

            return true;
        } 
    }
}