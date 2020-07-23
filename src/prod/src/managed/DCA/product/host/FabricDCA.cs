// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.Fabric.Health;
    using System.Fabric.Strings;
    using System.Linq;

    // This class implements the overall flow of control for:
    // - DCA initialization
    // - Producer and consumer management
    // - DCA teardown
    internal class FabricDCA : IDisposable
    {
        // Constants
        private const string TraceType = "FabricDCA";

        // Application instance ID of the application for whom we are collecting data
        private readonly Dictionary<string, IDcaProducer> producers;
        private readonly Dictionary<string, IDcaConsumer> consumers;
        private readonly DCASettings settings;

        private readonly HashSet<string> registeredAppConfigSections;
        private readonly HashSet<string> registeredServiceConfigSections;

        // Whether or not the object has been disposed
        private bool disposed;

        internal FabricDCA(string applicationInstanceId, DiskSpaceManager diskSpaceManager)
        {
            this.registeredAppConfigSections = new HashSet<string>();
            this.registeredServiceConfigSections = new HashSet<string>();

            // Retrieve DCA settings
            this.settings = new DCASettings(applicationInstanceId);

            // Get the names of sections in settings.xml that contain DCA-related 
            // configuration information
            this.registeredAppConfigSections.UnionWith(GetConfigurationSections(this.settings));

            // Dictionary that represents the mapping of producers and consumers
            // Key is the producer instance and value is the list of consumer instances
            // that are interested in receiving data from that producer instance.
            var producerConsumerMap = new Dictionary<string, List<object>>();

            var errorEvents = new List<string>();

            this.consumers = CreateConsumers(
                new ConsumerFactory(),
                producerConsumerMap,
                this.settings,
                diskSpaceManager,
                applicationInstanceId,
                errorEvents);

            // Create the Telemetry consumer and maps it to a etlfileproducer if it exists
            if (applicationInstanceId == Utility.WindowsFabricApplicationInstanceId)
            {
                CreateTelemetryConsumer(
                    this.consumers,
                    producerConsumerMap,
                    this.settings,
                    applicationInstanceId);
            }

                // Create the producers
                this.producers = CreateProducers(
                new ProducerFactory(diskSpaceManager),
                producerConsumerMap, 
                this.settings, 
                applicationInstanceId,
                errorEvents);

            // Send all errors found in initialization of plugins as single report.
            if (Utility.IsSystemApplicationInstanceId(applicationInstanceId))
            {
                if (errorEvents.Any())
                {
                    var message = string.Join(Environment.NewLine, errorEvents);
                    HealthClient.SendNodeHealthReport(message, HealthState.Error);
                }
                else
                {
                    HealthClient.ClearNodeHealthReport();
                }
            }

            // Get additional configuration sections that the producers are 
            // interested in
            this.registeredAppConfigSections.UnionWith(GetAdditionalProducerAppSections(this.producers));
            this.RegisteredServiceConfigSections.UnionWith(GetAdditionalProducerServiceSections(this.producers));
        }
        
        // Set of configuration sections that we are interested in
        internal IEnumerable<string> RegisteredAppConfigSections
        {
            get
            {
                return this.registeredAppConfigSections;
            }
        }

        internal HashSet<string> RegisteredServiceConfigSections
        {
            get
            {
                return this.registeredServiceConfigSections;
            }
        }

        internal DCASettings Settings
        {
            get
            {
                return this.settings;
            }
        }

        public void FlushData()
        {
            // Flush producers before consumers.
            foreach (string producerInstance in this.producers.Keys)
            {
                this.producers[producerInstance].FlushData();
            }

            foreach (string consumerInstance in this.consumers.Keys)
            {
                this.consumers[consumerInstance].FlushData();
            }
        }

        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }

            this.disposed = true;

            // Dispose producers before disposing the consumers. This will ensure
            // that while consumers are being disposed, they are not receiving data
            // from the producers
            foreach (string producerInstance in this.producers.Keys)
            {
                this.producers[producerInstance].Dispose();
                FabricEvents.Events.PluginDisposed(producerInstance);
            }

            foreach (string consumerInstance in this.consumers.Keys)
            {
                this.consumers[consumerInstance].Dispose();
                FabricEvents.Events.PluginDisposed(consumerInstance);
            }

            GC.SuppressFinalize(this);
        }
        
        private static IEnumerable<string> GetAdditionalProducerAppSections(IDictionary<string, IDcaProducer> producers)
        {
            var registeredAppConfigSections = new HashSet<string>();

            foreach (string producerInstance in producers.Keys)
            {
                IEnumerable<string> appConfigSections = producers[producerInstance].AdditionalAppConfigSections;
                if (null != appConfigSections)
                {
                    foreach (string appConfigSection in appConfigSections)
                    {
                        registeredAppConfigSections.Add(appConfigSection);
                    }
                }
            }

            return registeredAppConfigSections;
        }

        private static IEnumerable<string> GetAdditionalProducerServiceSections(IDictionary<string, IDcaProducer> producers)
        {
            var registeredServiceConfigSections = new HashSet<string>();

            foreach (string producerInstance in producers.Keys)
            {
                IEnumerable<string> serviceConfigSections = producers[producerInstance].ServiceConfigSections;
                if (null != serviceConfigSections)
                {
                    foreach (string serviceConfigSection in serviceConfigSections)
                    {
                        registeredServiceConfigSections.Add(serviceConfigSection);
                    }
                }
            }

            return registeredServiceConfigSections;
        }

        private static Dictionary<string, IDcaProducer> CreateProducers(
            ProducerFactory producerFactory,
            IDictionary<string, List<object>> producerConsumerMap,
            DCASettings settings,
            string applicationInstanceId,
            IList<string> errorEvents)
        {
            // Initialize producer instance list
            var producers = new Dictionary<string, IDcaProducer>();
            Debug.Assert(null != producerConsumerMap, "Map of producers to consumers must be initialized.");

            foreach (string producerInstance in settings.ProducerInstances.Keys)
            {
                // Get the producer instance information
                DCASettings.ProducerInstanceInfo producerInstanceInfo = settings.ProducerInstances[producerInstance];
                
                // Prepare the producer initialization parameters
                ProducerInitializationParameters initParam = new ProducerInitializationParameters();
                initParam.ApplicationInstanceId = applicationInstanceId;
                initParam.SectionName = producerInstanceInfo.SectionName;
                initParam.LogDirectory = Utility.LogDirectory;
                initParam.WorkDirectory = Utility.DcaWorkFolder;

                if (ContainerEnvironment.IsContainerApplication(applicationInstanceId))
                {
#if DotNetCoreClrLinux
                    string traceReaderProducerTypeName = StandardPluginTypes.LttProducer;
#else
                    string traceReaderProducerTypeName = StandardPluginTypes.EtlFileProducer;
#endif
                    if (producerInstanceInfo.TypeName != traceReaderProducerTypeName)
                    {
                        continue;
                    }

                    string containerLogFolder = ContainerEnvironment.GetContainerLogFolder(applicationInstanceId);
                    initParam.LogDirectory = containerLogFolder;
                    initParam.WorkDirectory = Utility.GetOrCreateContainerWorkDirectory(containerLogFolder);
                }

                if (producerConsumerMap.ContainsKey(producerInstance))
                {
                    initParam.ConsumerSinks = producerConsumerMap[producerInstance];
                    producerConsumerMap.Remove(producerInstance);
                }
                else
                {
                    initParam.ConsumerSinks = null;
                }
                
                // Create producer instance
                try
                {
                    var producerInterface = producerFactory.CreateProducer(
                                            producerInstance,
                                            initParam,
                                            producerInstanceInfo.TypeName);

                    // Add the producer to the producer list
                    producers[producerInstance] = producerInterface;
                }
                catch (Exception e)
                {
                    // Initialize the producer
                    Utility.TraceSource.WriteError(
                        TraceType,
                        "Failed to create producer {0}. {1}",
                        producerInstance,
                        e);
                    var message = string.Format(
                        StringResources.DCAError_UnhandledPluginExceptionHealthDescription, 
                        producerInstanceInfo.SectionName,
                        e.Message);
                    errorEvents.Add(message);
                }
            }

            return producers;
        }

        private static Dictionary<string, IDcaConsumer> CreateConsumers(
            ConsumerFactory consumerFactory,
            IDictionary<string, List<object>> producerConsumerMap,
            DCASettings settings,
            DiskSpaceManager diskSpaceManager,
            string applicationInstanceId,
            IList<string> errorEvents)
        {
            // Initialize consumer instance list
            var consumers = new Dictionary<string, IDcaConsumer>();

            foreach (string consumerInstance in settings.ConsumerInstances.Keys)
            {
                // Get the consumer instance information
                DCASettings.ConsumerInstanceInfo consumerInstanceInfo = settings.ConsumerInstances[consumerInstance];
                
                // Prepare the consumer initialization parameters
                var initParam = new ConsumerInitializationParameters(
                    applicationInstanceId,
                    consumerInstanceInfo.SectionName,
                    Utility.FabricNodeId,
                    Utility.FabricNodeName,
                    Utility.LogDirectory,
                    Utility.DcaWorkFolder,
                    diskSpaceManager);

                // if the application is a container move to container log folder.
                if (ContainerEnvironment.IsContainerApplication(applicationInstanceId))
                {
                    string containerLogFolder = ContainerEnvironment.GetContainerLogFolder(applicationInstanceId);

                    initParam = new ConsumerInitializationParameters(
                        applicationInstanceId,
                        consumerInstanceInfo.SectionName,
                        Utility.FabricNodeId,
                        Utility.FabricNodeName,
                        containerLogFolder,
                        Utility.GetOrCreateContainerWorkDirectory(containerLogFolder),
                        diskSpaceManager);
                }

                // Create consumer instance
                IDcaConsumer consumerInterface;
                try
                {
                    consumerInterface = consumerFactory.CreateConsumer(
                        consumerInstance,
                        initParam,
                        consumerInstanceInfo.TypeInfo.AssemblyName,
                        consumerInstanceInfo.TypeInfo.TypeName);
                }
                catch (Exception e)
                {
                    // We should continue trying to create other consumers.
                    errorEvents.Add(e.Message);
                    continue;
                }

                // Get the consumer's data sink
                object sink = consumerInterface.GetDataSink();
                if (null == sink)
                {
                    // The consumer does not wish to provide a data sink. 
                    // One situation this might happen is if the consumer has been
                    // disabled. This is not an error, so just move on to the next
                    // consumer.
                    continue;
                }

                // Add the data sink to the corresponding producer's consumer sink list
                string producerInstance = consumerInstanceInfo.ProducerInstance;
                Debug.Assert(false == string.IsNullOrEmpty(producerInstance), "Consumers must be tied to a producer");
                if (false == producerConsumerMap.ContainsKey(producerInstance))
                {
                    producerConsumerMap[producerInstance] = new List<object>();
                }

                producerConsumerMap[producerInstance].Add(sink);

                // Add the consumer to the consumer list
                consumers[consumerInstance] = consumerInterface;
            }

            return consumers;
        }

        private static void CreateTelemetryConsumer(
            IDictionary<string, IDcaConsumer> consumers,
            IDictionary<string, List<object>> producerConsumerMap,
            DCASettings settings,
            string applicationInstanceId)
        {
            Debug.Assert(null != consumers, "Consumers must be initialized.");

            ConfigReader producerCfg = new ConfigReader(applicationInstanceId);

#if !DotNetCoreClrLinux
            Func<KeyValuePair<string, DCASettings.ProducerInstanceInfo>, bool> pluginComparator = x => x.Value.TypeName.Equals(StandardPluginTypes.EtlFileProducer) || x.Value.TypeName.Equals(StandardPluginTypes.EtlInMemoryProducer);
            string producerTypeDefault = EtlProducerValidator.DefaultEtl;
            string producerWindowsFabricTypeParamName = EtlProducerValidator.WindowsFabricEtlTypeParamName;
            string producerServiceFabricTypeParamName = EtlProducerValidator.ServiceFabricEtlTypeParamName;
#else
            Func<KeyValuePair<string, DCASettings.ProducerInstanceInfo>, bool> pluginComparator = x => x.Value.TypeName.Equals(StandardPluginTypes.LttProducer);
            string producerTypeDefault = "";
            string producerWindowsFabricTypeParamName = "";
            string producerServiceFabricTypeParamName = LttProducerConstants.ServiceFabricLttTypeParamName;
#endif
            // first look for a EtlFileProducer or EtlInMemoryProducer type, then, if found, create a TelemetryConsumer and map it to the Etl*Producer
            var traceProducerKeys = new List<string>();
            var traceProducers = settings.ProducerInstances.Where(pluginComparator).ToArray();

            foreach (var producer in traceProducers)
            {
                string traceProducerType = producerCfg.GetUnencryptedConfigValue(producer.Key, producerWindowsFabricTypeParamName, string.Empty);
                if (string.IsNullOrWhiteSpace(traceProducerType) ||
                    traceProducerType.Equals(producerTypeDefault))
                {
                    // Check if plugin is using new type field name
                    traceProducerType = producerCfg.GetUnencryptedConfigValue(producer.Key, producerServiceFabricTypeParamName, string.Empty);
                    if (string.IsNullOrWhiteSpace(traceProducerType) ||
                        traceProducerType.Equals(producerTypeDefault))
                    {
                        traceProducerKeys.Add(producer.Key);
                    }
                }
            }

            if (traceProducerKeys.Any())
            {
#if !DotNetCoreClrLinux
                var telemetryConsumer = new TelemetryConsumerWindows(applicationInstanceId);
#else
                var telemetryConsumer = new TelemetryConsumerLinux(applicationInstanceId);
#endif
                consumers["TelemetryConsumer"] = telemetryConsumer;
                foreach (var etlProducerKey in traceProducerKeys)
                {
                    if (false == producerConsumerMap.ContainsKey(etlProducerKey))
                    {
                        producerConsumerMap[etlProducerKey] = new List<object>();
                    }

                    // if telemetry is disabled then sink is null so don't add it to the mapping.
                    var sink = telemetryConsumer.GetDataSink();
                    if (null != sink)
                    {
                        producerConsumerMap[etlProducerKey].Add(sink);
                    }
                }
            }
        }

        private static IEnumerable<string> GetConfigurationSections(DCASettings settings)
        {
            // Add the "Diagnostics" section
            var registeredAppConfigSections = new List<string> { ConfigReader.DiagnosticsSectionName };

            // Add the producer-specific sections
            foreach (string producerInstance in settings.ProducerInstances.Keys)
            {
                registeredAppConfigSections.Add(producerInstance);
            }

            // Add the consumer-specific sections
            foreach (string consumerInstance in settings.ConsumerInstances.Keys)
            {
                registeredAppConfigSections.Add(consumerInstance);

                string typeName = settings.ConsumerInstances[consumerInstance].TypeName;
                if ((false == string.IsNullOrEmpty(typeName)) &&
                    (false == StandardPlugins.Consumers.ContainsKey(typeName)))
                {
                    registeredAppConfigSections.Add(typeName);
                }
            }

            return registeredAppConfigSections;
        }
    }
}
