// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Collections.Generic;

    internal interface IFabricPerformanceCountersDefinition
    {
        Dictionary<FabricPerformanceCounterSetDefinition, IEnumerable<FabricPerformanceCounterDefinition>> GetCounterSets();
    }
}