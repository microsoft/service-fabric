// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Data.Common;
    using System.Globalization;
    using System.Threading;

    /// <summary>
    /// Tracks the total number of values being loaded.
    /// </summary>
    internal class LoadValueCounter
    {
        private readonly IStoreSweepProvider store;
        private long count;
        private string traceType;

        public LoadValueCounter(IStoreSweepProvider store, string traceType)
        {
            this.store = store;
            this.count = 0;
            this.traceType = traceType;
        }

        public long Count
        {
            get
            {
                return this.count;
            }
        }

        public void IncrementCounter()
        {
            if (Interlocked.Increment(ref this.count) >= this.store.SweepThreshold)
            {
#if !DotNetCoreClr
                FabricEvents.Events.SweepOnReads(this.traceType);
#else
                AppTrace.TraceSource.WriteInfo(string.Format(CultureInfo.InvariantCulture, "TStore.Sweep.OnReads@{0}", this.traceType), "starting");
#endif
                this.store.TryStartSweep();
            }

            Diagnostics.Assert(this.count >= 0, this.traceType, "Invalid ");
        }

        public void ResetCounter()
        {
            Interlocked.Exchange(ref this.count, 0);
        }
    }
}