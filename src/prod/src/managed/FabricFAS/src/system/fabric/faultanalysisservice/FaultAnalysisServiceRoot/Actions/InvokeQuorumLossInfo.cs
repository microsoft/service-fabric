// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Actions
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using IO;

    internal class InvokeQuorumLossInfo
    {
        public InvokeQuorumLossInfo(PartitionSelector partitionSelector, QuorumLossMode quorumLossMode, TimeSpan quorumLossDuration)
        {
            this.PartitionSelector = partitionSelector;
            this.QuorumLossMode = quorumLossMode;
            this.QuorumLossDuration = quorumLossDuration;

            this.NodesApplied = new List<string>();
            this.ReplicaIds = new List<long>();

            this.UnreliableTransportInfo = new List<Tuple<string, string>>();
        }

        public PartitionSelector PartitionSelector
        {
            get;
            private set;
        }

        public QuorumLossMode QuorumLossMode
        {
            get;
            private set;
        }
        
        public TimeSpan QuorumLossDuration
        {
            get;
            private set;
        }

        public Guid PartitionId
        {
            get;
            set;
        }

        public List<string> NodesApplied
        {
            get;
            set;
        }
        
        public List<long> ReplicaIds
        {
            get;
            set;
        }
        
        public List<Tuple<string, string>> UnreliableTransportInfo
        {
            get;
            set;
        }

        public void Read(BinaryReader br)
        {         
            this.PartitionSelector = PartitionSelector.Read(br);

            this.QuorumLossMode = (QuorumLossMode)br.ReadInt32();
            this.QuorumLossDuration = TimeSpan.FromSeconds(br.ReadDouble());

            this.PartitionId = new Guid(br.ReadBytes(16));

            int nodesAppliedCount = br.ReadInt32();
            this.NodesApplied = new List<string>();
            for (int i = 0; i < nodesAppliedCount; i++)
            {
                string n = br.ReadString();
                this.NodesApplied.Add(n);
            }

            int replicaIdCount = br.ReadInt32();
            this.ReplicaIds = new List<long>();
            for (int i = 0; i < replicaIdCount; i++)
            {
                long r = br.ReadInt64();
                this.ReplicaIds.Add(r);
            }

            int unreliableTransportCount = br.ReadInt32();
            this.UnreliableTransportInfo = new List<Tuple<string, string>>();
            for (int i = 0; i < unreliableTransportCount; i++)
            {
                string k = br.ReadString();
                string v = br.ReadString();
                this.UnreliableTransportInfo.Add(new Tuple<string, string>(k, v));
            }
        }

        public override string ToString()
        {
            string s = System.Fabric.Common.StringHelper.Format(
                "PartitionSelector: {0} QuorumLossMode: {1} QuorumLossDuration: {2} PartitionId: {3}",
                this.PartitionId.ToString(),
                this.QuorumLossMode,
                this.QuorumLossDuration,
                this.PartitionId);

            foreach (var ut in this.UnreliableTransportInfo)
            {
                s += System.Fabric.Common.StringHelper.Format(" {0}:{1} ", ut.Item1, ut.Item2);
            }

            return s;
        }

        // test only
        internal bool VerifyEquals(InvokeQuorumLossInfo other)
        {
            TestabilityTrace.TraceSource.WriteInfo("InvokeQuorumLossInfo", "Enter VerifyEquals in InvokeQuorumLossInfo");
            bool eq = this.PartitionSelector.Equals(other.PartitionSelector);
            if (!eq)
            {
                TestabilityTrace.TraceSource.WriteError("InvokeQuorumLossInfo", "PartitionSelector does not match this:{0} other:{1}", this.PartitionSelector.ToString(), other.PartitionSelector.ToString());
            }

            eq = this.QuorumLossMode.Equals(other.QuorumLossMode) &&
                (this.PartitionId == other.PartitionId);
            if (!eq)
            {
                TestabilityTrace.TraceSource.WriteError("InvokeQuorumLossInfo", "QuorumLossMode or PartitionId do not match");
                return false;
            }

            if (this.NodesApplied.Count != other.NodesApplied.Count)
            {
                TestabilityTrace.TraceSource.WriteError("InvokeQuorumLossInfo", "Size of NodesApplied does not match {0} vs {1}", this.NodesApplied.Count, other.NodesApplied.Count);
                return false;
            }

            for (int i = 0; i < this.NodesApplied.Count; i++)
            {
                if (this.NodesApplied[i] != other.NodesApplied[i])
                {
                    TestabilityTrace.TraceSource.WriteError("InvokeQuorumLossInfo", "NodesApplied was expected to match: '{0}' vs '{1}'", this.NodesApplied[i], other.NodesApplied[i]);
                    return false;
                }
            }

            if (this.ReplicaIds.Count != other.ReplicaIds.Count)
            {
                TestabilityTrace.TraceSource.WriteError("InvokeQuorumLossInfo", "Size of ReplicaIds does not match {0} vs {1}", this.ReplicaIds.Count, other.ReplicaIds.Count);
                return false;
            }

            for (int i = 0; i < this.ReplicaIds.Count; i++)
            {
                if (this.ReplicaIds[i] != other.ReplicaIds[i])
                {
                    TestabilityTrace.TraceSource.WriteError("InvokeQuorumLossInfo", "ReplicaIds was expected to match: '{0}' vs '{1}'", this.ReplicaIds[i], other.ReplicaIds[i]);
                    return false;
                }
            }

            if (this.UnreliableTransportInfo.Count != other.UnreliableTransportInfo.Count)
            {
                TestabilityTrace.TraceSource.WriteError("InvokeQuorumLossInfo", "Size of UnreliableTransportInfo does not match {0} vs {1}", this.UnreliableTransportInfo.Count, other.UnreliableTransportInfo.Count);
                return false;
            }

            for (int i = 0; i < this.UnreliableTransportInfo.Count; i++)
            {
                if ((this.UnreliableTransportInfo[i].Item1 != other.UnreliableTransportInfo[i].Item1) &&
                    (this.UnreliableTransportInfo[i].Item2 != other.UnreliableTransportInfo[i].Item2))
                {
                    TestabilityTrace.TraceSource.WriteError("InvokeQuorumLossInfo", "UT behavior was expected to match: '{0}', '{1}'", this.UnreliableTransportInfo[i], other.UnreliableTransportInfo[i]);
                    return false;
                }
            }

            return true;
        }

        internal void Write(BinaryWriter bw)
        {
            this.PartitionSelector.Write(bw);

            bw.Write((int)this.QuorumLossMode);
            bw.Write(this.QuorumLossDuration.TotalSeconds);

            bw.Write(this.PartitionId.ToByteArray());

            // NodesApplied
            bw.Write(this.NodesApplied.Count);
            foreach (string node in this.NodesApplied)
            {
                bw.Write(node);
            }

            // ReplicaIds
            bw.Write(this.ReplicaIds.Count);
            foreach (long r in this.ReplicaIds)
            {
                bw.Write(r);
            }

            // UnreliableTransportInfo
            bw.Write(this.UnreliableTransportInfo.Count);
            foreach (Tuple<string, string> ut in this.UnreliableTransportInfo)
            {
                bw.Write(ut.Item1);
                bw.Write(ut.Item2);
            }
        }
    }
}