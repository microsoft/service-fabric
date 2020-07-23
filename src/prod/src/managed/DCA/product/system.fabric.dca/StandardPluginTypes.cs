// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    using System.Collections.Generic;
    using System.Collections.ObjectModel;

    internal static class StandardPluginTypes
    {
        internal const string EtlFileProducer = "EtlFileProducer";
        internal const string EtlInMemoryProducer = "EtlInMemoryProducer";
        internal const string FolderProducer = "FolderProducer";
        internal const string LttProducer = "LttProducer";
        internal const string AzureBlobCsvUploader = "AzureBlobCsvUploader";
        internal const string PerfCounterConfigReader = "PerformanceCounterConfigReader";
        internal const string FileShareFolderUploader = "FileShareFolderUploader";
        internal const string FileShareEtwCsvUploader = "FileShareEtwCsvUploader";
        internal const string AzureBlobFolderUploader = "AzureBlobFolderUploader";
        internal const string AzureBlobEtwCsvUploader = "AzureBlobEtwCsvUploader";
        internal const string AzureBlobEtwUploader = "AzureBlobEtwUploader";
        internal const string AzureTableEtwEventUploader = "AzureTableEtwEventUploader";
        internal const string AzureTableQueryableEventUploader = "AzureTableQueryableEventUploader";
        internal const string AzureTableOperationalEventUploader = "AzureTableOperationalEventUploader";
        internal const string AzureTableQueryableCsvUploader = "AzureTableQueryableCsvUploader";
        internal const string MdsEtwEventUploader = "MdsEtwEventUploader";
        internal const string MdsFileProducer = "MdsFileProducer";
        internal const string SyslogConsumer = "SyslogConsumer";
        
        internal static readonly ReadOnlyDictionary<string, PluginType> PluginTypeMap = new ReadOnlyDictionary<string, PluginType>(
            new Dictionary<string, PluginType>
            { 
                { EtlFileProducer, PluginType.EtlFileProducer },
                { EtlInMemoryProducer, PluginType.EtlInMemoryProducer },
                { FolderProducer, PluginType.FolderProducer },
                { LttProducer, PluginType.LttProducer },
                { AzureBlobCsvUploader, PluginType.AzureBlobCsvUploader },
                { PerfCounterConfigReader, PluginType.PerfCounterConfigReader },
                { FileShareFolderUploader, PluginType.FileShareFolderUploader },
                { FileShareEtwCsvUploader, PluginType.FileShareEtwCsvUploader },
                { AzureBlobFolderUploader, PluginType.AzureBlobFolderUploader },
                { AzureBlobEtwCsvUploader, PluginType.AzureBlobEtwCsvUploader },
                { AzureBlobEtwUploader, PluginType.AzureBlobEtwUploader },
                { AzureTableEtwEventUploader, PluginType.AzureTableEtwEventUploader },
                { AzureTableQueryableEventUploader, PluginType.AzureTableQueryableEventUploader },
                { AzureTableQueryableCsvUploader, PluginType.AzureTableQueryableCsvUploader },
                { MdsEtwEventUploader, PluginType.MdsEtwEventUploader },
                { MdsFileProducer, PluginType.MdsFileProducer },
                { SyslogConsumer, PluginType.SyslogConsumer }
            });

        /// <summary>
        /// This allows for a long value with the enabled plugins set as flags.
        /// </summary>
        [Flags]
        public enum PluginType : long
        {
            None = 0,
            EtlFileProducer = 1,
            EtlInMemoryProducer = 1 << 1,
            FolderProducer = 1 << 2,
            LttProducer = 1 << 3,
            AzureBlobCsvUploader = 1 << 4,
            PerfCounterConfigReader = 1 << 5,
            FileShareFolderUploader = 1 << 6,
            FileShareEtwCsvUploader = 1 << 7,
            AzureBlobFolderUploader = 1 << 8,
            AzureBlobEtwCsvUploader = 1 << 9,
            AzureBlobEtwUploader = 1 << 10,
            AzureTableEtwEventUploader = 1 << 11,
            AzureTableQueryableEventUploader = 1 << 12,
            AzureTableQueryableCsvUploader = 1 << 13,
            MdsEtwEventUploader = 1 << 14,
            MdsFileProducer = 1 << 15,
            SyslogConsumer = 1 << 16
        }
    }
}
