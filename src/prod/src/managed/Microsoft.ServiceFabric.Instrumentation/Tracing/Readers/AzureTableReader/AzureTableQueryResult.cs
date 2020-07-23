// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Readers.AzureTableReader
{
    using System.Collections.Generic;

    public sealed class AzureTableQueryResult
    {
        public AzureTableQueryResult(IEnumerable<AzureTablePropertyBag> properties)
        {
            this.AzureTableProperties = properties;
        }

        public IEnumerable<AzureTablePropertyBag> AzureTableProperties { get; }
    }
}