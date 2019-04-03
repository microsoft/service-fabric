// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.WindowsFabricValidator
{
    using System.Collections.Generic;
    using FabricDCA;
    using System.Reflection;

    class PluginTypeInfo
    {
        internal string AssemblyName;
        internal string TypeName;
    }

    class DcaStandardPluginValidators
    {
#if DotNetCoreClrLinux
        public const string LttProducer = "LttProducer";
        public const string AzureBlobCsvUploader = "AzureBlobCsvUploader";
        public const string AzureTableQueryableCsvUploader = "AzureTableQueryableCsvUploader";
#endif
        public const string EtlFileProducer = "EtlFileProducer";
        public const string EtlInMemoryProducer = "EtlInMemoryProducer";
        public const string FolderProducer = "FolderProducer";
        public const string PerfCounterConfigReader = "PerformanceCounterConfigReader";
        public const string FileShareFolderUploader = "FileShareFolderUploader";
        public const string FileShareEtwCsvUploader = "FileShareEtwCsvUploader";
        public const string AzureBlobFolderUploader = "AzureBlobFolderUploader";
        public const string AzureBlobEtwCsvUploader = "AzureBlobEtwCsvUploader";
        public const string AzureBlobEtwUploader = "AzureBlobEtwUploader";
        public const string AzureTableEtwEventUploader = "AzureTableEtwEventUploader";
        public const string AzureTableQueryableEventUploader = "AzureTableQueryableEventUploader";
        public const string AzureTableOperationalEventUploader = "AzureTableOperationalEventUploader";
        public const string MdsEtwEventUploader = "MdsEtwEventUploader";
        public const string MdsFileProducer = "MdsFileProducer";
        public const string SyslogConsumer = "SyslogConsumer";

        internal static Dictionary<string, PluginTypeInfo> ProducerValidators = new Dictionary<string, PluginTypeInfo>()
            {
#if !DotNetCoreClrLinux
                {
                    EtlFileProducer, 
                    new PluginTypeInfo() 
                    {
                        AssemblyName = typeof(FabricDCA.EtlProducerValidator).GetTypeInfo().Assembly.FullName, 
                        TypeName = typeof(FabricDCA.EtlProducerValidator).FullName
                    } 
                },
                {
                    EtlInMemoryProducer, 
                    new PluginTypeInfo() 
                    {
                        AssemblyName = typeof(FabricDCA.EtlInMemoryProducerValidator).GetTypeInfo().Assembly.FullName, 
                        TypeName = typeof(FabricDCA.EtlInMemoryProducerValidator).FullName
                    } 
                },
    #if !DotNetCoreClrIOT
                { 
                    PerfCounterConfigReader, 
                    new PluginTypeInfo() 
                    {
                        AssemblyName = typeof(FabricDCA.PerfCounterConfigReaderValidator).GetTypeInfo().Assembly.FullName, 
                        TypeName = typeof(FabricDCA.PerfCounterConfigReaderValidator).FullName
                    } 
                },
    #endif
#else
                {
                    LttProducer,
                    new PluginTypeInfo()
                    {
                        AssemblyName = typeof(FabricDCA.LttProducerValidator).GetTypeInfo().Assembly.FullName,
                        TypeName = typeof(FabricDCA.LttProducerValidator).FullName
                    }
                },
#endif
                {
                    FolderProducer,
                    new PluginTypeInfo()
                    {
                        AssemblyName = typeof(FabricDCA.FolderProducerValidator).GetTypeInfo().Assembly.FullName,
                        TypeName = typeof(FabricDCA.FolderProducerValidator).FullName
                    }
                },
            };

        internal static Dictionary<string, PluginTypeInfo> ConsumerValidators = new Dictionary<string, PluginTypeInfo>()
            {
#if !DotNetCoreClrLinux
                { 
                    FileShareEtwCsvUploader, 
                    new PluginTypeInfo() 
                    {
                        AssemblyName = typeof(FabricDCA.FileShareEtwCsvUploaderValidator).GetTypeInfo().Assembly.FullName, 
                        TypeName = typeof(FabricDCA.FileShareEtwCsvUploaderValidator).FullName
                    } 
                }, 
                { 
                    FileShareFolderUploader, 
                    new PluginTypeInfo() 
                    {
                        AssemblyName = typeof(FabricDCA.FileShareFolderUploaderValidator).GetTypeInfo().Assembly.FullName, 
                        TypeName = typeof(FabricDCA.FileShareFolderUploaderValidator).FullName
                    } 
                },
                { 
                    AzureBlobEtwCsvUploader, 
                    new PluginTypeInfo() 
                    {
                        AssemblyName = typeof(FabricDCA.AzureBlobEtwCsvUploaderValidator).GetTypeInfo().Assembly.FullName, 
                        TypeName = typeof(FabricDCA.AzureBlobEtwCsvUploaderValidator).FullName
                    } 
                },
    #if !DotNetCoreClrIOT
                { 
                    AzureBlobEtwUploader, 
                    new PluginTypeInfo() 
                    {
                        AssemblyName = typeof(FabricDCA.AzureBlobEtwUploaderValidator).GetTypeInfo().Assembly.FullName, 
                        TypeName = typeof(FabricDCA.AzureBlobEtwUploaderValidator).FullName
                    } 
                },
                { 
                    AzureTableEtwEventUploader, 
                    new PluginTypeInfo() 
                    {
                        AssemblyName = typeof(FabricDCA.AzureTableEtwEventUploaderValidator).GetTypeInfo().Assembly.FullName, 
                        TypeName = typeof(FabricDCA.AzureTableEtwEventUploaderValidator).FullName
                    } 
                },
    #endif
                { 
                    AzureTableQueryableEventUploader, 
                    new PluginTypeInfo() 
                    {
                        AssemblyName = typeof(FabricDCA.AzureTableSelectiveEventUploaderValidator).GetTypeInfo().Assembly.FullName, 
                        TypeName = typeof(FabricDCA.AzureTableSelectiveEventUploaderValidator).FullName
                    } 
                },
    #if !DotNetCoreClrIOT
                {
                    AzureTableOperationalEventUploader,
                    new PluginTypeInfo()
                    {
                        AssemblyName = typeof(FabricDCA.AzureTableSelectiveEventUploaderValidator).GetTypeInfo().Assembly.FullName,
                        TypeName = typeof(FabricDCA.AzureTableSelectiveEventUploaderValidator).FullName
                    }
                },
                { 
                    MdsFileProducer, 
                    new PluginTypeInfo() 
                    {
                        AssemblyName = typeof(FabricDCA.MdsFileProducerValidator).GetTypeInfo().Assembly.FullName, 
                        TypeName = typeof(FabricDCA.MdsFileProducerValidator).FullName
                    } 
                },
                { 
                    MdsEtwEventUploader, 
                    new PluginTypeInfo() 
                    {
                        AssemblyName = typeof(FabricDCA.MdsEtwEventUploaderValidator).GetTypeInfo().Assembly.FullName, 
                        TypeName = typeof(FabricDCA.MdsEtwEventUploaderValidator).FullName
                    } 
                },
    #endif
#else
                {
                    AzureBlobCsvUploader,
                    new PluginTypeInfo()
                    {
                        AssemblyName = typeof(FabricDCA.AzureBlobCsvUploaderValidator).GetTypeInfo().Assembly.FullName,
                        TypeName = typeof(FabricDCA.AzureBlobCsvUploaderValidator).FullName
                    }
                },
                { 
                    AzureTableQueryableCsvUploader, 
                    new PluginTypeInfo() 
                    {
                        AssemblyName = typeof(FabricDCA.AzureTableSelectiveEventUploaderValidator).GetTypeInfo().Assembly.FullName, 
                        TypeName = typeof(FabricDCA.AzureTableSelectiveEventUploaderValidator).FullName
                    } 
                },
                {
                    AzureTableOperationalEventUploader,
                    new PluginTypeInfo()
                    {
                        AssemblyName = typeof(FabricDCA.AzureTableSelectiveEventUploaderValidator).GetTypeInfo().Assembly.FullName,
                        TypeName = typeof(FabricDCA.AzureTableSelectiveEventUploaderValidator).FullName
                    }
                },
                {
                    SyslogConsumer,
                    new PluginTypeInfo()
                    {
                        AssemblyName = typeof(FabricDCA.SyslogConsumerValidator).GetTypeInfo().Assembly.FullName,
                        TypeName = typeof(FabricDCA.SyslogConsumerValidator).FullName
                    }
                },
#endif
                {
                    AzureBlobFolderUploader,
                    new PluginTypeInfo()
                    {
                        AssemblyName = typeof(FabricDCA.AzureBlobFolderUploaderValidator).GetTypeInfo().Assembly.FullName,
                        TypeName = typeof(FabricDCA.AzureBlobFolderUploaderValidator).FullName
                    }
                },
            };

        internal static Dictionary<string, string[]> ProducerConsumerCompat = new Dictionary<string, string[]>()
            {
#if DotNetCoreClrLinux
                {
                    LttProducer,
                    new string[]
                    {
                        AzureBlobCsvUploader,
                        AzureTableQueryableCsvUploader,
                        AzureTableOperationalEventUploader,
                        SyslogConsumer
                    }
                },
#else
                {
                    EtlFileProducer,
                    new string[]
                    {
                        FileShareEtwCsvUploader,
                        AzureBlobEtwCsvUploader,
                        AzureTableEtwEventUploader,
                        AzureTableQueryableEventUploader,
                        MdsEtwEventUploader
                    }
                },
                {
                    EtlInMemoryProducer,
                    new string[]
                    {
                        MdsFileProducer,
                        AzureBlobEtwUploader
                    }
                },
#endif
                {
                    FolderProducer,
                    new string[]
                    {
                        FileShareFolderUploader,
                        AzureBlobFolderUploader
                    }
                },
           };
    }
}
