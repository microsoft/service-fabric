// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Readers.AzureTableReader
{
    using System;

    public interface IFilterBuilder
    {
        string BuildFilter(string propertyFilter, DateTimeOffset startTime, DateTimeOffset endTime);
    }
}
