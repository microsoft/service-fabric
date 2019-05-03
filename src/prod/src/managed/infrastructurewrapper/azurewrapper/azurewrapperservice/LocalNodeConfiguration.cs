// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.Fabric.InfrastructureWrapper
{
    using System;
    using System.Diagnostics;            
    using System.Globalization;
    using System.IO;    

    using Microsoft.WindowsAzure.ServiceRuntime;

    internal class LocalNodeConfiguration
    {
        private LocalNodeConfiguration()
        {
        }        

        internal NonWindowsFabricEndpointSet NonWindowsFabricEndpointSet { get; private set; }

        internal int ApplicationPortCountConfigurationSettingValue { get; private set; }

        internal int EphemeralPortCountConfigurationSettingValue { get; private set; }

        internal int ApplicationStartPort { get; private set; }

        internal int ApplicationEndPort { get; private set; }        

        internal int EphemeralStartPort { get; private set; }

        internal int EphemeralEndPort { get; private set; }

        internal string BootstrapClusterManifestLocation { get; private set; }

        internal string RootDirectory { get; private set; }

        internal string DataRootDirectory { get; private set; }

        internal string DeploymentRootDirectory { get; private set; }

        internal int DeploymentRetryIntervalSeconds { get; private set; }

        internal TraceLevel TextLogTraceLevel { get; private set; }

        internal bool MultipleCloudServiceDeployment { get; private set; }

        internal bool SubscribeBlastNotification { get; private set; }

        internal static LocalNodeConfiguration GetLocalNodeConfiguration()
        {
            string rootDirectory = GetRootDirectory();

            int applicationPortCount = GetApplicationPortCount();

            int applicationStartPort = GetApplicationStartPort();

            int ephemeralPortCountConfigurationSettingValue =
                GetConfigurationSettingValueAsInt(
                    WindowsFabricAzureWrapperServiceCommon.EphemeralPortCountConfigurationSetting,
                    applicationPortCount / 2);

            int ephemeralPortCount = ephemeralPortCountConfigurationSettingValue;

            // If a invalid value for configuration setting WindowsFabric.EphemeralPortCount is specified, use the default value
            if (ephemeralPortCount <= 0 || ephemeralPortCount >= applicationPortCount)
            {
                ephemeralPortCount = applicationPortCount / 2;
            }
                        
            int ephemeralEndPort = applicationStartPort + applicationPortCount - 1;
           
            int ephemeralStartPort = ephemeralEndPort - ephemeralPortCount + 1;
            
            int applicationEndPort = ephemeralStartPort - 1;

            return new LocalNodeConfiguration()
            {                                
                BootstrapClusterManifestLocation = GetBootstrapClusterManifestLocation(),                

                ApplicationPortCountConfigurationSettingValue = applicationPortCount,

                EphemeralPortCountConfigurationSettingValue = ephemeralPortCountConfigurationSettingValue,

                ApplicationStartPort = applicationStartPort,   
    
                ApplicationEndPort = applicationEndPort,                        

                EphemeralStartPort =  ephemeralStartPort,

                EphemeralEndPort = ephemeralEndPort,

                NonWindowsFabricEndpointSet = GetNonWindowsFabricEndpointSet(),

                RootDirectory = rootDirectory,

                DataRootDirectory = GetDataRootDirectory(rootDirectory),

                DeploymentRootDirectory = GetDeploymentRootDirectory(rootDirectory),

                DeploymentRetryIntervalSeconds =
                    GetConfigurationSettingValueAsInt(
                        WindowsFabricAzureWrapperServiceCommon.DeploymentRetryIntervalSecondsConfigurationSetting,
                        WindowsFabricAzureWrapperServiceCommon.DefaultDeploymentRetryIntervalSeconds),

                TextLogTraceLevel =
                    GetConfigurationSettingValueAsTraceLevel(
                        WindowsFabricAzureWrapperServiceCommon.TextLogTraceLevelConfigurationSetting,
                        WindowsFabricAzureWrapperServiceCommon.DefaultTextLogTraceLevel),

                MultipleCloudServiceDeployment =
                    GetConfigurationSettingValueAsBool(
                        WindowsFabricAzureWrapperServiceCommon.MultipleCloudServiceDeploymentConfigurationSetting,
                        WindowsFabricAzureWrapperServiceCommon.DefaultMultipleCloudServiceDeployment),

                SubscribeBlastNotification =
                    GetConfigurationSettingValueAsBool(
                        WindowsFabricAzureWrapperServiceCommon.SubscribeBlastNotificationConfigurationSetting,
                        WindowsFabricAzureWrapperServiceCommon.DefaultSubscribeBlastNotification),
            };
        }
      
        private static string GetBootstrapClusterManifestLocation()
        {
            return GetConfigurationSettingValue(WindowsFabricAzureWrapperServiceCommon.BootstrapClusterManifestLocationConfigurationSetting);
        }

        private static int GetApplicationPortCount()
        {
            return int.Parse(GetConfigurationSettingValue(WindowsFabricAzureWrapperServiceCommon.ApplicationPortCountConfigurationSetting), CultureInfo.InvariantCulture);
        }

        private static int GetApplicationStartPort()
        {
            return RoleEnvironment.CurrentRoleInstance.InstanceEndpoints[WindowsFabricAzureWrapperServiceCommon.ApplicationPortsEndpointName].IPEndpoint.Port;
        }

        private static string GetRootDirectory()
        {
            string localResourcePath = RoleEnvironment.GetLocalResource(WindowsFabricAzureWrapperServiceCommon.WindowsFabricRootDirectoryLocalResourceName).RootPath;
            return Path.Combine(Path.GetPathRoot(localResourcePath), WindowsFabricAzureWrapperServiceCommon.WindowsFabricRootDirectoryName);
        }

        private static string GetDataRootDirectory(string rootDirectory)
        {
            return rootDirectory;
        }

        private static string GetDeploymentRootDirectory(string rootDirectory)
        {
            return Path.Combine(rootDirectory, WindowsFabricAzureWrapperServiceCommon.WindowsFabricDeploymentRootDirectoryName);
        }

        private static int GetConfigurationSettingValueAsInt(string configurationSettingName, int defaultConfigurationSettingValue)
        {
            int configurationSettingValue;

            try
            {
                string configurationSettingValueStr = GetConfigurationSettingValue(configurationSettingName);

                configurationSettingValue = int.Parse(configurationSettingValueStr, CultureInfo.InvariantCulture);
            }
            catch (Exception e)
            {
                if (WindowsFabricAzureWrapperServiceCommon.IsFatalException(e))
                {
                    throw;
                }

                configurationSettingValue = defaultConfigurationSettingValue;
            }

            return configurationSettingValue;
        }

        private static TraceLevel GetConfigurationSettingValueAsTraceLevel(string configurationSettingName, string defaultConfigurationSettingValue)
        {
            TraceLevel configurationSettingValue;

            try
            {
                string configurationSettingValueStr = GetConfigurationSettingValue(configurationSettingName);

                configurationSettingValue = (TraceLevel)Enum.Parse(typeof(TraceLevel), configurationSettingValueStr, true);
            }
            catch (Exception e)
            {
                if (WindowsFabricAzureWrapperServiceCommon.IsFatalException(e))
                {
                    throw;
                }

                configurationSettingValue = (TraceLevel)Enum.Parse(typeof(TraceLevel), defaultConfigurationSettingValue, true);
            }

            return configurationSettingValue;
        }

        private static bool GetConfigurationSettingValueAsBool(string configurationSettingName, bool defaultConfigurationSettingValue)
        {
            bool configurationSettingValue;

            try
            {
                string configurationSettingValueStr = GetConfigurationSettingValue(configurationSettingName);

                configurationSettingValue = bool.Parse(configurationSettingValueStr);
            }
            catch (Exception e)
            {
                if (WindowsFabricAzureWrapperServiceCommon.IsFatalException(e))
                {
                    throw;
                }

                configurationSettingValue = defaultConfigurationSettingValue;
            }

            return configurationSettingValue;
        }


        private static string GetConfigurationSettingValue(string configurationSettingName)
        {
            return RoleEnvironment.GetConfigurationSettingValue(configurationSettingName);
        }

        private static NonWindowsFabricEndpointSet GetNonWindowsFabricEndpointSet()
        {
            return NonWindowsFabricEndpointSet.GetNonWindowsFabricEndpointSet(RoleEnvironment.CurrentRoleInstance.InstanceEndpoints);
        }
    }
}

