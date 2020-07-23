// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using Linq;
    using Newtonsoft.Json;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.WRP.Model;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;

    internal sealed class PaasCoordinator : IUpgradeCoordinator
    {
        private const string ApiVersion = "6.5";

        private static readonly TraceType TraceType = new TraceType("PaasCoordinator");
        private IList<IUpgradeCoordinator> coordinators;

        public PaasCoordinator(
            IConfigStore configStore,
            string configSectionName,
            KeyValueStoreReplica kvsStore,
            IStatefulServicePartition partition)
        {
            configStore.ThrowIfNull("configStore");
            configSectionName.ThrowIfNullOrWhiteSpace("configSectionName");
            partition.ThrowIfNull("partition");

            this.Initialize(configStore, configSectionName, kvsStore, partition);
        }

        public string ListeningAddress
        {
            get { return string.Empty; }
        }

        public IEnumerable<Task> StartProcessing(CancellationToken token)
        {
            var tasks = new List<Task>();
            Trace.WriteInfo(TraceType, "StartProcessing called");

            foreach (var coordinator in this.coordinators)
            {
                tasks.AddRange(coordinator.StartProcessing(token));
            }

            return tasks;
        }

        private void Initialize(
            IConfigStore configStore,
            string configSectionName,
            KeyValueStoreReplica kvsStore,
            IStatefulServicePartition partition)
        {
            var baseUrlStr = configStore.ReadUnencryptedString(configSectionName, Constants.ConfigurationSection.BaseUrl);
            var clusterId = configStore.ReadUnencryptedString(Constants.ConfigurationSection.PaasSectionName, Constants.ConfigurationSection.ClusterId);            

            // Error checking
            baseUrlStr.ThrowIfNullOrWhiteSpace("baseUrlStr");
            clusterId.ThrowIfNullOrWhiteSpace("clusterId");
            Uri baseUrl;
            if (!Uri.TryCreate(baseUrlStr, UriKind.Absolute, out baseUrl))
            {
                throw new ArgumentException("BaseUrl in clusterManifest is not of Uri type");
            }

            Trace.WriteNoise(TraceType, "BaseUrl: {0}", baseUrl);

            var wrpStreamChannelUrlStr = Path.Combine(baseUrlStr, clusterId);            
            Uri wrpStreamChannelUri;
            if (!Uri.TryCreate(wrpStreamChannelUrlStr, UriKind.Absolute, out wrpStreamChannelUri))
            {
                throw new ArgumentException($"StreamChannel URL is not of Uri type: {wrpStreamChannelUrlStr}");
            }

            Trace.WriteInfo(TraceType, "StreamChannel URL: {0}", wrpStreamChannelUrlStr);

            var wrpPollUrlStr = $"{wrpStreamChannelUrlStr}?api-version={ApiVersion}";
            Uri wrpPollUri;
            if (!Uri.TryCreate(wrpPollUrlStr, UriKind.Absolute, out wrpPollUri))
            {
                throw new ArgumentException($"Poll URL is not of Uri type: {wrpPollUrlStr}");
            }
            Trace.WriteInfo(TraceType, "Poll URL: {0}", wrpPollUrlStr);

            var fabricClientWrapper = new FabricClientWrapper();

            var resourceCommandProcessors = new Dictionary<ResourceType, IResourceCommandProcessor>()
            {
                {
                    ResourceType.Cluster,
                    new ClusterCommandProcessor(
                        configStore,
                        configSectionName,
                        fabricClientWrapper,
                        new CommandParameterGenerator(fabricClientWrapper),
                        new NodeStatusManager(kvsStore, configStore, configSectionName, new ExceptionHandlingPolicy(Constants.PaasCoordinator.ClusterCommandProcessorHealthProperty, ResourceCoordinator.TaskName, configStore, configSectionName, partition)),
                        JsonSerializer.Create(WrpGatewayClient.SerializerSettings))
                },
                {
                    ResourceType.ApplicationType,
                    new ApplicationTypeCommandProcessor(
                        new ApplicationTypeClient(fabricClientWrapper.FabricClient, configStore, configSectionName))
                },
                {
                    ResourceType.Application,
                    new ApplicationCommandProcessor(
                        new ApplicationClient(fabricClientWrapper.FabricClient))
                },
                {
                    ResourceType.Service,
                    new ServiceCommandProcessor(
                        new ServiceClient(fabricClientWrapper.FabricClient))
                }
            };

            this.coordinators = new List<IUpgradeCoordinator>
            {
                new ResourceCoordinator(
                    kvsStore,
                    configStore,
                    configSectionName, 
                    resourceCommandProcessors, 
                    new WrpPackageRetriever(wrpPollUri, clusterId, configStore, configSectionName),
                    new ExceptionHandlingPolicy(Constants.PaasCoordinator.SFRPPollHealthProperty, ResourceCoordinator.TaskName, configStore, configSectionName, partition)),

                new StreamChannelCoordinator(
                    new WrpStreamChannel(wrpStreamChannelUri, clusterId, configStore, configSectionName), 
                    new ExceptionHandlingPolicy(Constants.PaasCoordinator.SFRPStreamChannelHealthProperty, ResourceCoordinator.TaskName, configStore, configSectionName, partition))
            };            
        }        
    }
}