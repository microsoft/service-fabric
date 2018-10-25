// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Linq;

    internal abstract class PartitionBase
    {
        public PartitionBase()
        {
        }

        public Guid Id
        {
            get;
            private set;
        }

        public ServicePartitionKind ServicePartitionKind
        {
            get;
            private set;
        }

        public ServicePartitionInformation PartitionInfo
        {
            get;
            protected set;
        }

        public void ReportLoad(IEnumerable<LoadMetric> metrics)
        {
            Requires.Argument("metrics", metrics).NotNull();

            Utility.WrapNativeSyncInvoke(() => this.ReportLoadHelper(metrics), "ServicePartition.ReportLoad");
        }

        // Methods implemented by StateFulPartition and StatelessPartition
        public abstract void ReportLoad(NativeTypes.FABRIC_LOAD_METRIC[] loadmetrics, PinCollection pin);

        private void ReportLoadHelper(IEnumerable<LoadMetric> metrics)
        {
            NativeTypes.FABRIC_LOAD_METRIC[] loadmetrics = new NativeTypes.FABRIC_LOAD_METRIC[metrics.Count()];
            using (PinCollection pin = new PinCollection())
            {
                int index = 0;
                foreach (LoadMetric metric in metrics)
                {
                    loadmetrics[index].Name = pin.AddBlittable(metric.Name);
                    loadmetrics[index].Value = checked((uint)metric.Value);
                    index++;
                }

                this.ReportLoad(loadmetrics, pin);
            }
        }
    }
}