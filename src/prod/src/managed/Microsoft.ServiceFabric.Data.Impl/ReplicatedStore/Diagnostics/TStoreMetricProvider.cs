// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using Microsoft.ServiceFabric.Replicator.Diagnostics;
using System.Collections.Generic;
using System.Fabric;

namespace Microsoft.ServiceFabric.ReplicatedStore.Diagnostics
{
    class TStoreMetricProvider : IMetricProvider
    {
        private readonly IReadOnlyDictionary<TStoreMetricType, IMetric> metrics;
        private readonly IEnumerable<ISamplingSuggester> samplableMetrics;

        internal TStoreMetricProvider()
        {
            metrics = new Dictionary<TStoreMetricType, IMetric>()
            {
                { TStoreMetricType.ReadLatency, new ReadLatencyMetric() }
            };

            var samplables = new List<ISamplingSuggester>();

            foreach (var metric in this.metrics.Values)
            {
                if (metric is ISamplingSuggester samplable)
                {
                    samplables.Add(samplable);
                }
            }

            this.samplableMetrics = samplables;
        }

        /// <summary>
        /// Gets the metric that is associated with the specified metric type.
        /// </summary>
        /// <param name="metricType">The metric type to locate.</param>
        /// <returns>The metric associated with the specified type.</returns>
        internal IMetric this[TStoreMetricType metricType]
        {
            get
            {
                if (metrics.TryGetValue(metricType, out IMetric result) == false)
                {
                    throw new KeyNotFoundException("TStoreMetricType not found.");
                }

                return result;
            }
        }

        /// <summary>
        /// Reports the metrics' aggregated data.
        /// </summary>
        public void Report(StatefulServiceContext context)
        {
            foreach (var metric in this.metrics.Values)
            {
                metric.Report(context);
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
