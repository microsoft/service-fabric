// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.Common.PerformanceTracker
{
    using System;

    public interface ITimeProvider
    {
        DateTimeOffset UtcNow { get; }
    }
}