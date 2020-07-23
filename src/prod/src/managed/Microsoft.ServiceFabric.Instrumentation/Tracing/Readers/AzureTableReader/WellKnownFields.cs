// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Readers.AzureTableReader
{
    /// <summary>
    /// Encapsulate well known Fields
    /// </summary>
    public class WellKnownFields
    {
        // Partition Key for Table
        public const string PartitionKey = "PartitionKey";

        // Row Key for Table
        public const string RowKey = "RowKey";

        /// <summary>
        /// The field which contains the time the row was created in Azure table.
        /// </summary>
        public const string TimeStamp = "Timestamp";

        /// <summary>
        /// Name of the field containing the Event Instance Id
        /// </summary>
        public const string EventInstanceId = "eventInstanceId";

        /// <summary>
        /// Name of the field that identifies Node Name for Node Event store
        /// </summary>
        public const string NodeName = "nodeName";

        /// <summary>
        /// Name of the field that identifies Application
        /// </summary>
        /// TODO: Change to id.
        public const string ApplicationId = "applicationName";

        public const string ApplicationTypeName = "applicationTypeName";

        // Name of the field that identifies Service
        public const string ServiceId = "serviceId";

        /// <summary>
        /// Name of the field that identifies Partition
        /// </summary>
        public const string PartitionId = "partitionId";

        /// <summary>
        /// Name of the field that identifies Replica
        /// </summary>
        public const string ReplicaId = "ReplicaId";

        public const string EventType = "EventType";
        public const string EventVersion = "EventVersion";
        public const string TaskName = "TaskName";

        public const string EventDataId = "id";

        public const string DcaInstance = "dca_instance";
        public const string DcaVersion = "dca_version";
        public const string IsDeleted = "isDeleted";

        public const char Underscore = '_';
        public const char Dot = '.';
    }
}