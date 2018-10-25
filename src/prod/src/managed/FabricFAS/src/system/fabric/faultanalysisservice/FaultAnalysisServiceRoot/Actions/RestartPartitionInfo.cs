// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Actions
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using IO;

    internal class RestartPartitionInfo
    {
        private const string TraceType = "RestartPartitionInfo";

        public RestartPartitionInfo(PartitionSelector partitionSelector, RestartPartitionMode restartPartitionMode)
        {
            this.PartitionSelector = partitionSelector;
            this.RestartPartitionMode = restartPartitionMode;

            // This is a default value and will be overwritten when the command executes.  The default value is not used during the command.
            this.NodeName = "UNKNOWNNODE";
            this.UnreliableTransportInfo = new List<Tuple<string, string>>();
        }

        public PartitionSelector PartitionSelector
        {
            get;
            private set;
        }

        public RestartPartitionMode RestartPartitionMode
        {
            get;
            private set;
        }

        public Guid PartitionId
        {
            get;
            set;
        }

        public string NodeName
        {
            get;
            set;
        }

        public bool HasPersistedState
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

            this.RestartPartitionMode = (RestartPartitionMode)br.ReadInt32();
            this.PartitionId = new Guid(br.ReadBytes(16));

            this.NodeName = br.ReadString();
            this.HasPersistedState = br.ReadBoolean();

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
                "PartitionSelector: {0} RestartPartitionMode: {1} PartitionId: {2} NodeName: {3} HasPersistedState: {4}",
                this.PartitionId.ToString(),
                this.RestartPartitionMode,
                this.PartitionId,
                this.NodeName,
                this.HasPersistedState);

            foreach (var ut in this.UnreliableTransportInfo)
            {
                s += System.Fabric.Common.StringHelper.Format(" {0}:{1} ", ut.Item1, ut.Item2);
            }

            return s;
        }

        internal void Write(BinaryWriter bw)
        {
            this.PartitionSelector.Write(bw);

            bw.Write((int)this.RestartPartitionMode);
            bw.Write(this.PartitionId.ToByteArray());

            bw.Write(this.NodeName);
            bw.Write(this.HasPersistedState);

            // UnreliableTransportInfo
            bw.Write(this.UnreliableTransportInfo.Count);
            foreach (Tuple<string, string> ut in this.UnreliableTransportInfo)
            {
                bw.Write(ut.Item1);
                bw.Write(ut.Item2);
            }
        }

        // Test only
        internal bool VerifyEquals(RestartPartitionInfo other)
        {
            bool eq = this.PartitionSelector.Equals(other.PartitionSelector);
            if (!eq)
            {
                TestabilityTrace.TraceSource.WriteError(TraceType, "PartitionSelector does not match this:{0} other:{1}", this.PartitionSelector.ToString(), other.PartitionSelector.ToString());
            }

            eq = this.RestartPartitionMode.Equals(other.RestartPartitionMode) &&
                this.PartitionId.Equals(other.PartitionId) &&
                this.NodeName.Equals(other.NodeName) &&
                (this.HasPersistedState == other.HasPersistedState);
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
    }
}