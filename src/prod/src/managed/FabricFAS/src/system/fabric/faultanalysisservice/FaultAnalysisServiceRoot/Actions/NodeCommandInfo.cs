// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Actions
{
    using System.Fabric.Common;
    using System.Numerics;
    using IO;
    using Query;

    internal class NodeCommandInfo
    {
        public NodeCommandInfo(string nodeName, BigInteger nodeInstanceId, int stopDurationInSeconds)
        {
            this.Version = 1;
            this.NodeName = nodeName;
            this.InputNodeInstanceId = nodeInstanceId;
            this.StopDurationInSeconds = stopDurationInSeconds;
        }

        public byte Version
        {
            get;
            private set;
        }

        public string NodeName
        {
            get;
            private set;
        }

        // The instance id passed into the api by the user
        public BigInteger InputNodeInstanceId
        {
            get;
            private set;
        }

        public NodeStatus InitialQueriedNodeStatus
        {
            get;
            set;
        }

        public bool NodeWasInitiallyInStoppedState
        {
            get;
            set;
        }

        public int StopDurationInSeconds
        {
            get;
            private set;
        }

        public void Read(BinaryReader br)
        {
            this.Version = br.ReadByte();

            this.NodeName = br.ReadString();

            int numNodeInstanceIdBytes = br.ReadInt32();
            byte[] nodeInstanceIdBytes = br.ReadBytes(numNodeInstanceIdBytes);
            this.InputNodeInstanceId = new BigInteger(nodeInstanceIdBytes);

            this.InitialQueriedNodeStatus = (NodeStatus)br.ReadInt32();
            this.NodeWasInitiallyInStoppedState = br.ReadBoolean();
            this.StopDurationInSeconds = br.ReadInt32();
        }

        public override string ToString()
        {
            string s = System.Fabric.Common.StringHelper.Format(
                "NodeName: {0} ApiInputNodeInstanceId: {1} InitialQueriedNodeStatus: {2} NodeWasInitiallyInStoppedState: {3}",
                this.NodeName,
                this.InputNodeInstanceId,
                this.InitialQueriedNodeStatus,
                this.NodeWasInitiallyInStoppedState);

            return s;
        }

        // test only
        internal bool VerifyEquals(NodeCommandInfo other)
        {
            TestabilityTrace.TraceSource.WriteInfo("NodeCommandInfo", "Enter VerifyEquals in StartNodeInfo");
            bool eq = this.Version.Equals(other.Version);
            if (!eq)
            {
                TestabilityTrace.TraceSource.WriteError("NodeCommandInfo", "Version does not match this:{0} other:{1}", this.Version, other.Version);
            }

            TestabilityTrace.TraceSource.WriteInfo("NodeCommandInfo", "Enter VerifyEquals in StartNodeInfo");
            eq = this.NodeName.Equals(other.NodeName);
            if (!eq)
            {
                TestabilityTrace.TraceSource.WriteError("NodeCommandInfo", "NodeName does not match this:{0} other:{1}", this.NodeName, other.NodeName);
            }

            eq = this.InputNodeInstanceId.Equals(other.InputNodeInstanceId);
            if (!eq)
            {
                TestabilityTrace.TraceSource.WriteError("NodeCommandInfo", "ApiInputNodeInstanceId does not match this:{0} other:{1}", this.InputNodeInstanceId, other.InputNodeInstanceId);
                return false;
            }

            eq = this.InitialQueriedNodeStatus.Equals(other.InitialQueriedNodeStatus);
            if (!eq)
            {
                TestabilityTrace.TraceSource.WriteError("NodeCommandInfo", "InitialQueriedNodeStatus does not match this:{0} other:{1}", this.InitialQueriedNodeStatus, other.InitialQueriedNodeStatus);
                return false;
            }

            eq = this.NodeWasInitiallyInStoppedState.Equals(other.NodeWasInitiallyInStoppedState);
            if (!eq)
            {
                TestabilityTrace.TraceSource.WriteError("NodeCommandInfo", "NodeWasInitiallyInStoppedState does not match this:{0} other:{1}", this.NodeWasInitiallyInStoppedState, other.NodeWasInitiallyInStoppedState);
                return false;
            }

            eq = this.StopDurationInSeconds.Equals(other.StopDurationInSeconds);
            if (!eq)
            {
                TestabilityTrace.TraceSource.WriteError("NodeCommandInfo", "StopDurationInSeconds does not match this:{0} other:{1}", this.StopDurationInSeconds, other.StopDurationInSeconds);
                return false;
            }

            return true;
        }

        internal void Write(BinaryWriter bw)
        {
            bw.Write(this.Version);

            bw.Write(this.NodeName);

            byte[] nodeInstanceIdBytes = this.InputNodeInstanceId.ToByteArray();
            bw.Write(nodeInstanceIdBytes.Length);
            bw.Write(nodeInstanceIdBytes);

            bw.Write((int)this.InitialQueriedNodeStatus);
            bw.Write(this.NodeWasInitiallyInStoppedState);
            bw.Write(this.StopDurationInSeconds);
        }
    }
}