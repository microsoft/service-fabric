// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Actions
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.FaultAnalysis.Service.Actions.Steps;
    using System.Fabric.FaultAnalysis.Service.Test;
    using System.Threading.Tasks;
    using IO;
    using Linq;

    internal abstract class ActionStateBase
    {
        private const string TraceType = "ActionStateBase";

        public ActionStateBase(Guid operationId, ActionType actionType, ServiceInternalFaultInfo serviceInternalFaultInfo)
        {
            this.OperationId = operationId;
            this.ActionType = actionType;
            this.ServiceInternalFaultInfo = serviceInternalFaultInfo;

            this.StateProgress = new Stack<StepStateNames>();
            this.StateProgress.Push(StepStateNames.IntentSaved);

            this.TimeReceived = DateTime.UtcNow;
        }

        public ActionType ActionType
        {
            get;
            private set;
        }

        public Guid OperationId
        {
            get;
            private set;
        }
      
        public Stack<StepStateNames> StateProgress
        {
            get;
            private set;
        }

        public RollbackState RollbackState
        {
            get;
            set;
        }

        // Indicates the error code that caused the action to roll back.  Not used (0) if the action is not/was not rolled back.
        public int ErrorCausingRollback
        {
            get;
            set;
        }

        // The time the input from the client is saved
        public DateTime TimeReceived
        {
            get;
            private set;
        }

        // The time the service starts running the action
        public DateTime TimeStarted
        {
            get;
            private set;
        }

        // The time an action reached Faulted or CompletedSuccessfully
        public DateTime TimeStopped
        {
            get;
            set;
        }

        // For testing - if populated specifies faults to perform on this service itself.  null means normal behavior.        
        public ServiceInternalFaultInfo ServiceInternalFaultInfo
        {
            get;
            private set;
        }

        // Note: all commands that set this to the non-default value (default is false) should set this value to true in all of its constructors.
        // In most likely implementations, this does not need to be persisted since a command of a particular ActionType should always have this set to true
        // or always have this set to false.
        public bool RetryStepWithoutRollingBackOnFailure
        {
            get;
            protected set;
        }

        // Read the first 4 bytes and translate it to the enum
        public static ActionType ReadCommandType(BinaryReader br)
        {
            ActionType a = (ActionType)br.ReadInt32();
            TestabilityTrace.TraceSource.WriteNoise(TraceType, "ReadCommandType, read type:{0}", a.ToString());
            return a;
        }

        // Any work to be executed immediately after the transaction to change a command to StepStateName.CompletedSuccessfully state.
        public virtual Task AfterSuccessfulCompletion()
        {
            return Task.FromResult(true);
        }

        // Any work to be executed immediately after the transaction to change a command to StepStateName.Failed state.
        public virtual Task AfterFatalRollback()
        {
            return Task.FromResult(true);
        }

        public void RecordActionStarted()
        {
            this.TimeStarted = DateTime.UtcNow;
        }

        public virtual void ClearInfo()
        {
        }       

        internal abstract byte[] ToBytes();

        // test only
        internal virtual bool VerifyEquals(ActionStateBase other)
        {
            bool eq = (this.OperationId == other.OperationId) &&
                (this.ActionType == other.ActionType) &&
                (this.RollbackState == other.RollbackState) &&
                (this.ErrorCausingRollback == other.ErrorCausingRollback);

            if (!eq)
            {
                TestabilityTrace.TraceSource.WriteError(TraceType, "OperationId ActionType RollbackState or ErrorCausingRollback does not match");
                return false;
            }

            if (this.StateProgress.Count != other.StateProgress.Count)
            {
                TestabilityTrace.TraceSource.WriteError(TraceType, "StateProgress.Count does not match {0} vs {1}", this.StateProgress.Count, other.StateProgress.Count);
                return false;
            }

            if (this.StateProgress.Count != 0)
            {
                if (this.StateProgress.Peek() != other.StateProgress.Peek())
                {
                    TestabilityTrace.TraceSource.WriteError(TraceType, "StateProgress.Peek does not match {0} vs {1}", this.StateProgress.Peek(), other.StateProgress.Peek());
                    return false;
                }
            }

            return true;
        }
        
        internal virtual void Write(BinaryWriter bw)
        {
            // Note: "ActionType" is written by the caller
            bw.Write(this.OperationId.ToByteArray());
          
            // StateProgress
            bw.Write(this.StateProgress.Count);

            // Reversed so it's read in correctly later
            List<StepStateNames> copy = this.StateProgress.Reverse().ToList();

            foreach (StepStateNames state in copy)
            {
                bw.Write((int)state);
            }

            bw.Write((int)this.RollbackState);
            bw.Write(this.ErrorCausingRollback);
            bw.Write(this.TimeReceived.ToBinary());
            bw.Write(this.TimeStarted.ToBinary());
            bw.Write(this.TimeStopped.ToBinary());

            // Note: "ServiceInternalFaultInfo" for testing only and is intentionally omitted  
        }

        internal virtual void Read(BinaryReader br)
        {            
            this.OperationId = new Guid(br.ReadBytes(16));
         
            int stateProgressCount = br.ReadInt32();            

            Stack<StepStateNames> tempStateProgress = new Stack<StepStateNames>();
            for (int i = 0; i < stateProgressCount; i++)
            {                
                tempStateProgress.Push((StepStateNames)br.ReadInt32());
            }

            this.StateProgress = tempStateProgress; 

            this.RollbackState = (RollbackState)br.ReadInt32();
            this.ErrorCausingRollback = br.ReadInt32();
            this.TimeReceived = DateTime.FromBinary(br.ReadInt64());
            this.TimeStarted = DateTime.FromBinary(br.ReadInt64());
            this.TimeStopped = DateTime.FromBinary(br.ReadInt64());
            
            // Note: "ServiceInternalFaultInfo" for testing only and is intentionally omitted         
        }
    }
}