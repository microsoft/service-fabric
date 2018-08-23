// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.Fabric.Health;
    using System.IO;
    using System.Linq;

    // This class implements the logic for managing DCA settings
    internal class DCASettings
    {
        // Constants
        private const string TraceType = "Settings";

        // Parameter names in DCA-specific sections in settings.xml
        private const string AssemblyParamName = "Assembly";
        private const string TypeParamName = "Type";
        private const string HealthSubProperty = "Configuration";

        // Configuration reader
        private readonly IDcaSettingsConfigReader configReader;
        private readonly ReadOnlyDictionary<string, ProducerInstanceInfo> producerInstances;
        private readonly ReadOnlyDictionary<string, ConsumerInstanceInfo> consumerInstances;
        private readonly string applicationInstanceId;

        private readonly List<string> errors = new List<string>();

        internal DCASettings(string applicationInstanceId)
            : this(applicationInstanceId, new DcaSettingsConfigReader(applicationInstanceId))
        {
        }

        internal DCASettings(string applicationInstanceId, IDcaSettingsConfigReader dcaSettingsConfigReader)
        {
            this.applicationInstanceId = applicationInstanceId;
            this.configReader = dcaSettingsConfigReader;

            Action<string> onError = error =>
                {
                    // Trace immediately and aggregate for health.
                    Utility.TraceSource.WriteError(TraceType, error);
                    this.errors.Add(error);
                };

            this.producerInstances = new ReadOnlyDictionary<string, ProducerInstanceInfo>(this.GetProducers(onError));
            this.consumerInstances = new ReadOnlyDictionary<string, ConsumerInstanceInfo>(this.GetConsumers(onError));

            var pluginsEnabledBitFlags = this.producerInstances
                .Where(pair => StandardPluginTypes.PluginTypeMap.ContainsKey(pair.Value.TypeName))
                .Aggregate(StandardPluginTypes.PluginType.None, (agg, pair) => agg | StandardPluginTypes.PluginTypeMap[pair.Value.TypeName]);
            pluginsEnabledBitFlags = this.consumerInstances
                .Where(pair => StandardPluginTypes.PluginTypeMap.ContainsKey(pair.Value.TypeName))
                .Aggregate(pluginsEnabledBitFlags, (agg, pair) => agg | StandardPluginTypes.PluginTypeMap[pair.Value.TypeName]);

            if (Utility.IsSystemApplicationInstanceId(this.applicationInstanceId))
            {
                FabricEvents.Events.PluginConfigurationTelemetry((long)pluginsEnabledBitFlags);

                if (this.errors.Any())
                {
                    HealthClient.SendNodeHealthReport(string.Join("\n", this.errors), HealthState.Error, HealthSubProperty);
                }
                else
                {
                    HealthClient.ClearNodeHealthReport(HealthSubProperty);
                }
            }
            else
            {
                // ApplicationInstanceId will contain user app names, so we should hash value.
                var derivedApplicationInstanceId = string.Format("App{0}", this.applicationInstanceId.GetHashCode());
                FabricEvents.Events.AppPluginConfigurationTelemetry(derivedApplicationInstanceId, (long)pluginsEnabledBitFlags);
            }
        }

        internal interface IDcaSettingsConfigReader
        {
            string GetProducerType(string sectionName);

            string GetProducerInstances();

            string GetAssemblyName(string consumerType);

            string GetTypeName(string consumerType);

            string GetConsumerType(string sectionName);

            string GetConsumerInstances();

            string GetProducerInstance(string sectionName);
        }

        internal IEnumerable<string> Errors
        {
            get
            {
                return this.errors.AsReadOnly();
            }
        }

        // Dictionary of producer instances. The key is the name of the section
        // containing information about the producer instance.
        internal IReadOnlyDictionary<string, ProducerInstanceInfo> ProducerInstances
        {
            get
            {
                return this.producerInstances;
            }
        }

        // Dictionary of consumer instances. The key is the name of the section
        // containing information about the consumer instance.
        internal IReadOnlyDictionary<string, ConsumerInstanceInfo> ConsumerInstances 
        {
            get
            {
                return this.consumerInstances;
            }
        }

        private ProducerInstanceInfo GetProducerInstanceInfo(string sectionName)
        {
            // Get the producer type
            string producerType = this.configReader.GetProducerType(sectionName);
            if (string.IsNullOrEmpty(producerType))
            {
                throw new InvalidOperationException(string.Format("Producer type has not been specified in section '{0}'.", sectionName));
            }

            producerType = producerType.Trim();

            // Ensure that the producer type is supported
            if (false == StandardPlugins.Producers.Contains(producerType))
            {
                    throw new InvalidOperationException(
                        string.Format(
                        "Producer type '{0}' in section '{1}' is not supported.",
                        producerType,
                        sectionName));
            }

            // Return information to caller
            var instanceInfo = new ProducerInstanceInfo
            {
                TypeName = producerType,
                SectionName = sectionName
            };

            Utility.TraceSource.WriteInfo(
                TraceType,
                "Producer instance: {0}, producer type: {1}.",
                sectionName,
                producerType);
            return instanceInfo;
        }
        
        private Dictionary<string, ProducerInstanceInfo> GetProducers(Action<string> onError)
        {
            // Initialize the producer info list
            var instances = new Dictionary<string, ProducerInstanceInfo>();

            // Get the producer information as a string from settings.xml
            string producerList = this.configReader.GetProducerInstances();
            if (string.IsNullOrEmpty(producerList))
            {
                Utility.TraceSource.WriteInfo(
                    TraceType,
                    "No producer instances specified in section '{0}'.",
                    ConfigReader.DiagnosticsSectionName);
                return instances;
            }
            
            // Get the producer instance sections
            string[] producers = producerList.Split(',');
            
            // Go through each producer instance section to get more information
            foreach (string producerAsIs in producers)
            {
                string producer = producerAsIs.Trim();
                if (instances.ContainsKey(producer))
                {
                   var message = string.Format(
                        "Producer instance '{0}' has been specified more than once in section '{1}'. Only the first appearance is used, subsequent appearances are ignored.",
                        producer,
                        ConfigReader.DiagnosticsSectionName);
                    onError(message);
                    continue;
                }
                
                try
                {
                    instances[producer] = this.GetProducerInstanceInfo(producer);
                }
                catch (InvalidOperationException ex)
                {
                    onError(ex.Message);
                }
            }

            return instances;
        }

        private PluginInfo GetConsumerPluginInfo(string consumerType)
        {
            string assemblyName;
            string typeName;

            if (StandardPlugins.Consumers.ContainsKey(consumerType))
            {
                // This is a standard consumer plugin
                assemblyName = StandardPlugins.Consumers[consumerType].AssemblyName;
                typeName = StandardPlugins.Consumers[consumerType].TypeName;
            }
            else
            {
                // This is a custom consumer plugin

                // Get the assembly name
                assemblyName = this.configReader.GetAssemblyName(consumerType);
                if (string.IsNullOrEmpty(assemblyName))
                {
                    throw new InvalidOperationException(
                        string.Format(
                        "Assembly has not been specified in section '{0}'.",
                        consumerType));
                }

                assemblyName = assemblyName.Trim();

                // Make sure that only the name of plug-in assembly is specified, 
                // without any path information
                if (false == assemblyName.Equals(Path.GetFileName(assemblyName)))
                {
                    throw new InvalidOperationException(
                        string.Format(
                        "Assembly name '{0}' in section '{1}' is not in the correct format. The assembly name should not include the path.",
                        assemblyName,
                        consumerType));
                }

                // Get the type name
                typeName = this.configReader.GetTypeName(consumerType);
                if (string.IsNullOrEmpty(typeName))
                {
                    throw new InvalidOperationException(
                        string.Format(
                        "Type has not been specified in section '{0}'.",
                        consumerType));
                }

                typeName = typeName.Trim();
            }

            // Return information to caller
            return new PluginInfo
            {
                AssemblyName = assemblyName,
                TypeName = typeName
            };
        }
        
        private ConsumerInstanceInfo GetConsumerInstanceInfo(string sectionName)
        {
            // Get the consumer type name
            string consumerType = this.configReader.GetConsumerType(sectionName);
            if (string.IsNullOrEmpty(consumerType))
            {
                throw new InvalidOperationException(
                    string.Format(
                    "Consumer type has not been specified in section '{0}'.",
                    sectionName));
            }

            consumerType = consumerType.Trim();

            // Get information about the plug in that implements the consumer type
            var pluginInfo = this.GetConsumerPluginInfo(consumerType);

            // Get the producer instance that the consumer is associated with
            string producerInstance = this.configReader.GetProducerInstance(sectionName);
            if (string.IsNullOrEmpty(producerInstance))
            {
                throw new InvalidOperationException(
                    string.Format(
                    "Producer instance has not been specified in section '{0}'.",
                    sectionName));
            }

            producerInstance = producerInstance.Trim();

            if (false == this.producerInstances.ContainsKey(producerInstance))
            {
                throw new InvalidOperationException(
                    string.Format(
                    "Producer instance '{0}' referenced in section '{1}' could not be found. This could be because the producer instance was not specified or was incorrectly specified.",
                    producerInstance,
                    sectionName));
            }

            // Return information to caller
            var instanceInfo = new ConsumerInstanceInfo
            {
                TypeName = consumerType,
                TypeInfo = pluginInfo,
                ProducerInstance = producerInstance,
                SectionName = sectionName
            };

            Utility.TraceSource.WriteInfo(
                TraceType,
                "Consumer instance: {0}, consumer type: {1}, consumer assembly: {2}, consumer type name: {3}, producer instance: {4}.",
                sectionName,
                consumerType,
                pluginInfo.AssemblyName,
                pluginInfo.TypeName,
                producerInstance);
            return instanceInfo;
        }

        private Dictionary<string, ConsumerInstanceInfo> GetConsumers(Action<string> onError)
        {
            // Initialize the consumer info list
            var instances = new Dictionary<string, ConsumerInstanceInfo>();

            // Get the consumer information as a string from settings.xml
            string consumerList = this.configReader.GetConsumerInstances();
            if (string.IsNullOrEmpty(consumerList))
            {
                Utility.TraceSource.WriteInfo(
                    TraceType,
                    "No consumer instances specified in section '{0}'.",
                    ConfigReader.DiagnosticsSectionName);
                return instances;
            }
        
            // Get the consumer instance sections
            string[] consumers = consumerList.Split(',');

            // Go through each consumer instance section to get more information
            foreach (string consumerAsIs in consumers)
            {
                string consumer = consumerAsIs.Trim();
                if (instances.ContainsKey(consumer))
                {
                    var message = string.Format(
                        "Consumer instance '{0}' has been specified more than once in section '{1}'. Only the first appearance is used, subsequent appearances are ignored.",
                        consumer,
                        ConfigReader.DiagnosticsSectionName);
                    onError(message);
                    continue;
                }

                try
                {
                    instances[consumer] = this.GetConsumerInstanceInfo(consumer);
                }
                catch (InvalidOperationException ioe)
                {
                    onError(ioe.Message);
                }
            }

            return instances;
        }

        // Information about a producer instance
        internal class ProducerInstanceInfo
        {
            internal string TypeName { get; set; }

            internal string SectionName { get; set; }
        }

        // Information about plugins
        internal class PluginInfo
        {
            internal string AssemblyName { get; set; }

            internal string TypeName { get; set; }
        }

        // Information about a consumer instance
        internal class ConsumerInstanceInfo
        {
            internal string TypeName { get; set; }

            internal PluginInfo TypeInfo { get; set; }

            internal string ProducerInstance { get; set; }

            internal string SectionName { get; set; }
        }

        private class DcaSettingsConfigReader : IDcaSettingsConfigReader
        {
            private readonly ConfigReader configReader;

            public DcaSettingsConfigReader(string applicationInstanceId)
            {
                this.configReader = new ConfigReader(applicationInstanceId);
            }

            public string GetProducerType(string sectionName)
            {
                return this.configReader.GetUnencryptedConfigValue(
                                      sectionName,
                                      ConfigReader.ProducerTypeParamName,
                                      string.Empty);
            }

            public string GetProducerInstances()
            {
                return this.configReader.GetUnencryptedConfigValue(
                                      ConfigReader.DiagnosticsSectionName,
                                      ConfigReader.ProducerInstancesParamName,
                                      string.Empty);
            }

            public string GetAssemblyName(string consumerType)
            {
                return this.configReader.GetUnencryptedConfigValue(
                                   consumerType,
                                   AssemblyParamName,
                                   string.Empty);
            }

            public string GetTypeName(string consumerType)
            {
                return this.configReader.GetUnencryptedConfigValue(
                               consumerType,
                               TypeParamName,
                               string.Empty);
            }

            public string GetConsumerType(string sectionName)
            {
                return this.configReader.GetUnencryptedConfigValue(
                                          sectionName,
                                          ConfigReader.ConsumerTypeParamName,
                                          string.Empty);
            }

            public string GetConsumerInstances()
            {
                return this.configReader.GetUnencryptedConfigValue(
                                          ConfigReader.DiagnosticsSectionName,
                                          ConfigReader.ConsumerInstancesParamName,
                                          string.Empty);
            }

            public string GetProducerInstance(string sectionName)
            {
                return this.configReader.GetUnencryptedConfigValue(
                                              sectionName,
                                              ConfigReader.ProducerInstanceParamName,
                                              string.Empty);
            }
        }
    }
}