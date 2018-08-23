// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Actions
{
    using System.Fabric.FaultAnalysis.Service.Test;
    using IO;

    internal class InvokeQuorumLossState : ActionStateBase
    {
        // For deserialization only
        public InvokeQuorumLossState()
            : base(Guid.Empty, ActionType.InvokeQuorumLoss, null)  // Default values that will be overwritten
        {
            // Default values that will be overwritten
            this.Info = new InvokeQuorumLossInfo(null, QuorumLossMode.Invalid, TimeSpan.Zero);
        }

        public InvokeQuorumLossState(Guid operationId, ServiceInternalFaultInfo serviceInternalFaultInfo, PartitionSelector partitionSelector, QuorumLossMode quorumLossMode, TimeSpan quorumLossDuration)
                : base(operationId, ActionType.InvokeQuorumLoss, serviceInternalFaultInfo)
        {
            this.Info = new InvokeQuorumLossInfo(partitionSelector, quorumLossMode, quorumLossDuration);
        }

        public InvokeQuorumLossInfo Info
        {
            get;
            private set;
        }

        public override void ClearInfo()
        {
            PartitionSelector ps = this.Info.PartitionSelector;
            QuorumLossMode mode = this.Info.QuorumLossMode;
            TimeSpan duration = this.Info.QuorumLossDuration;
            this.Info = new InvokeQuorumLossInfo(ps, mode, duration);
        }

        public override string ToString()
        {
            string s = System.Fabric.Common.StringHelper.Format(
                "Info: {0}",
                this.Info.ToString());

            return s;
        }

        internal static InvokeQuorumLossState FromBytes(BinaryReader br)
        {
            InvokeQuorumLossState state = new InvokeQuorumLossState();

            // Note: the type of command (an Enum serialized as Int32) was already read by caller
            state.Read(br);

            return state;
        }

        // test only
        internal override bool VerifyEquals(ActionStateBase other)
        {
            InvokeQuorumLossState state = other as InvokeQuorumLossState;
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
            bw.Write((int)ActionType.InvokeQuorumLoss);

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