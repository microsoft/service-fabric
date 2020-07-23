// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricMdsAgentSvc
{
    using System;
    using System.Fabric;
    using System.Fabric.Description;
    using System.Globalization;

    // This class implements the functionality to read configuration from Settings.xml
    internal static class ConfigReader
    {
        private const string TraceType = "ConfigReader";
        private const string ConfigPackageName = "MdsAgentService.Config";
        private static CodePackageActivationContext codePkgActivationCtx;
        private static MonitoringAgentServiceEvent serviceTrace;

        internal static void Initialize(
            StatelessServiceInitializationParameters initializationParameters, 
            MonitoringAgentServiceEvent trace)
        {
            codePkgActivationCtx = initializationParameters.CodePackageActivationContext;
            serviceTrace = trace;
        }

#pragma warning disable 618 // service needs to run on V2.1 cluster, so use obsolete API 
                            // CodePackageActivationContext.GetConfigurationPackage
        internal static T GetConfigValue<T>(string sectionName, string parameterName, T defaultValue)
        {
            T value = defaultValue;

            ConfigurationPackageDescription configPackageDesc = codePkgActivationCtx.GetConfigurationPackage(ConfigPackageName);
            if (null != configPackageDesc)
            {
                ConfigurationSettings configSettings = configPackageDesc.Settings;
                if (null != configSettings && 
                    null != configSettings.Sections &&
                    configSettings.Sections.Contains(sectionName))
                {
                    ConfigurationSection section = configSettings.Sections[sectionName];
                    if (null != section.Parameters &&
                        section.Parameters.Contains(parameterName))
                    {
                        ConfigurationProperty property = section.Parameters[parameterName];
                        try
                        {
                            value = (T)Convert.ChangeType(property.Value, typeof(T), CultureInfo.InvariantCulture);
                        }
                        catch (Exception e)
                        {
                            serviceTrace.Error(
                                "ConfigReader.GetConfigValue: Exception occurred while reading configuration from section {0}, parameter {1}. Exception: {2}",
                                sectionName,
                                parameterName,
                                e);
                        }
                    }
                }
            }

            return value;
        }
#pragma warning restore 618
    }
}