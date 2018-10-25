// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Actions
{
    using System.Fabric.FaultAnalysis.Service.Test;
    using IO;

    internal class InvokeDataLossState : ActionStateBase
    {
        // For deserialization only
        public InvokeDataLossState()
            : base(Guid.Empty, ActionType.InvokeDataLoss, null)  // Default values that will be overwritten
        {
            // Default values that will be overwritten
            this.Info = new InvokeDataLossInfo(null, DataLossMode.Invalid);
        }

        public InvokeDataLossState(Guid operationId, ServiceInternalFaultInfo serviceInternalFaultInfo, PartitionSelector partitionSelector, DataLossMode dataLossMode)
                : base(operationId, ActionType.InvokeDataLoss, serviceInternalFaultInfo)
        {
            this.Info = new InvokeDataLossInfo(partitionSelector, dataLossMode);
        }

        public InvokeDataLossInfo Info
        {
            get;
            private set;
        }

        public override void ClearInfo()
        {
            PartitionSelector ps = this.Info.PartitionSelector;
            DataLossMode dlm = this.Info.DataLossMode;
            this.Info = new InvokeDataLossInfo(ps, dlm);
        }

        public override string ToString()
        {
            string s = System.Fabric.Common.StringHelper.Format(
                "Info: {0}",
                this.Info.ToString());

            return s;
        }

        internal static InvokeDataLossState FromBytes(BinaryReader br)
        {
            InvokeDataLossState state = new InvokeDataLossState();

            // Note: the type of command (an Enum serialized as Int32) was already read by caller
            state.Read(br);

            return state;
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
            // Write the identifier first
            bw.Write((int)ActionType.InvokeDataLoss);

            // Write the "common things", which are in the base class
            base.Write(bw);

            // Write the items specific to this type of command
            this.Info.Write(bw);
        }

        internal override void Read(BinaryReader br)
        {
            base.Read(br);

            this.Info.Read(br);

            return;
        }

        // test only
        internal override bool VerifyEquals(ActionStateBase other)
        {
            InvokeDataLossState state = other as InvokeDataLossState;
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
    }
}