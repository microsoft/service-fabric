// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Autopilot
{
    internal class StringConstants
    {
        public const string ApplicationConfigurationFileName = "Config.ini";

        public const string MachineInfoFileName = "MachineInfo.csv";

        public const string DiskLayoutConfigurationFileName = "DiskLayout.csv";

        public const string AgentConfigurationSectionName = "ServiceFabricAutopilotAgent";

        public const string PortForwardingConfigurationSectionName = "ServiceFabricAutopilotAgent.PortForwarding";

        public const string EventSourceName = "ServiceFabricAutopilotAgent";

        public const string CustomLogIdName = "ServiceFabricAutopilotAgent";

        public const string EnvironmentDefaultConfigDirectoryName = "EnvironmentConfig";

        public const string DefaultBootstrapClusterManifestFileName = "ClusterManifest.xml";

        public const string ServiceFabricRootDirectoryName = "SFRoot";

        public const string TestModeLogFileNameTemplate = "ServiceFabricAutopilotAgent_{0}.log";

        public const string TestModeLogDirectoryName = "ServiceFabricAutopilotAgentLog";

        public const string TestModeTraceListenerName = "ServiceFabricAutopilotAgentTraceListener";

        public const string PropertyName = "ServiceFabricAutopilotAgent";
    }
}