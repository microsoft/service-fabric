// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Dca;
    using System.Fabric.Dca.EtlConsumerHelper;
    using System.IO;
    using System.Linq;

    using Tools.EtlReader;

    internal static class EtlProducerWorkerSettingsHelper
    {
        // Constants
        internal const string FabricLastEtlReadFileName = "fabric_DCALastEtlRead.dat";
        internal const string LeaseLastEtlReadFileName = "lease_DCALastEtlRead.dat";

        private const string LastReadTimestampFileSuffix = "DCALastEtlRead.dat";
        private const string ApplicationTracesEtlFilePattern = "fabric_application_traces_*.etl";
        private const string StarDotEtl = "*.etl";
        private const string QueryLastEtlReadFileName = "query_DCALastEtlRead.dat";
        private const string OperationalLastEtlReadFileName = "operational_DCALastEtlRead.dat";

        private static readonly EtlProducerWorker.ProviderInfo[] DefaultProviders =
        {
            new EtlProducerWorker.ProviderInfo(
                FabricLastEtlReadFileName,
                string.Concat(WinFabricManifestManager.FabricEtlPrefix, "*.etl"),
                "Fabric"),
            new EtlProducerWorker.ProviderInfo(
                LeaseLastEtlReadFileName,
                string.Concat(WinFabricManifestManager.LeaseLayerEtlPrefix, "*.etl"),
                "Lease Layer")
        };

        private static readonly EtlProducerWorker.ProviderInfo[] QueryProviders =
        {
            new EtlProducerWorker.ProviderInfo(
                QueryLastEtlReadFileName,
                string.Concat(WinFabricManifestManager.QueryEtlPrefix, "*.etl"),
                "Query")
        };

        private static readonly EtlProducerWorker.ProviderInfo[] OperationalProviders =
        {
            new EtlProducerWorker.ProviderInfo(
                OperationalLastEtlReadFileName,
                string.Concat(WinFabricManifestManager.OperationalEtlPrefix, "*.etl"),
                "Operational")
        };

        public static EtlProducerWorker.EtlProducerWorkerSettings InitializeSettings(EtlProducerWorker.EtlProducerWorkerParameters param)
        {
            // For most settings we'll just apply the latest setting that 
            // the caller has provided
            var etlProducerSettings = new EtlProducerWorker.EtlProducerWorkerSettings
            {
                EtlReadInterval = param.LatestSettings.EtlReadInterval,
                EtlDeletionAge = param.LatestSettings.EtlDeletionAgeMinutes,
                WindowsFabricEtlType = param.LatestSettings.ServiceFabricEtlType,
                EtlPath = param.LatestSettings.EtlPath,
                EtlFilePatterns = param.LatestSettings.EtlFilePatterns,
                ProcessingWinFabEtlFiles = param.LatestSettings.ProcessingWinFabEtlFiles
            };

            if (param.IsReadingFromApplicationManifest)
            {
                // We need to merge the remaining settings from each of the ETL producers 
                // that depend on us
                var appEtwGuids = new List<Guid>();
                foreach (var producer in param.EtlProducers)
                {
                    appEtwGuids = appEtwGuids.Union(producer.Settings.AppEtwGuids).ToList();
                }

                etlProducerSettings.CustomManifestPaths = GetServiceEtwManifestPaths(param.EtlProducers);
                etlProducerSettings.AppEtwGuids = appEtwGuids;
            }
            else
            {
                // If we're getting settings from cluster manifest, then 
                // all our settings are copied from the latest settings
                // provided by caller.
                etlProducerSettings.CustomManifestPaths = param.LatestSettings.CustomManifestPaths;
                etlProducerSettings.AppEtwGuids = param.LatestSettings.AppEtwGuids;
            }

            return etlProducerSettings;
        }

        public static List<IEtlFileSink> InitializeSinks(IEnumerable<EtlProducer> etlProducers, Action<string> onInvalidCast)
        {
            var consumerSinks = new List<object>();

            // Get the union of the consumer sink lists from each of the ETL producers
            // that depend on us
            foreach (var producer in etlProducers)
            {
                if (null != producer.ConsumerSinks)
                {
                    consumerSinks = consumerSinks.Union(producer.ConsumerSinks).ToList();
                }
            }

            var sinks = new List<IEtlFileSink>();

            // Go through the consumer sinks, cast each of them into an IEtlFileSink
            // and add them to the sink list
            foreach (var sinkAsObject in consumerSinks)
            {
                IEtlFileSink etlFileSink;
                try
                {
                    etlFileSink = (IEtlFileSink)sinkAsObject;
                }
                catch (InvalidCastException e)
                {
                    onInvalidCast(
                        string.Format(
                            "Exception occured while casting an object of type {0} to interface IEtlFileSink. Exception information: {1}.",
                            sinkAsObject.GetType(),
                            e));
                    continue;
                }

                sinks.Add(etlFileSink);
            }

            return sinks;
        }

        public static List<EtlToCsvFileWriter> InitializeFileWriters(IEnumerable<IEtlFileSink> sinks, EtlProducerWorker worker)
        {
            var etlToCsvFileWriters = new List<EtlToCsvFileWriter>();

            // Figure out which sinks are implemented by the EtlToCsvFileWriter class.
            // We make some special calls into those sinks that we do not make
            // into other sinks.
            foreach (var sink in sinks)
            {
                if (sink is EtlToCsvFileWriter)
                {
                    etlToCsvFileWriters.Add((EtlToCsvFileWriter)sink);
                }
            }

            // Provide the EtlToCsvFileWriter sinks with our IEtlProducer interface
            foreach (var etlToCsvFileWriter in etlToCsvFileWriters)
            {
                etlToCsvFileWriter.SetEtlProducer(worker);
            }

            return etlToCsvFileWriters;
        }

        public static List<BufferedEtwEventProvider> InitializeBufferedEtwEventProviders(IEnumerable<IEtlFileSink> sinks, EtlProducerWorker worker)
        {
            var bufferedEtwEventProviders = new List<BufferedEtwEventProvider>();

            // Figure out which sinks are implemented by the BufferedEtwEventProvider class.
            // We make some special calls into those sinks that we do not make
            // into other sinks.
            foreach (var sink in sinks)
            {
                if (sink is BufferedEtwEventProvider)
                {
                    bufferedEtwEventProviders.Add((BufferedEtwEventProvider)sink);
                }
            }

            // Provide the BufferedEtwEventProvider sinks with our IEtlProducer interface
            foreach (var bufferedEtwEventProvider in bufferedEtwEventProviders)
            {
                bufferedEtwEventProvider.SetEtlProducer(worker);
            }

            return bufferedEtwEventProviders;
        }

        public static string InitializeTraceDirectory(
            bool isReadingFromApplicationManifest, 
            string etlPath, 
            string logDirectory,
            WinFabricEtlType fabricEtlType)
        {
            if (isReadingFromApplicationManifest || !string.IsNullOrEmpty(etlPath))
            {
                return etlPath;
            }

            return Path.Combine(logDirectory, GetTracesSubDirectory(fabricEtlType));
        }

        public static string InitializeMarkerFileDirectory(
            bool isReadingFromApplicationManifest, 
            string etlPath, 
            string logDirectory, 
            WinFabricEtlType fabricEtlType, 
            string traceDirectory, 
            string producerInstanceId)
        {
            if (isReadingFromApplicationManifest)
            {
                return Path.Combine(
                    traceDirectory,
                    producerInstanceId,
                    EtlProducerConstants.MarkerFileDirectory);
            }

            if (!string.IsNullOrEmpty(etlPath))
            {
                return Path.Combine(
                    traceDirectory,
                    EtlProducerConstants.MarkerFileDirectory);
            }

            return Path.Combine(logDirectory, GetMarkerFileDirectory(fabricEtlType));
        }

        public static List<EtlProducerWorker.ProviderInfo> InitializeProviders(bool isReadingFromApplicationManifest, string etlPath, WinFabricEtlType fabricEtlType, string etlFilePatterns, Action<string> onValidationError)
        {
            if (isReadingFromApplicationManifest)
            {
                return InitializeProviderInfo(
                    etlFilePatterns,
                    onValidationError);
            }

            if (!string.IsNullOrEmpty(etlPath))
            {
                return InitializeProviderInfo(
                    etlFilePatterns,
                    onValidationError);
            }

            switch (fabricEtlType)
            {
                case WinFabricEtlType.QueryEtl:
                    return new List<EtlProducerWorker.ProviderInfo>(QueryProviders);
                case WinFabricEtlType.OperationalEtl:
                    return new List<EtlProducerWorker.ProviderInfo>(OperationalProviders);
                default:
                    return new List<EtlProducerWorker.ProviderInfo>(DefaultProviders);
            }
        }

        private static List<string> GetServiceEtwManifestPaths(List<EtlProducer> etlProducers)
        {
            // Build up a dictionary of ETW manifests with the highest data package versions. We ignore
            // any ETW manifests with lower data package versions.
            var manifestMaxVersions =
                new Dictionary<string, Dictionary<string, EtwManifestVersionedPath>>();
            foreach (var etlProducer in etlProducers)
            {
                // Go through each service package associated with this producer
                foreach (var servicePackageName in etlProducer.Settings.ServiceEtwManifests.Keys)
                {
                    if (manifestMaxVersions.ContainsKey(servicePackageName))
                    {
                        // Another producer is already associated with this service package
                        var manifestMaxVersionsForServicePkg =
                            manifestMaxVersions[servicePackageName];
                        var manifests =
                            etlProducer.Settings.ServiceEtwManifests[servicePackageName];

                        // Go through each ETW manifest data package associated with this service package
                        foreach (var manifestInfo in manifests)
                        {
                            if (manifestMaxVersionsForServicePkg.ContainsKey(manifestInfo.DataPackageName))
                            {
                                // Another producer is already associated with this data package
                                var versionedPath =
                                    manifestMaxVersionsForServicePkg[manifestInfo.DataPackageName];

                                // Compare the version of the data package associated with the current producer with the highest
                                // known version so far
                                if (versionedPath.Version.CompareTo(manifestInfo.DataPackageVersion) < 0)
                                {
                                    // The current producer has the highest version of the manifest data package so far.
                                    // So use the manifest path provided by the current producer.
                                    versionedPath.Path = manifestInfo.Path;
                                    versionedPath.Version = manifestInfo.DataPackageVersion;
                                }
                            }
                            else
                            {
                                // No other producer is associated with this data package. So the version
                                // associated with the current producer is the highest version by default.
                                var versionedPath = new EtwManifestVersionedPath
                                {
                                    Version = manifestInfo.DataPackageVersion,
                                    Path = manifestInfo.Path
                                };
                                manifestMaxVersionsForServicePkg[manifestInfo.DataPackageName] = versionedPath;
                            }
                        }
                    }
                    else
                    {
                        // No other producer is associated with this service package. So for each manifest
                        // data package, the version associated with the current producer is the highest 
                        // version by default.
                        var manifestMaxVersionsForServicePkg =
                            new Dictionary<string, EtwManifestVersionedPath>();
                        var manifests =
                            etlProducer.Settings.ServiceEtwManifests[servicePackageName];
                        foreach (var manifestInfo in manifests)
                        {
                            var versionedPath = new EtwManifestVersionedPath
                            {
                                Version = manifestInfo.DataPackageVersion,
                                Path = manifestInfo.Path
                            };
                            manifestMaxVersionsForServicePkg[manifestInfo.DataPackageName] = versionedPath;
                        }

                        manifestMaxVersions[servicePackageName] = manifestMaxVersionsForServicePkg;
                    }
                }
            }

            // Now iterate over the dictionary and build a list of ETW manifest paths
            var serviceManifestEtwPaths = new List<string>();
            foreach (
                var manifestMaxVersionsForServicePkg in
                    manifestMaxVersions.Values)
            {
                foreach (var versionedPath in manifestMaxVersionsForServicePkg.Values)
                {
                    serviceManifestEtwPaths.Add(versionedPath.Path);
                }
            }

            return serviceManifestEtwPaths;
        }

        private static List<EtlProducerWorker.ProviderInfo> InitializeProviderInfo(string etlFilePatterns, Action<string> onValidationError)
        {
            var providers = new List<EtlProducerWorker.ProviderInfo>();

            var patternArray = etlFilePatterns.Split(',');
            foreach (var patternAsIs in patternArray)
            {
                var pattern = patternAsIs.Trim();

                // Make sure the pattern contains no path information
                var tempPattern = Path.GetFileName(pattern);
                if (false == tempPattern.Equals(
                    pattern,
                    StringComparison.OrdinalIgnoreCase))
                {
                    onValidationError(string.Format(
                        "The ETL file pattern '{0}' is invalid and will be ignored. Patterns should not contain any path information.",
                        pattern));
                    continue;
                }

                // Make sure the pattern ends with *.etl"
                if (false == pattern.EndsWith(
                    StarDotEtl,
                    StringComparison.OrdinalIgnoreCase))
                {
                    onValidationError(string.Format(
                        "The ETL file pattern '{0}' is invalid and will be ignored. Patterns should always end with {1}.",
                        pattern,
                        StarDotEtl));
                    continue;
                }

                // Get the pattern prefix
                var index = pattern.LastIndexOf(StarDotEtl, StringComparison.OrdinalIgnoreCase);
                var patternPrefix = pattern.Remove(index);

                // Compute the name of the file that stores the last read timestamp
                var lastReadTimestampFile = string.Concat(
                    patternPrefix,
                    LastReadTimestampFileSuffix);

                // Create a provider object and add it to the list of providers
                var provider = new EtlProducerWorker.ProviderInfo(
                    lastReadTimestampFile,
                    pattern,
                    patternPrefix);
                providers.Add(provider);
            }

            return providers;
        }

        private static string GetTracesSubDirectory(WinFabricEtlType fabricEtlType)
        {
            switch (fabricEtlType)
            {
                case WinFabricEtlType.QueryEtl:
                    return EtlProducerConstants.QueryTracesDirectory;
                case WinFabricEtlType.OperationalEtl:
                    return EtlProducerConstants.OperationalTracesDirectory;
                default:
                    return EtlProducerConstants.TracesDirectory;
            }
        }

        private static string GetMarkerFileDirectory(WinFabricEtlType fabricEtlType)
        {
            switch (fabricEtlType)
            {
                case WinFabricEtlType.QueryEtl:
                    return EtlProducerConstants.QueryMarkerFileDirectory;
                case WinFabricEtlType.OperationalEtl:
                    return EtlProducerConstants.OperationalMarkerFileDirectory;
                default:
                    return EtlProducerConstants.MarkerFileDirectory;
            }
        }

        private class EtwManifestVersionedPath
        {
            internal string Path { get; set; }

            internal Version Version { get; set; }
        }
    }
}