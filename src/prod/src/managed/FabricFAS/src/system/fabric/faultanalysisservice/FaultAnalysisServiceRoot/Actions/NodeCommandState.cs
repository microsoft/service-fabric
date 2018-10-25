// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Actions
{
    using System.Fabric.FaultAnalysis.Service.Test;
    using System.Numerics;
    using Engine;
    using IO;
    using Microsoft.ServiceFabric.Data.Collections;
    using Threading.Tasks;

    internal class NodeCommandState : ActionStateBase
    {
        // For deserialization only
        public NodeCommandState(ActionType actionType)
            : base(Guid.Empty, actionType, null)  // Default values that will be overwritten
        {
            // Default values that will be overwritten
            this.Info = new NodeCommandInfo(null, BigInteger.MinusOne, 0);
            this.RetryStepWithoutRollingBackOnFailure = true;
        }

        public NodeCommandState(ActionType actionType, Guid operationId, NodeCommandSynchronizer nodeSync, ServiceInternalFaultInfo serviceInternalFaultInfo, string nodeName, BigInteger nodeInstanceId, int stopDurationInSeconds)
            : base(operationId, actionType, serviceInternalFaultInfo)
        {
            this.Info = new NodeCommandInfo(nodeName, nodeInstanceId, stopDurationInSeconds);
            this.NodeSync = nodeSync;
            this.RetryStepWithoutRollingBackOnFailure = true;
        }

        public NodeCommandInfo Info
        {
            get;
            private set;
        }

        public IReliableDictionary<string, bool> StoppedNodeTable
        {
            get;
            set;
        }

        // not replicated
        public NodeCommandSynchronizer NodeSync
        {
            get;
            set;
        }        

        public override Task AfterSuccessfulCompletion()
        {
            this.NodeSync.Remove(this.Info.NodeName);
            return Task.FromResult(true);
        }

        public override Task AfterFatalRollback()
        {
            this.NodeSync.Remove(this.Info.NodeName);
            return Task.FromResult(true);
        }

        public override void ClearInfo()
        {
            string nodeName = this.Info.NodeName;
            BigInteger apiInputNodeInstanceId = this.Info.InputNodeInstanceId;
            int stopDurationInSeconds = this.Info.StopDurationInSeconds;

            this.Info = new NodeCommandInfo(nodeName, apiInputNodeInstanceId, stopDurationInSeconds);
        }

        public override string ToString()
        {
            string s = System.Fabric.Common.StringHelper.Format(
                "Info: {0}",
                this.Info.ToString());

            return s;
        }

        internal static NodeCommandState FromBytes(BinaryReader br, ActionType type)
        {
            NodeCommandState state = new NodeCommandState(type);

            // Note: the type of command (an Enum serialized as Int32) was already read by caller
            state.Read(br);

            return state;
        }

        // test only
        internal override bool VerifyEquals(ActionStateBase other)
        {
            NodeCommandState state = other as NodeCommandState;
            if (state == null)
            {
                return false;
            }

            if (!base.VerifyEquals(other))
            {
                return false;
            }

            return this.Info.VerifyEquals(state.Info);
        }

        internal override byte[] ToBytes()
        {
            using (MemoryStream mem = new MemoryStream())
            {
                using (BinaryWriter bw = new BinaryWriter(mem))
                {
                    this.Write(bw);
                    return mem.ToArray();
                }
            }
        }

        internal override void Write(BinaryWriter bw)
        {
            bw.Write((int)this.ActionType);

            base.Write(bw);

            this.Info.Write(bw);
        }

        internal override void Read(BinaryReader br)
        {
            base.Read(br);

            this.Info.Read(br);

            return;
        }
    }
}