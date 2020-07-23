// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Autopilot
{
    using System;
    using System.Collections.Generic;
    using System.Linq;

    using Microsoft.Search.Autopilot;    

    internal static class ConfigurationManager
    {        
        public static int BackgroundStateMachineRunIntervalSeconds { get; private set; }

        public static int DeploymentRetryIntervalSeconds { get; private set; }

        public static BootstrapClusterManifestLocationType BootstrapClusterManifestLocation { get; private set; }

        public static int DynamicPortRangeStart { get; private set; }

        public static int DynamicPortRangeEnd { get; private set; }

        public static List<PortFowardingRule> PortFowardingRules { get; private set; }

        public static DynamicTopologyUpdateMode DynamicTopologyUpdateMode { get; private set; }

        public static LogLevel LogLevel { get; private set; }

        /*
         * TODO: currently AP safe config deployment is not supported to update configurations. 
         * The only way to update the configurations is via an application upgrade.
         * This is OK since none of the configurations are critical. 
         * To update all things critical in Service Fabric Autopilot Agent, an AP application upgrade is needed.
         */
        public static bool GetCurrentConfigurations(bool useTestMode = false)
        {
            if (useTestMode)
            {
                GetTestConfigurations();
            }
            else
            {
                IConfiguration configuration = APConfiguration.GetConfiguration();

                BackgroundStateMachineRunIntervalSeconds = configuration.GetInt32Value(StringConstants.AgentConfigurationSectionName, "BackgroundStateMachineRunIntervalSeconds", 10);

                DeploymentRetryIntervalSeconds = configuration.GetInt32Value(StringConstants.AgentConfigurationSectionName, "DeploymentRetryIntervalSeconds", 120);

                string bootstrapClusterManifestLocationString = configuration.GetStringValue(StringConstants.AgentConfigurationSectionName, "BootstrapClusterManifestLocation", string.Empty);

                BootstrapClusterManifestLocationType bootstrapClusterManifestLocation;
                if (!Enum.TryParse<BootstrapClusterManifestLocationType>(bootstrapClusterManifestLocationString, out bootstrapClusterManifestLocation))
                {
                    TextLogger.LogError("Bootstrap cluster manifest location is invalid or not specified : {0}.", bootstrapClusterManifestLocationString);

                    return false;
                }

                BootstrapClusterManifestLocation = bootstrapClusterManifestLocation;

                DynamicPortRangeStart = configuration.GetInt32Value(StringConstants.AgentConfigurationSectionName, "DynamicPortRangeStart", -1);

                DynamicPortRangeEnd = configuration.GetInt32Value(StringConstants.AgentConfigurationSectionName, "DynamicPortRangeEnd", -1);

                // please refer to http://support.microsoft.com/kb/929851 for valid dynamic port range in Windows.                
                if (DynamicPortRangeStart < 1025 || DynamicPortRangeEnd > 65535 || DynamicPortRangeEnd - DynamicPortRangeStart + 1 < 255)
                {
                    TextLogger.LogError("Dynamic port range is invalid or not specified : [{0}, {1}].", DynamicPortRangeStart, DynamicPortRangeEnd);

                    return false;
                }

                DynamicTopologyUpdateMode = (DynamicTopologyUpdateMode)configuration.GetInt32Value(StringConstants.AgentConfigurationSectionName, "DynamicTopologyUpdateMode", 0);

                string logLevelString = configuration.GetStringValue(StringConstants.AgentConfigurationSectionName, "LogLevel", "Info");

                LogLevel logLevel;
                if (!Enum.TryParse<LogLevel>(logLevelString, out logLevel))
                {
                    TextLogger.LogError("Log level is invalid : {0}.", logLevelString);

                    return false;
                }

                LogLevel = logLevel;

                PortFowardingRules = new List<PortFowardingRule>();
                if (configuration.SectionExists(StringConstants.PortForwardingConfigurationSectionName))
                {
                    string[] portFowardingRuleNames = configuration.GetSectionKeys(StringConstants.PortForwardingConfigurationSectionName);

                    if (portFowardingRuleNames != null)
                    {
                        foreach (string portFowardingRuleName in portFowardingRuleNames)
                        {
                            string[] portFowardingRuleComponents = configuration.GetStringValueAndSplit(StringConstants.PortForwardingConfigurationSectionName, portFowardingRuleName, ",");

                            if (portFowardingRuleComponents == null || portFowardingRuleComponents.Length != 3)
                            {
                                TextLogger.LogError("Port forwarding rule {0} is invalid. Valid rule format : RuleName=ListenPort,ConnectPort,MachineFunction", portFowardingRuleName);

                                return false;
                            }

                            int listenPort;                            
                            if (!int.TryParse(portFowardingRuleComponents[0], out listenPort) || listenPort <= 0 || listenPort > 65535)
                            {
                                TextLogger.LogError("Port forwarding rule {0} is invalid with invalid listen port {1}. Valid rule format : RuleName=ListenPort,ConnectPort,MachineFunction", portFowardingRuleName, portFowardingRuleComponents[0]);

                                return false;
                            }

                            int connectPort;
                            if (!int.TryParse(portFowardingRuleComponents[1], out connectPort) || connectPort <= 0 || connectPort > 65535)
                            {
                                TextLogger.LogError("Port forwarding rule {0} is invalid with invalid connect port {1}. Valid rule format : RuleName=ListenPort,ConnectPort,MachineFunction", portFowardingRuleName, portFowardingRuleComponents[1]);

                                return false;
                            }

                            PortFowardingRules.Add(new PortFowardingRule { RuleName = portFowardingRuleName, ListenPort = listenPort, ConnectPort = connectPort, MachineFunction = portFowardingRuleComponents[2] });
                        }
                    }                    
                }
            }

            return true;
        }

        public static void LogCurrentConfigurations()
        {
            TextLogger.LogInfo("Service Fabric Autopilot Agent configurations:");
            TextLogger.LogInfo("BackgroundStateMachineRunIntervalSeconds = {0}", BackgroundStateMachineRunIntervalSeconds);
            TextLogger.LogInfo("DeploymentRetryIntervalSeconds = {0}", DeploymentRetryIntervalSeconds);
            TextLogger.LogInfo("DynamicTopologyUpdateMode = {0}", DynamicTopologyUpdateMode);
            TextLogger.LogInfo("BootstrapClusterManifestLocation = {0}", BootstrapClusterManifestLocation);
            TextLogger.LogInfo("DynamicPortRangeStart = {0}", DynamicPortRangeStart);
            TextLogger.LogInfo("DynamicPortRangeEnd = {0}", DynamicPortRangeEnd);
            TextLogger.LogInfo("PortFowardingRulesCount = {0}", PortFowardingRules.Count);
        }

        // TODO: constants for defaults
        private static void GetTestConfigurations()
        {
            BackgroundStateMachineRunIntervalSeconds = 10;

            DeploymentRetryIntervalSeconds = 120;

            DynamicTopologyUpdateMode = DynamicTopologyUpdateMode.None;

            BootstrapClusterManifestLocation = BootstrapClusterManifestLocationType.Default;

            DynamicPortRangeStart = 33001;

            DynamicPortRangeEnd = 36000;

            PortFowardingRules = new List<PortFowardingRule>();

            LogLevel = LogLevel.Info;
        }
    }

    internal enum BootstrapClusterManifestLocationType
    {
        NotReady,

        Default,

        EnvironmentDefault
    }

    internal class PortFowardingRule
    {
        public string RuleName { get; set; }

        public int ListenPort { get; set; }

        public int ConnectPort { get; set; }

        public string MachineFunction { get; set; }
    }
}