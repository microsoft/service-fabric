// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Dca;
    using System.IO;
    using System.Linq;
    using System.Reflection;

    internal static class StandardPlugins
    {
        private const string EventType = "StandardPlugins";

        private static readonly string[] DefaultProducerList;

        private static Dictionary<string, PluginTypeInfo> consumers;

        static StandardPlugins()
        {
            DefaultProducerList = new[]
                        {
#if DotNetCoreClrLinux
                            StandardPluginTypes.LttProducer,
#else
                            StandardPluginTypes.EtlFileProducer,
                            StandardPluginTypes.EtlInMemoryProducer,
 #if !DotNetCoreClrIOT
                            StandardPluginTypes.PerfCounterConfigReader,
#endif
#endif
                            StandardPluginTypes.FolderProducer
                        };

            consumers = new Dictionary<string, PluginTypeInfo>();
            Dictionary<string, Type> consumerTypes = InitializeConsumerTypes();

            Utility.TraceSource.WriteInfo(
                EventType,
                "Found following consumer types available: {0}.", 
                string.Join(", ", consumerTypes.Keys));

            foreach (string consumerName in consumerTypes.Keys)
            {
                Consumers[consumerName] = new PluginTypeInfo(
                    consumerTypes[consumerName].GetTypeInfo().Assembly.FullName,
                    consumerTypes[consumerName].FullName);
            }
        }

        internal static string[] Producers
        {
            get { return DefaultProducerList; }
        }

        internal static Dictionary<string, PluginTypeInfo> Consumers
        {
            get { return consumers; }
            set { consumers = value; }
        }

        private static Dictionary<string, Type> InitializeConsumerTypes()
        {
            Dictionary<string, Type> pluginTypes = new Dictionary<string, Type>();
#if !DotNetCoreClrLinux
            pluginTypes[StandardPluginTypes.FileShareEtwCsvUploader] = typeof(FileShareEtwCsvUploader);
            pluginTypes[StandardPluginTypes.FileShareFolderUploader] = typeof(FileShareFolderUploader);
#endif

            // Add Syslog consumer on Linux system.
#if DotNetCoreClrLinux
            pluginTypes[StandardPluginTypes.SyslogConsumer] = typeof(SyslogConsumer);
#endif

            // In the Windows Server environment, we don't ship any Azure plugins. So an exception will be raised
            // while JIT-compiling the method below because it references some types defined in the Azure plugins.
            // If that happens, handle the exception and move on.
            try
            {
                AddAzureSpecificConsumers(pluginTypes);
            }
            catch (FileNotFoundException e)
            {
                Utility.TraceSource.WriteInfo(
                    EventType,
                    "Azure-specific consumer plugins are not available. Assume that we are running in Windows Server environment. {0}",
                    e);
            }

            try
            {
                AddMicrosoftInternalConsumers(pluginTypes);
            }
            catch (FileNotFoundException e)
            {
                Utility.TraceSource.WriteInfo(
                    EventType,
                    "Microsoft internal consumer plugins are not available. Assume that we are running in a public environment. {0}",
                    e);
            }

            return pluginTypes;
        }

        private static void AddAzureSpecificConsumers(Dictionary<string, Type> pluginTypes)
        {
#if DotNetCoreClrLinux
            pluginTypes[StandardPluginTypes.AzureBlobCsvUploader] = typeof(AzureBlobCsvUploader);
            pluginTypes[StandardPluginTypes.AzureTableQueryableCsvUploader] = typeof(AzureTableQueryableCsvUploader);
            pluginTypes[StandardPluginTypes.AzureTableOperationalEventUploader] = typeof(AzureTableOperationalEventUploader);
#else
            pluginTypes[StandardPluginTypes.AzureBlobEtwCsvUploader] = typeof(AzureBlobEtwCsvUploader);
            pluginTypes[StandardPluginTypes.AzureTableQueryableEventUploader] = typeof(AzureTableQueryableEventUploader);
#if !DotNetCoreClrIOT
            pluginTypes[StandardPluginTypes.AzureBlobEtwUploader] = typeof(AzureBlobEtwUploader);
            pluginTypes[StandardPluginTypes.AzureTableEtwEventUploader] = typeof(AzureTableEtwEventUploader);
            pluginTypes[StandardPluginTypes.AzureTableOperationalEventUploader] = typeof(AzureTableOperationalEventUploader);
#endif
#endif
            pluginTypes[StandardPluginTypes.AzureBlobFolderUploader] = typeof(AzureBlobFolderUploader);
        }

        private static void AddMicrosoftInternalConsumers(Dictionary<string, Type> pluginTypes)
        {
#if !DotNetCoreClrLinux && !DotNetCoreClrIOT
            pluginTypes[StandardPluginTypes.MdsEtwEventUploader] = typeof(MdsEtwEventUploader);
            pluginTypes[StandardPluginTypes.MdsFileProducer] = typeof(MdsFileProducer);
#endif
        }

        internal class PluginTypeInfo
        {
            private readonly string assemblyName;
            private readonly string typeName;

            public PluginTypeInfo(string assemblyName, string typeName)
            {
                this.assemblyName = assemblyName;
                this.typeName = typeName;
            }

            public string AssemblyName 
            {
                get { return this.assemblyName; }
            }

            public string TypeName
            {
                get { return this.typeName; }
            }
        }
    }
}