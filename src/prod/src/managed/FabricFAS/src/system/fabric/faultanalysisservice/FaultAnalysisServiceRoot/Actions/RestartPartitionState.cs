// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Actions
{
    using System.Fabric.FaultAnalysis.Service.Test;
    using IO;

    internal class RestartPartitionState : ActionStateBase
    {
        // For deserialization only
        public RestartPartitionState()
            : base(Guid.Empty, ActionType.RestartPartition, null)  // Default values that will be overwritten
        {
            // Default values that will be overwritten
            this.Info = new RestartPartitionInfo(null, RestartPartitionMode.Invalid);
        }

        public RestartPartitionState(Guid operationId, ServiceInternalFaultInfo serviceInternalFaultInfo, PartitionSelector partitionSelector, RestartPartitionMode restartPartitionMode)
                : base(operationId, ActionType.RestartPartition, serviceInternalFaultInfo)
        {
            this.Info = new RestartPartitionInfo(partitionSelector, restartPartitionMode);
        }

        public RestartPartitionInfo Info
        {
            get;
            private set;
        }

        public override void ClearInfo()
        {
            PartitionSelector ps = this.Info.PartitionSelector;
            RestartPartitionMode mode = this.Info.RestartPartitionMode;
            this.Info = new RestartPartitionInfo(ps, mode);
        }

        public override string ToString()
        {
            string s = System.Fabric.Common.StringHelper.Format(
                "Info: {0}",
                this.Info.ToString());

            return s;
        }

        internal static RestartPartitionState FromBytes(BinaryReader br)
        {
            RestartPartitionState state = new RestartPartitionState();

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
            bw.Write((int)ActionType.RestartPartition);

            base.Write(bw);

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
            RestartPartitionState state = other as RestartPartitionState;
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