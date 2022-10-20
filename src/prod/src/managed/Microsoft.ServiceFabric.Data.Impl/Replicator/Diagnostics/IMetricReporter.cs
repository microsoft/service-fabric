// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator.Diagnostics
{
    using System.Fabric;

    internal interface IMetricReporter
    {
        /// <summary>
        /// Reports the metric's aggregated data.
        /// </summary>
        void Report(StatefulServiceContext context);
    }
}
