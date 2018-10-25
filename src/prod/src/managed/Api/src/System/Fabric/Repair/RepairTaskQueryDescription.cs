// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Repair
{
    using System;
    using System.Fabric.Interop;

    internal sealed class RepairTaskQueryDescription
    {
        internal RepairTaskQueryDescription(
            RepairScopeIdentifier scope,
            string taskIdFilter,
            RepairTaskStateFilter stateFilter,
            string executorFilter)
        {
            this.Scope = scope;
            this.TaskIdFilter = taskIdFilter;
            this.StateFilter = stateFilter;
            this.ExecutorFilter = executorFilter;
        }

        public RepairScopeIdentifier Scope { get; set; }
        public string TaskIdFilter { get; set; }
        public RepairTaskStateFilter StateFilter { get; set; }
        public string ExecutorFilter { get; set; }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_REPAIR_TASK_QUERY_DESCRIPTION()
            {
                Scope = (this.Scope == null ? IntPtr.Zero : this.Scope.ToNative(pinCollection)),
                TaskIdFilter = pinCollection.AddBlittable(this.TaskIdFilter),
                StateFilter = (uint)this.StateFilter,
                ExecutorFilter = pinCollection.AddBlittable(this.ExecutorFilter),
            };

            return pinCollection.AddBlittable(nativeDescription);
        }
    }
}