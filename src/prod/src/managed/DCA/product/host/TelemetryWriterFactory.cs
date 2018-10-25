// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.IO;
    using TelemetryAggregation;

    internal static class TelemetryWriterFactory
    {
        internal static ITelemetryWriter CreateTelemetryWriter(
            ConfigReader configReader,
            string clusterId,
            string nodeName,
            TelemetryCollection.LogErrorDelegate logErrorDelegate)
        {
#if !DotNetCoreClrLinux
            bool createFileShareWriter = false;
#endif
            FabricEvents.ExtensionsEvents traceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.FabricDCA);
            traceSource.WriteInfo("TelemetryWriterFactory", "Entering TelemetryWriterFactory");

#if !DotNetCoreClrLinux
            string fabricLogsShare;
            bool isFabricLogsLocalFolder;
            if (FileShareCommon.GetDestinationPath(
                            traceSource,
                            string.Concat(ConfigReader.FileShareWinFabSectionName),
                            configReader,
                            ConfigReader.FileShareWinFabSectionName,
                            out isFabricLogsLocalFolder,
                            out fabricLogsShare))
            {
                traceSource.WriteInfo("TelemetryWriterFactory", "isFabricLogsLocalFolder: {0}, fabricLogsShare: {1}", isFabricLogsLocalFolder, fabricLogsShare);

                string customizedGoalStateFileUrl = configReader.GetUnencryptedConfigValue(
                    ConfigReader.UpgradeOrchestrationServiceConfigSectionName,
                    ConfigReader.GoalStateFileUrlParamName,
                    string.Empty);

                if (customizedGoalStateFileUrl != string.Empty)
                {
                    createFileShareWriter = true;
                }
            }

            if (createFileShareWriter)
            {
                string fileShareDirectory = Path.Combine(Path.GetDirectoryName(fabricLogsShare), "fabrictelemetries-" + clusterId, nodeName);
                traceSource.WriteInfo("TelemetryWriterFactory", "return fileShare {0}", fileShareDirectory);
                return new FileShareTelemetryWriter(fileShareDirectory, logErrorDelegate);
            }
            else
#endif
            {
                traceSource.WriteInfo("TelemetryWriterFactory", "return appinsight");
                return new AppInsightsTelemetryWriter();
            }
        }
    }
}