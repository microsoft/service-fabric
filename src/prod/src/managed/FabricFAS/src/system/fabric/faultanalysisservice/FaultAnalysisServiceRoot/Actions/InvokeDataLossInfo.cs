// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Actions
{
    using System.Collections.Generic;
    using Fabric.Common;
    using IO;

    internal class InvokeDataLossInfo
    {
        private const string TraceType = "InvokeDataLossInfo";

        public InvokeDataLossInfo(PartitionSelector partitionSelector, DataLossMode dataLossMode)
        {
            this.PartitionSelector = partitionSelector;
            this.DataLossMode = dataLossMode;

            // This is a default value and will be overwritten when the command executes.  The default value is not used during the command.
            this.NodeName = "UNKNOWNNODE";
            this.UnreliableTransportInfo = new List<Tuple<string, string>>();
        }
        
        public PartitionSelector PartitionSelector
        {
            get;
            private set;
        }
        
        public DataLossMode DataLossMode
        {
            get;
            private set;
        }
        
        public long DataLossNumber
        {
            get;
            set;
        }
        
        public string NodeName
        {
            get;
            set;
        }
        
        public Guid PartitionId
        {
            get;
            set;
        }
        
        public List<Tuple<string, string>> UnreliableTransportInfo
        {
            get;
            set;
        }
        
        public int TargetReplicaSetSize
        {
            get;
            set;
        }

        public void Read(BinaryReader br)
        {
            this.PartitionSelector = PartitionSelector.Read(br);
         
            this.DataLossMode = (DataLossMode)br.ReadInt32();
            this.DataLossNumber = br.ReadInt64();
         
            this.NodeName = br.ReadString();
            this.PartitionId = new Guid(br.ReadBytes(16));
         
            int unreliableTransportInfoCount = br.ReadInt32();
            this.UnreliableTransportInfo = new List<Tuple<string, string>>();
            for (int i = 0; i < unreliableTransportInfoCount; i++)
            {
                string k = br.ReadString();
                string v = br.ReadString();
                this.UnreliableTransportInfo.Add(new Tuple<string, string>(k, v));
            }

            this.TargetReplicaSetSize = br.ReadInt32();
        }

        public override string ToString()
        {
            string s = System.Fabric.Common.StringHelper.Format(
                "PartitionSelector: {0} DataLossMode: {1} DataLossNumber: {2} NodeName: {3} PartitionId: {4} TargetReplicaSetSize: {5} ",
                this.PartitionId.ToString(),
                this.DataLossMode,
                this.DataLossNumber,
                this.NodeName,
                this.PartitionId,
                this.TargetReplicaSetSize);

            foreach (var ut in this.UnreliableTransportInfo)
            {
                s += System.Fabric.Common.StringHelper.Format(" {0}:{1} ", ut.Item1, ut.Item2);
            }

            return s;
        }

        // test only
        internal bool VerifyEquals(InvokeDataLossInfo other)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Enter VerifyEquals in InvokeDataLossInfo");
            bool eq = this.PartitionSelector.Equals(other.PartitionSelector);
            if (!eq)
            {
                TestabilityTrace.TraceSource.WriteError(TraceType, "PartitionSelector does not match this:{0} other:{1}", this.PartitionSelector.ToString(), other.PartitionSelector.ToString());
            }

            eq = this.DataLossMode.Equals(other.DataLossMode) &&
                this.DataLossNumber.Equals(other.DataLossNumber) &&
                this.NodeName.Equals(other.NodeName) &&
                (this.PartitionId == other.PartitionId);
            if (!eq)
            {
                TestabilityTrace.TraceSource.WriteError(TraceType, "DataLossMode/Number, NodeName, or PartitionId do not match");
                return false;
            }

            if (this.UnreliableTransportInfo.Count != other.UnreliableTransportInfo.Count)
            {
                TestabilityTrace.TraceSource.WriteError(TraceType, "Size of UnreliableTransportInfo does not match {0} vs {1}", this.UnreliableTransportInfo.Count, other.UnreliableTransportInfo.Count);
                return false;
            }

            for (int i = 0; i < this.UnreliableTransportInfo.Count; i++)
            {
                if ((this.UnreliableTransportInfo[i].Item1 != other.UnreliableTransportInfo[i].Item1) &&
                    (this.UnreliableTransportInfo[i].Item2 != other.UnreliableTransportInfo[i].Item2))
                {
                    TestabilityTrace.TraceSource.WriteError(TraceType, "UT behavior was expected to match: '{0}', '{1}'", this.UnreliableTransportInfo[i], other.UnreliableTransportInfo[i]);
                    return false;
                }
            }

            return true;
        }
   
        internal void Write(BinaryWriter bw)
        {
            this.PartitionSelector.Write(bw);

            bw.Write((int)this.DataLossMode);

            bw.Write(this.DataLossNumber);

            bw.Write(this.NodeName);
            bw.Write(this.PartitionId.ToByteArray());

            // UnreliableTransportInfo
            bw.Write(this.UnreliableTransportInfo.Count);
            foreach (Tuple<string, string> ut in this.UnreliableTransportInfo)
            {
                bw.Write(ut.Item1);
                bw.Write(ut.Item2);
            }

            bw.Write(this.TargetReplicaSetSize);
        }
    }
}