// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsReader
{
    internal class ReaderConstants
    {
        public const string GETQuery = "getQuery";
        public const string OutputFile = "outputFile";
        public const string ErrorFileExt = ".error";

        public const char SegmentDelimiter = '/';
        public const char QueryStringDelimiter = '?';
        public const char FragmentDelimiter = '#';
        public const string MetadataSegment = "$";
        public const char And = '&';
        public const char Equal = '=';

        public const string Cluster = "cluster";
        public const string Containers = "containers";
        public const string Nodes = "nodes";
        public const string Applications = "applications";
        public const string Services = "services";
        public const string Partitions = "partitions";
        public const string Replicas = "replicas";
        public const string PartitionsReplicas = "partitions:replicas";
        public const string CorrelatedEvents = "correlatedevents";

        public const string NodeName = "nodeName";
        public const string ApplicationId = "applicationId";
        public const string ServiceId = "serviceId";
        public const string PartitionId = "partitionId";
        public const string ReplicaId = "replicaId";
        public const string EventInstanceId = "eventInstanceId";

        public const string EventsStoreBaseCollection = "EventsStore";
        public const string EventsCollection = "Events";
        public const string ApiVersion6dot2Preview = "6.2-preview";
        public const string ApiVersion6dot3Preview = "6.3-preview";

        internal class ClusterSettings
        {
            public const string Diagnostics = "Diagnostics";
            public const string ConsumerInstances = "ConsumerInstances";
            public const string ConsumerType = "ConsumerType";
            public const string TableNamePrefix = "TableNamePrefix";
            public const string StoreConnectionString = "StoreConnectionString";
            public const string IsEnabled = "IsEnabled";
        }
    }
}