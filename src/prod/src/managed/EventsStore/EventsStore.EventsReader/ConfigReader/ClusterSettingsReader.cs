// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsReader.ConfigReader
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Management.WindowsFabricValidator;
    using System.IO;
    using System.Linq;
    using EventsReader;

    /// <summary>
    /// Helper to extract cluster settings
    /// </summary>
    internal class ClusterSettingsReader
    {
        private static NativeConfigStore configStore;
        private const string QueryableConsumerIdentifier = "AzureTableQueryableEventUploader";
        private const string OperationalConsumerIdentifier = "AzureTableOperationalEventUploader";

        static ClusterSettingsReader()
        {
            configStore = NativeConfigStore.FabricGetConfigStore();
        }

        public static AzureTableConsumer OperationalConsumer
        {
            get
            {
                return GetEnabledAzureTableConsumersSettings().FirstOrDefault(consumer => consumer.Type == AzureConsumerType.Operational);
            }
        }

        public static AzureTableConsumer QueryableConsumer
        {
            get
            {
                return GetEnabledAzureTableConsumersSettings().FirstOrDefault(consumer => consumer.Type == AzureConsumerType.Query);
            }
        }

        public static bool IsOneBoxEnvironment()
        {
            var fabricDataRoot = FabricEnvironment.GetDataRoot();
            var clusterManifestLocation = Helpers.GetCurrentClusterManifestPath(fabricDataRoot);
            if (clusterManifestLocation == null)
            {
                clusterManifestLocation = Directory.EnumerateFiles(
                    fabricDataRoot,
                    "clusterManifest.xml",
                    SearchOption.AllDirectories).FirstOrDefault();
            }

            if (clusterManifestLocation == null)
            {
                throw new Exception("Unable to locate cluster manifest path");
            }

            var clusterManifest = XMLHelper.ReadClusterManifest(clusterManifestLocation);
            if (clusterManifest == null)
            {
                throw new Exception("Unable to read cluster manifest file");
            }
            
            if (clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsServer)
            {
                var windowsServerItem = ((ClusterManifestTypeInfrastructureWindowsServer)clusterManifest.Infrastructure.Item);
                return windowsServerItem.IsScaleMin || windowsServerItem.NodeList.Length <= 1; 
            }

            if (clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureLinux)
            {
                var linuxItem = ((ClusterManifestTypeInfrastructureLinux)clusterManifest.Infrastructure.Item);
                return linuxItem.IsScaleMin || linuxItem.NodeList.Length <= 1;
            }
            
            return false;
        }

        public static string[] GetDiagnosticsConsumersNames()
        {
            bool isEncrypted;
            var consumers = configStore.ReadString(
                ReaderConstants.ClusterSettings.Diagnostics,
                ReaderConstants.ClusterSettings.ConsumerInstances,
                out isEncrypted);
            if (string.IsNullOrEmpty(consumers))
            {
                return new string[] { };
            }

            return consumers.Split(
                new[] { ',' },
                StringSplitOptions.RemoveEmptyEntries)
                  .Select(instance => instance.Trim()).ToArray();
        }

        public static AzureTableConsumer[] GetEnabledAzureTableConsumersSettings()
        {
            var result = new List<AzureTableConsumer>();
            var consumers = GetDiagnosticsConsumersNames();
            foreach (var consumer in consumers)
            {
                bool isEncrypted;
                AzureConsumerType consumerType;
                var consumerTypeString = configStore.ReadString(consumer, ReaderConstants.ClusterSettings.ConsumerType, out isEncrypted);

                if (consumerTypeString != QueryableConsumerIdentifier && consumerTypeString != OperationalConsumerIdentifier)
                {
                    continue;
                }

                if(consumerTypeString == QueryableConsumerIdentifier)
                {
                    consumerType = AzureConsumerType.Query;
                }
                else
                {
                    consumerType = AzureConsumerType.Operational;
                }

                bool isConnectionStringEncrypted;
                var tablesPrefix = configStore.ReadString(
                    consumer,
                    ReaderConstants.ClusterSettings.TableNamePrefix,
                    out isEncrypted);

                var connectionString = configStore.ReadString(
                    consumer,
                    ReaderConstants.ClusterSettings.StoreConnectionString, 
                    out isConnectionStringEncrypted);

                var isEnabled = configStore.ReadString(
                    consumer,
                    ReaderConstants.ClusterSettings.IsEnabled,
                    out isEncrypted);

                if (string.IsNullOrEmpty(tablesPrefix) || string.IsNullOrEmpty(connectionString))
                {
                    continue;
                }

                if (!string.IsNullOrEmpty(isEnabled) && !bool.Parse(isEnabled))
                {
                    continue;
                }

                if (isConnectionStringEncrypted)
                {
                    var secureString = NativeConfigStore.DecryptText(connectionString);
                    var secureCharArray = FabricValidatorUtility.SecureStringToCharArray(secureString);
                    connectionString = new string(secureCharArray);
                }

                
                result.Add(new AzureTableConsumer(consumerType, tablesPrefix, connectionString));
            }

            return result.ToArray();
        }


    }
}