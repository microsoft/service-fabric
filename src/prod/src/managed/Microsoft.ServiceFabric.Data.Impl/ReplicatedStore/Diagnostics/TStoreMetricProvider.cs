// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using Microsoft.ServiceFabric.Replicator;
using Microsoft.ServiceFabric.Replicator.Diagnostics;
using System.Collections.Generic;
using System.Fabric;

namespace Microsoft.ServiceFabric.ReplicatedStore.Diagnostics
{
    class TStoreMetricProvider : IMetricProvider
    {
        private readonly List<IMetricReporter> metricReporters;
        private readonly List<ISamplingSuggester> samplableMetrics;

        public DiskSizeMetric DiskSizeMetric { get; private set; }
        public ItemCountMetric ItemCountMetric { get; private set; }
        public KeyTypeMetric KeyTypeMetric { get; private set; }
        public KeyValueSizeMetric KeyValueSizeMetric { get; private set; }
        public ReadLatencyMetric ReadLatencyMetric { get; private set; }
        public ReadWriteMetric ReadWriteMetric { get; private set; }
        public StoreCountMetric StoreCountMetric { get; private set; }

        internal TStoreMetricProvider(ITransactionalReplicator replicator)
        {
            this.DiskSizeMetric = new DiskSizeMetric(replicator);
            this.ItemCountMetric = new ItemCountMetric(replicator);
            this.KeyTypeMetric = new KeyTypeMetric(replicator);
            this.KeyValueSizeMetric = new KeyValueSizeMetric(replicator);
            this.ReadLatencyMetric = new ReadLatencyMetric();
            this.ReadWriteMetric = new ReadWriteMetric();
            this.StoreCountMetric = new StoreCountMetric(replicator);

            // Add all metrics to list for reporting.
            metricReporters = new List<IMetricReporter>()
            {
                this.DiskSizeMetric,
                this.ItemCountMetric,   
                this.KeyTypeMetric,
                this.KeyValueSizeMetric,
                this.ReadLatencyMetric,
                this.ReadWriteMetric,
                this.StoreCountMetric
            };

            // Add any metrics to samplables list if they require
            // controlled sampling to mitigate performance hits.
            this.samplableMetrics = new List<ISamplingSuggester>(metricReporters.Count);
            foreach (var metric in this.metricReporters)
            {
                if (metric is ISamplingSuggester samplable)
                {
                    samplableMetrics.Add(samplable);
                }
            }
        }

        /// <summary>
        /// Reports the metrics' aggregated data.
        /// </summary>
        public void Report(StatefulServiceContext context)
        {
            foreach (var reporter in this.metricReporters)
            {
                reporter.Report(context);
            }
        }

        /// <summary>
        /// Re-enable measurements for any metric that requires sampling.
        /// </summary>
        public void ResetSampling()
        {
            foreach (var samplable in this.samplableMetrics)
            {
                samplable.ResetSampling();
            }
        }
    }
}
