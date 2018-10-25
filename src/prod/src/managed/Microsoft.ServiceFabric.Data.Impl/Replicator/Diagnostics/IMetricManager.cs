// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator.Diagnostics
{
    using System.Fabric;
    using System.Threading.Tasks;

    internal interface IMetricManager
    {
        /// <summary>
        /// Initialize metric manager with stateful service context information.
        /// </summary>
        /// <param name="statefulServiceContext"></param>
        void Initialize(StatefulServiceContext statefulServiceContext);

        /// <summary>
        /// Gets the metric provider that is associated with the specified type.
        /// </summary>
        /// <param name="type">The metric provider type to locate.</param>
        /// <returns>The metric provider associated with the specified type.</returns>
        IMetricProvider GetProvider(MetricProviderType type);

        /// <summary>
        /// Start metric aggregation and reporting task.
        /// </summary>
        void StartTask();

        /// <summary>
        /// Stop metric aggregation and reporting task.
        /// </summary>
        Task StopTaskAsync();
    }
}