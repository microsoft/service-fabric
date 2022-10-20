// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    using System.Collections.Generic;

    public interface IPerformanceCounterConfigurationSink
    {
        // Method invoked to inform the consumer about the performance counter configuration
        void RegisterPerformanceCounterConfiguration(TimeSpan samplingInterval, IEnumerable<string> counterPaths);
    }
}