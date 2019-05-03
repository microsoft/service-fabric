// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using ClusterAnalysis.TraceAccessLayer;
using EventsReader.Models.Partition;
using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords;
using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Testability;

namespace EventsReader.DataReader
{
    /// <summary>
    /// Create Filters for different Entity Types.
    /// </summary>
    internal static class FilterFactory
    {
        private const string NodeFilterName = "nodeName";
        private const string ApplicationFilterName = "applicationName";
        private const string ServiceFilterName = "serviceName";
        private const string PartitionFilterName = "partitionId";
        private const string ReplicaFilterName = "replicaId";
        private const string EventInstanceFilterName = "eventInstanceId";

        public static ReadFilter CreateApplicationFilter(ITraceStoreReader traceStoreReader, IList<Type> types, string appName)
        {
            var filter = ReadFilter.CreateReadFilter(types);
            if (!string.IsNullOrEmpty(appName) && traceStoreReader.IsPropertyLevelFilteringSupported())
            {
                foreach (var oneType in types)
                {
                    filter.AddFilter(oneType, ApplicationFilterName, appName);
                }
            }

            return filter;
        }

        public static ReadFilter CreateServiceFilter(ITraceStoreReader traceStoreReader, IList<Type> types, string serviceName)
        {
            var filter = ReadFilter.CreateReadFilter(types);
            if (!string.IsNullOrEmpty(serviceName) && traceStoreReader.IsPropertyLevelFilteringSupported())
            {
                foreach (var oneType in types)
                {
                    filter.AddFilter(oneType, ServiceFilterName, serviceName);
                }
            }

            return filter;
        }


        public static ReadFilter CreateNodeFilter(ITraceStoreReader traceStoreReader, IList<Type> types, string nodeName)
        {
            var filter = ReadFilter.CreateReadFilter(types);
            if (!string.IsNullOrEmpty(nodeName) && traceStoreReader.IsPropertyLevelFilteringSupported())
            {
                foreach (var oneType in types)
                {
                    filter.AddFilter(oneType, NodeFilterName, nodeName);
                }
            }

            return filter;
        }

        public static ReadFilter CreatePartitionFilter(ITraceStoreReader traceStoreReader, IList<Type> types, Guid partitionId)
        {
            var filter = ReadFilter.CreateReadFilter(types);
            if (partitionId != Guid.Empty && traceStoreReader.IsPropertyLevelFilteringSupported())
            {
                foreach (var oneType in types)
                {
                    filter.AddFilter(oneType, PartitionFilterName, partitionId);
                }
            }

            return filter;
        }

        public static ReadFilter CreateReplicaFilter(ITraceStoreReader traceStoreReader, IList<Type> types, Guid partitionId, long? replicaId)
        {
            var filter = ReadFilter.CreateReadFilter(types);
            if (partitionId != Guid.Empty && traceStoreReader.IsPropertyLevelFilteringSupported())
            {
                foreach (var oneType in types)
                {
                    filter.AddFilter(oneType, PartitionFilterName, partitionId);
                    if (replicaId.HasValue)
                    {
                        filter.AddFilter(oneType, ReplicaFilterName, replicaId);
                    }
                }
            }

            return filter;
        }

        public static ReadFilter CreateTypeAndIdFilter(ITraceStoreReader traceStoreReader, Type type, Guid eventInstanceId)
        {
            var filter = ReadFilter.CreateReadFilter(type);
            if (eventInstanceId != Guid.Empty && traceStoreReader.IsPropertyLevelFilteringSupported())
            {
                filter.AddFilter(type, EventInstanceFilterName, eventInstanceId);
            }

            return filter;
        }

        public static ReadFilter CreateClusterFilter(IList<Type> types)
        {
            return ReadFilter.CreateReadFilter(types);
        }

        public static ReadFilter CreateCorrelationEventFilter(ITraceStoreReader reader, IList<Guid> eventInstances)
        {
            var type = typeof(CorrelationTraceRecord);
            var filter = ReadFilter.CreateReadFilter(type);

            if(!reader.IsPropertyLevelFilteringSupported())
            {
                return filter;
            }

            foreach (var oneInstance in eventInstances)
            {
                filter.AddFilter(type, CorrelationTraceRecord.RelatedFromIdPropertyName, oneInstance);
                filter.AddFilter(type, CorrelationTraceRecord.RelatedToIdPropertyName, oneInstance);
            }

            return filter;
        }
    }
}
