// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Insights
{
    using System;
    using System.Globalization;
    using System.Linq;
    using System.Threading;
    using Cluster.ClusterQuery;
    using ClusterAnalysis.AnalysisCore.ClusterEntityDescription;
    using ClusterAnalysis.Common.Log;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Hosting;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.RA;
    using Assert = ClusterAnalysis.Common.Util.Assert;

    internal static class ClusterEntityExtractor
    {
        // Supply from Config in future.
        private static readonly string[] SystemProcessesWithParentCodePackage =
        {
            "FabricGateway.exe",
            "FabricApplicationGateway.exe",
            "FabricDCA.exe",
            "FabricDeployer.exe",
            "FabricInstallerService.exe",
            "FabricSetup.exe",
            "Fabric.exe",
            "FabricUS.exe",
            "FabricHost.exe",
            "FabricRM.exe"
        };

        public static bool TryExtractClusterEntity(
            ILogger logger,
            IClusterQuery clusterQuery,
            TraceRecord traceRecord,
            CancellationToken cancelToken,
            out BaseEntity extractedEntity)
        {
            Assert.IsNotNull(clusterQuery);
            Assert.IsNotNull(traceRecord);

            extractedEntity = null;
            try
            {
                extractedEntity = ExtractClusterEntity(logger, clusterQuery, traceRecord, cancelToken);
            }
            catch (Exception exp)
            {
                logger.LogWarning("TryExtractClusterEntity:: Couldn't Extract Entity for Event: '{0}'. Exp: '{1}'", traceRecord, exp);
                return false;
            }

            return true;
        }

        // TODO: Make Async
        public static BaseEntity ExtractClusterEntity(ILogger logger, IClusterQuery clusterQuery, TraceRecord traceRecord, CancellationToken cancelToken)
        {
            Assert.IsNotNull(clusterQuery);
            Assert.IsNotNull(traceRecord);

            if (traceRecord is ReconfigurationCompletedTraceRecord)
            {
                return (BaseEntity)ExtractPartitionEntityFromReconfigEvent(clusterQuery, (ReconfigurationCompletedTraceRecord)traceRecord, cancelToken);
            }

            throw new NotSupportedException(string.Format(CultureInfo.InvariantCulture, "Type '{0}' Event not supported", traceRecord.GetType()));
        }

        private static object ExtractPartitionEntityFromReconfigEvent(
            IClusterQuery clusterQuery,
            ReconfigurationCompletedTraceRecord reconfigEvent,
            CancellationToken cancelToken)
        {
            return clusterQuery.GetPartitionAsync(reconfigEvent.PartitionId, cancelToken).GetAwaiter().GetResult();
        }

        /// <summary>
        /// Best Effort Today. TODO: Need to improve this but require the incoming event to have more information
        /// attached to it (like Service Manifest Name etc.). This would increase the efficiency as well.
        /// </summary>
        /// <remarks>
        /// TODO: Look at moving all out the special casing complexity into Cluster query object itself.
        /// </remarks>
        /// <param name="logger"></param>
        /// <param name="clusterQuery"></param>
        /// <param name="processTerminationEvent"></param>
        /// <param name="cancelToken"></param>
        /// <returns></returns>
        private static object ExtractDeployedCodePackageEntityFromProcessCrashEvent(
            ILogger logger,
            IClusterQuery clusterQuery,
            ProcessUnexpectedTerminationTraceRecord processTerminationEvent,
            CancellationToken cancelToken)
        {
            var nodeObject = clusterQuery.GetNodeListAsync(CancellationToken.None).GetAwaiter().GetResult().Single(node => node.NodeId == processTerminationEvent.NodeId);

            var allCps = clusterQuery.GetDeployedCodePackageAsync(nodeObject.NodeName, cancelToken).GetAwaiter().GetResult();
            logger.LogMessage(
                "ExtractDeployedCodePackageEntityFromProcessCrashEvent:: Found '{0}' code packages on Node '{1}'",
                allCps.Count(),
                nodeObject.NodeName);

            foreach (var codePackage in allCps)
            {
                if ((codePackage.SetupPointLocation != null && codePackage.SetupPointLocation.Contains(processTerminationEvent.ProcessName)) ||
                    codePackage.EntryPointLocation.Contains(processTerminationEvent.ProcessName))
                {
                    return codePackage;
                }
            }

            // There are bunch of system processes which SF tracks and traces but they don't have a Code package for them (e.g. FabricDca.exe)
            // For such processes, we wrap them up in a Dummy System Code Package.
            if (SystemProcessesWithParentCodePackage.Contains(processTerminationEvent.ProcessName, StringComparer.InvariantCultureIgnoreCase))
            {
                var defaultSystemProcessApp = new ApplicationEntity()
                {
                    ApplicationName = ApplicationEntity.DefaultAppForSystemProcesses,
                    ApplicationTypeName = ApplicationEntity.DefaultAppTypeNameForSystemProcesses,
                    ApplicationTypeVersion = nodeObject.CodeVersion
                };

                return new DeployedCodePackageEntity
                {
                    NodeName = nodeObject.NodeName,
                    Application = defaultSystemProcessApp,
                    CodePackageName = DeployedCodePackageEntity.SystemCodePackageName,
                    CodePackageVersion = nodeObject.CodeVersion,
                    ServiceManifestName = DeployedCodePackageEntity.SystemServiceManifestName,
                    EntryPointLocation = processTerminationEvent.ProcessName
                };
            }

            var placeholderApplicationEntity = new ApplicationEntity()
            {
                ApplicationName = ApplicationEntity.PlaceholderApplicationName,
                ApplicationTypeName = ApplicationEntity.PlaceholderApplicationTypeName,
                ApplicationTypeVersion = nodeObject.CodeVersion
            };

            // Today this calculation is best effort. We continue to work toward encriching trace information so that we can get to cluster entity with more confidence
            // With the information we have today, there are few scenario where we may run into below return. There is a time gap between process crashing and we kicking
            // off analysis. In that meantime, the replica may have been moved of the node (for example by PLB or let's say Chaos). In such cases, we don't want to stop
            // analyzing the process crash and hence we place this process into a Place holder Code Package Object. 
            return new DeployedCodePackageEntity
            {
                NodeName = nodeObject.NodeName,
                Application = placeholderApplicationEntity,
                CodePackageName = DeployedCodePackageEntity.PlaceholderCodePackageName,
                CodePackageVersion = nodeObject.CodeVersion,
                ServiceManifestName = DeployedCodePackageEntity.PlaceholderServiceManifestName,
                EntryPointLocation = processTerminationEvent.ProcessName
            };
        }
    }
}