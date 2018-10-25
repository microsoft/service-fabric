// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ReplicatedStore.Diagnostics
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Data.Common;
    using System.Threading;

    /// <summary>
    /// Helper class to just hold a static TStoreCounterSet.
    /// This can't be done inside TStore because it is templated and will attempt
    /// to initialize the counter set for each closed type
    /// </summary>
    class TStorePerformanceCounterSet
    {
        private static readonly FabricPerformanceCounterSet CounterSet = InitializePerformanceCounterDefinition();
        private static long InstanceDifferentiator = 0;

        private static FabricPerformanceCounterSet InitializePerformanceCounterDefinition()
        {
            var performanceCounters = new TStorePerformanceCounters();
            var counterSets = performanceCounters.GetCounterSets();

            Diagnostics.Assert(counterSets.Keys.Count == 1, "More than 1 category of perf counters found in TStore");

            var enumerator = counterSets.Keys.GetEnumerator();
            enumerator.MoveNext();
            var key = enumerator.Current;

            return new FabricPerformanceCounterSet(key, counterSets[key]);
        }

        public static FabricPerformanceCounterSetInstance CreateCounterSetInstance(Guid partitionId, long replicaId, long stateProviderId)
        {
            // The <instanceDifferentiator> is used to protect us from running into an issue where the Store could be initialized
            // twice without a cleanup in between. By appending the <instanceDifferentiator> portion, we ensure that the old and
            // new performance counter instances avoid any name collision.
            var instanceDifferentiator = Interlocked.Increment(ref InstanceDifferentiator);

            var instanceName = string.Format(
                "{0}:{1}:{2}_{3}",
                partitionId,
                replicaId,
                stateProviderId,
                instanceDifferentiator);
            return CounterSet.CreateCounterSetInstance(instanceName);
        }
    }
}
