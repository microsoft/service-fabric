// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.Common.Store
{
    using System;

    public abstract class DataRetentionPolicy
    {
        public TimeSpan GapBetweenScrubs { get; protected set; }
    }
}