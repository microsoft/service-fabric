// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Readers.AzureTableReader
{
    using System;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;

    /// <summary>
    /// This class encapsulates the metadata related to a trace record and it's Azure table specific metadata.
    /// </summary>
    /// TODO : Improve this.
    public class TraceRecordAzureTableMetadata
    {
        public TraceRecordAzureTableMetadata(
            Type type,
            PartitionDataType dataType,
            string partitionKeyDetails,
            string tableName,
            string eventType,
            TaskName taskName)
        {
            Assert.IsNotNull(partitionKeyDetails, "partitionKeyDetails != null");
            Assert.IsNotNull(tableName, "tableName != null");
            Assert.IsNotNull(eventType, "eventType != null");

            this.Type = type;
            this.PartitionDataType = dataType;
            this.PartitionKeyDetail = partitionKeyDetails;
            this.TableName = tableName;
            this.EventType = eventType;
            this.TaskName = taskName;
        }

        public TraceRecordAzureTableMetadata(
            Type type,
            PartitionDataType dataType,
            string tableName,
            string eventType,
            TaskName taskName)
        {
            Assert.IsNotNull(tableName, "tableName != null");
            Assert.IsNotNull(eventType, "eventType != null");

            this.Type = type;
            this.PartitionDataType = dataType;

            // This is the current Schema used by the Uploader. Be careful as it can change.
            this.PartitionKeyDetail = taskName + "." + eventType;
            this.TableName = tableName;
            this.EventType = eventType;
            this.TaskName = taskName;
        }

        public Type Type { get; }

        public PartitionDataType PartitionDataType { get; }

        // Depending upon partition data type, this property will either hold the actual partition key, or the name of the property which has the partition key as data.
        public string PartitionKeyDetail { get; }

        // Name of the Azure table.
        public string TableName { get; }

        public string EventType { get; }

        public TaskName TaskName { get; }
    }
}