// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport
{
    using System;
    using System.Fabric;
    using System.Fabric.Common;
    using System.IO;
    using System.Reflection;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Description;

    internal sealed class FabricServiceConfig
    {
        private static FabricServiceConfig instance;
        private static readonly object instanceLock = new object();
        private static readonly string TraceType = "FabricServiceConfig";
        private static readonly string DefaultPackageName = "Config";
        public SettingsType Settings;
        public ConfigurationSettings configurationSettings;

        private FabricServiceConfig()
        {
            this.Settings = null;
        }

        public static bool InitializeFromConfigPackage(string configPackageName)
        {
            lock (instanceLock)
            {
                return InitializeFromConfigPkgWithCallerHoldingLock(configPackageName);
            }
        }

        /// <summary>
        /// It will initialize the FabricServiceConfig with settings loaded from fullFilePath .
        /// If File not found, it will return false.
        /// if initialized successfully , it returns true.
        /// </summary>
        /// <param name="fullFilePath"></param>
        /// <param name="configParser"></param>
        /// <returns></returns>
        public static bool Initialize(string fullFilePath, IFabricServiceConfigParser configParser = null)
        {
            lock (instanceLock)
            {
                return InitializeWithCallerHoldingLock(fullFilePath, configParser);
            }
        }

        private static bool InitializeWithCallerHoldingLock(string fullFilePath, IFabricServiceConfigParser configParser = null)
        {
            if (CheckifFileIsPresent(fullFilePath))
            {
                instance = new FabricServiceConfig();
                if (configParser == null)
                {
                    configParser = new SettingsConfigParser();
                }

                instance.Settings = configParser.Parse(fullFilePath);
                AppTrace.TraceSource.WriteInfo(TraceType, "Initialize Config From File:{0}", fullFilePath);
                return true;
            }
            else
            {
                AppTrace.TraceSource.WriteInfo(
                    TraceType,
                    "Couldn't load config as Settings File {0} is not Found",
                    fullFilePath);
                return false;
            }
        }

        public static FabricServiceConfig GetConfig()
        {
            lock (instanceLock)
            {
                if (instance == null)
                {
                    //For the Default Scenario , it will first check for Settings file 
                    var isInitialized = InitializeFromConfigPkgWithCallerHoldingLock(DefaultPackageName);

                    //If Settings file not present in configPackage Path, it will check for ExecutableName.Settings.xml in Executable Directory
                    if (!isInitialized)
                    {
                        InitializeWithCallerHoldingLock(GetExeSettingsFilePath());
                    }
                }
            }
            return instance;
        }

        private static bool TryGetConfigPackageObject(string configPackageName, out ConfigurationPackage package)
        {
            CodePackageActivationContext context;
            try
            {
                context = FabricRuntime.GetActivationContext();
            }
            catch (Exception ex)
            {
                AppTrace.TraceSource.WriteInfo(TraceType, "Exception while retrieving Activation Context {0}",
                    ex.GetType().FullName);
                package = null;
                return false;
            }
            var packages = context.GetConfigurationPackageNames();
            if (packages.Contains(configPackageName))
            {
                package = context.GetConfigurationPackageObject(configPackageName);
                return true;
            }

            AppTrace.TraceSource.WriteInfo(TraceType, "Couldn't Find ConfigPackage {0}", configPackageName);
            package = null;
            return false;
        }

        private static string GetExeSettingsFilePath()
        {
            var path = string.Empty;
            if (Assembly.GetEntryAssembly() != null)
            {
                path = Path.Combine(
                    Path.GetDirectoryName(Assembly.GetEntryAssembly().Location),
                    Assembly.GetEntryAssembly().GetName().Name + ".Settings.xml");
                return path;
            }
   
            AppTrace.TraceSource.WriteInfo(TraceType, "GetEntryAssembly was null ,couldn't find file path");
            return path;
        }

        private static bool CheckifFileIsPresent(string fileName)
        {
            return !String.IsNullOrEmpty(fileName) && File.Exists(fileName);
        }

        private static bool InitializeFromConfigPkgWithCallerHoldingLock(string configPackageName)
        {

                ConfigurationPackage configurationPackage = null;
                if (TryGetConfigPackageObject(configPackageName, out configurationPackage))
                {
                    instance = new FabricServiceConfig();
                    instance.configurationSettings = configurationPackage.Settings;
                    return true;
                }
                return false;
            

        }
    }
}