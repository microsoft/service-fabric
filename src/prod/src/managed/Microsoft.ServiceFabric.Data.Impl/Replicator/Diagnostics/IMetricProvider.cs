// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric;

namespace Microsoft.ServiceFabric.Replicator.Diagnostics
{
    internal interface IMetricProvider
    {
        /// <summary>
        /// Reports the metrics' aggregated data.
        /// </summary>
        void Report(StatefulServiceContext context);

        /// <summary>
        /// Re-enable measurements for any metric that requires sampling.
        /// </summary>
        void ResetSampling();
    }
}
