// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Test
{
    using System.Fabric.FaultAnalysis.Service.Actions;
    using System.Fabric.FaultAnalysis.Service.Actions.Steps;
    using System.Runtime.Serialization;

    internal enum ServiceInternalFaultType
    {
        None,
        KillProcess,
        ReportFaultTransient,
        ReportFaultPermanent,
        RollbackAction,
        RollbackActionAndRetry,

        // Intentionally fail the step step a few times, then run it normally (pass).  It should succeed.
        ThrowThreeTimes,
    }

    internal class ServiceInternalFaultInfo
    {      
        public ServiceInternalFaultInfo(StepStateNames targetStepStateName, ServiceInternalFaultType faultType)
            : this(targetStepStateName, faultType, RollbackState.NotRollingBack, null)
        {
            this.TargetStepStateName = targetStepStateName;
            this.FaultType = faultType;
        }

        public ServiceInternalFaultInfo(StepStateNames targetStepStateName, ServiceInternalFaultType faultType, RollbackState rollbackState, string location)
            : this(targetStepStateName, faultType, rollbackState, location, StepStateNames.None)
        {
        }

        public ServiceInternalFaultInfo(StepStateNames targetStepStateName, ServiceInternalFaultType faultType, RollbackState rollbackState, string location, StepStateNames targetCancelStepStateName)
        {
            // The step in which to do the internal fault (other than user cancel)
            this.TargetStepStateName = targetStepStateName;

            // What type of intentional internal fault to do
            this.FaultType = faultType;

            // The type of user cancel to inject.  See ActionTest.PerformTestUserCancelIfRequested().
            this.RollbackType = rollbackState;

            // The engine location in which to perform the user cancel.  Note, not the same as the step to do it in, which is in TargetCancelStepStateName.
            this.Location = location;

            // The step in which to do user cancellation
            this.TargetCancelStepStateName = targetCancelStepStateName;

            this.CountForThrowThreeTimesFault = 3;
        }

        public StepStateNames TargetStepStateName
        {
            get;
            private set;
        }

        public string Location
        {
            get;
            private set;
        }

        public ServiceInternalFaultType FaultType
        {
            get;
            private set;
        }

        public RollbackState RollbackType
        {
            get;
            private set;
        }

        // Used for the step in which to do user cancellation.  None means does not matter.
        public StepStateNames TargetCancelStepStateName
        {
            get;
            private set;
        }

        // Used to count the number of remaining retries when using ServiceInternalFaultType.ThrowThreeTimes.  Internal test only.  
        public int CountForThrowThreeTimesFault
        {
            get;
            set;
        }

        public void SetFaultTypeToNone()
        {
            this.FaultType = ServiceInternalFaultType.None;
        }
    }
}