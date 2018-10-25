// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Actions
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.FaultAnalysis.Service.Actions.Steps;
    using System.Threading;
    using Microsoft.ServiceFabric.Data;

    [SuppressMessage("Microsoft.StyleCop.CSharp.MaintainabilityRules", "SA1402:FileMayOnlyContainASingleClass", Justification = "Related")]
    internal abstract class FabricTestAction<TResult> : FabricTestAction
    {
        protected FabricTestAction(TimeSpan requestTimeout, TimeSpan operationTimeout)
            : base(null, null, null, requestTimeout, operationTimeout)
        {
        }

        public TResult Result { get; set; }
    }

    [SuppressMessage("Microsoft.StyleCop.CSharp.MaintainabilityRules", "SA1402:FileMayOnlyContainASingleClass", Justification = "Related")]
    internal abstract class FabricTestAction
    {
        protected FabricTestAction(IReliableStateManager stateManager, IStatefulServicePartition partition, ActionStateBase actionState, TimeSpan requestTimeout, TimeSpan operationTimeout)
        {
            this.StateManager = stateManager;
            this.Partition = partition;
            this.State = actionState;
            this.RequestTimeout = requestTimeout;
            this.OperationTimeout = operationTimeout;
        }

        public IReliableStateManager StateManager
        {
            get;
            private set;
        }

        public IStatefulServicePartition Partition
        {
            get;
            private set;
        }

        public ActionStateBase State
        {
            get;
            private set;
        }

        public TimeSpan RequestTimeout
        {
            get;
            set;
        }

        public TimeSpan OperationTimeout
        {
            get;
            set;
        }

        public virtual StepBase GetStep(
            FabricClient fabricClient,
            ActionStateBase actionState,
            StepStateNames stateName,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }
    }
}