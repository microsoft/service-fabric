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

    internal static class EtlInMemoryProducerWorkerSettingsHelper
    {
        internal const string FabricLastEtlReadFileName = "fabric_DCALastEtlRead.dat";
        internal const string LeaseLastEtlReadFileName = "lease_DCALastEtlRead.dat";

        private const string LastReadTimestampFileSuffix = "DCALastEtlRead.dat";
        private const string StarDotEtl = "*.etl";
        private const string QueryLastEtlReadFileName = "query_DCALastEtlRead.dat";
        private const string OperationalLastEtlReadFileName = "operational_DCALastEtlRead.dat";

        private static readonly EtlInMemoryProducerWorker.ProviderInfo[] DefaultProviders =
        {
            new EtlInMemoryProducerWorker.ProviderInfo(
                FabricLastEtlReadFileName,
                string.Concat(WinFabricManifestManager.FabricEtlPrefix, "*.etl"),
                "Fabric"),
            new EtlInMemoryProducerWorker.ProviderInfo(
                LeaseLastEtlReadFileName,
                string.Concat(WinFabricManifestManager.LeaseLayerEtlPrefix, "*.etl"),
                "Lease Layer")
        };

        private static readonly EtlInMemoryProducerWorker.ProviderInfo[] QueryProviders =
        {
            new EtlInMemoryProducerWorker.ProviderInfo(
                QueryLastEtlReadFileName,
                string.Concat(WinFabricManifestManager.QueryEtlPrefix, "*.etl"),
                "Query")
        };

        private static readonly EtlInMemoryProducerWorker.ProviderInfo[] OperationalProviders =
        {
            new EtlInMemoryProducerWorker.ProviderInfo(
                OperationalLastEtlReadFileName,
                string.Concat(WinFabricManifestManager.OperationalEtlPrefix, "*.etl"),
                "Operational")
        };

        public static EtlInMemoryProducerWorker.EtlInMemoryProducerWorkerSettings InitializeSettings(EtlInMemoryProducerWorker.EtlInMemoryProducerWorkerParameters param)
        {
            // For most settings we'll just apply the latest setting that the caller has provided
            return new EtlInMemoryProducerWorker.EtlInMemoryProducerWorkerSettings
            {
                EtlReadInterval = param.LatestSettings.EtlReadInterval,
                EtlDeletionAge = param.LatestSettings.EtlDeletionAgeMinutes,
                WindowsFabricEtlType = param.LatestSettings.ServiceFabricEtlType,
                EtlPath = param.LatestSettings.EtlPath,
                EtlFilePatterns = param.LatestSettings.EtlFilePatterns,
                ProcessingWinFabEtlFiles = param.LatestSettings.ProcessingWinFabEtlFiles
            };
        }

        public static string InitializeTraceDirectory(
           string etlPath,
           string logDirectory,
           WinFabricEtlType fabricEtlType)
        {
            if (!string.IsNullOrEmpty(etlPath))
            {
                return etlPath;
            }

            return Path.Combine(logDirectory, GetTracesSubDirectory(fabricEtlType));
        }

        public static string InitializeMarkerFileDirectory(
            string etlPath,
            string logDirectory,
            string traceDirectory,
            WinFabricEtlType fabricEtlType)
        {
            if (!string.IsNullOrEmpty(etlPath))
            {
                return Path.Combine(
                    traceDirectory,
                    EtlInMemoryProducerConstants.MarkerFileDirectory);
            }

            return Path.Combine(logDirectory, GetMarkerFileDirectory(fabricEtlType));
        }

        public static List<EtlInMemoryProducerWorker.ProviderInfo> InitializeProviders(
            string etlPath, 
            string etlFilePatterns, 
            WinFabricEtlType fabricEtlType, 
            Action<string> onValidationError)
        {
            if (!string.IsNullOrEmpty(etlPath))
            {
                return InitializeProviderInfo(
                    etlFilePatterns,
                    onValidationError);
            }

            switch (fabricEtlType)
            {
                case WinFabricEtlType.QueryEtl:
                    return new List<EtlInMemoryProducerWorker.ProviderInfo>(QueryProviders);
                case WinFabricEtlType.OperationalEtl:
                    return new List<EtlInMemoryProducerWorker.ProviderInfo>(OperationalProviders);
                default:
                    return new List<EtlInMemoryProducerWorker.ProviderInfo>(DefaultProviders);
            }
        }

        private static List<EtlInMemoryProducerWorker.ProviderInfo> InitializeProviderInfo(string etlFilePatterns, Action<string> onValidationError)
        {
            var providers = new List<EtlInMemoryProducerWorker.ProviderInfo>();

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
                var provider = new EtlInMemoryProducerWorker.ProviderInfo(
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
                    return EtlInMemoryProducerConstants.QueryTracesDirectory;
                case WinFabricEtlType.OperationalEtl:
                    return EtlInMemoryProducerConstants.OperationalTracesDirectory;
                default:
                    return EtlInMemoryProducerConstants.TracesDirectory;
            }
        }

        private static string GetMarkerFileDirectory(WinFabricEtlType fabricEtlType)
        {
            switch (fabricEtlType)
            {
                case WinFabricEtlType.QueryEtl:
                    return EtlInMemoryProducerConstants.QueryMarkerFileDirectory;
                case WinFabricEtlType.OperationalEtl:
                    return EtlInMemoryProducerConstants.OperationalMarkerFileDirectory;
                default:
                    return EtlInMemoryProducerConstants.MarkerFileDirectory;
            }
        }
    }
}