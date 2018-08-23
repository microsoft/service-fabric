// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System;
    using System.IO;

    /// <summary>
    /// This class adds test parameters which are relevant to specific part of RandomSession -- NodeFaultActionsManager.
    /// This class exposed configurations/parameters relevant for generating random actions(add, remove, restart) for nodes in the cluster.
    /// </summary>
    [Serializable]
    internal class NodeFaultActionGeneratorParameters : ByteSerializable
    {
        // Default values for ring dynamism
        public const double MinLiveNodesRatioDefault = 0.71;
        public const double MinLiveSeedNodesRatioDefault = 0.51;
        public const double RestartNodeFaultWeightDefault = 50;
        public const double StartStopNodeFaultWeightDefault = 50;

        public NodeFaultActionGeneratorParameters()
        {
            this.MinLiveNodesRatio = MinLiveNodesRatioDefault;
            this.MinLiveSeedNodesRatio = MinLiveSeedNodesRatioDefault;
            this.RestartNodeFaultWeight = RestartNodeFaultWeightDefault;
            this.StartStopNodeFaultWeight = StartStopNodeFaultWeightDefault;
        }

        /// <summary>
        /// Creation of node-faults must maintain live/Up nodes's ratio higher than this value.
        /// This is a constraint while creating NodeFault actions.
        /// </summary>
        public double MinLiveNodesRatio { get; set; }

        /// <summary>
        /// Creation of node-faults must maintain live/Up seed-nodes's ratio higher than this value.
        /// This is a constraint while creating NodeFault actions.
        /// </summary>
        public double MinLiveSeedNodesRatio { get; set; }

        /// <summary>
        /// Action generator will create RestartNodeAction rather than StopNodeAction with this much probability.
        /// </summary>
        public double RestartNodeFaultWeight { get; set; }

        /// <summary>
        /// Action generator will create RestartNodeAction rather than StopNodeAction with this much probability.
        /// </summary>
        public double StartStopNodeFaultWeight { get; set; }

        /// <summary>
        /// This method will do a basic validation of the testParameters.
        /// </summary>
        /// <param name="errorMessage">Help message/hint if some parameters are not valid.</param>
        /// <returns>True iff all parameters in this class are valid.</returns>
        public bool ValidateParameters(out string errorMessage)
        {
            errorMessage = string.Empty;
            return true;
        }

        public override void Read(BinaryReader br)
        {
            this.MinLiveNodesRatio = br.ReadDouble();
            this.MinLiveSeedNodesRatio = br.ReadDouble();
            this.RestartNodeFaultWeight = br.ReadDouble();
            this.StartStopNodeFaultWeight = br.ReadDouble();
        }

        public override void Write(BinaryWriter bw)
        {
            bw.Write(this.MinLiveNodesRatio);
            bw.Write(this.MinLiveSeedNodesRatio);
            bw.Write(this.RestartNodeFaultWeight);
            bw.Write(this.StartStopNodeFaultWeight);
        }

        public override string ToString()
        {
            return string.Format("MinLiveNodesRatio: {0}, MinLiveSeedNodesRatio: {1}, RestartNodeFaultWeight: {2}, StartStopNodeFaultWeight: {3}",
                this.MinLiveNodesRatio,
                this.MinLiveSeedNodesRatio,
                this.RestartNodeFaultWeight,
                this.StartStopNodeFaultWeight);
        }
    }
}