// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.IO;

    /// <summary>
    /// This class adds test parameters which are relevant to specific part of RandomSession -- NodeFaultActionsManager.
    /// This class exposed configurations/parameters relevant for generating random actions(add, remove, restart) for nodes in the cluster.
    /// </summary>
    [Serializable]
    internal class ServiceFaultActionGeneratorParameters : ByteSerializable
    {
        public const double RestartReplicaFaultWeightDefault = 50;
        public const double RemoveReplicaFaultWeightDefault = 50;
        public const double RestartCodePackageFaultWeightDefault = 50;
        public const double MovePrimaryFaultWeightDefault = 50;
        public const double MoveSecondaryFaultWeightDefault = 50;

        public ServiceFaultActionGeneratorParameters()
        {
            this.RestartReplicaFaultWeight = RestartReplicaFaultWeightDefault;
            this.RemoveReplicaFaultWeight = RemoveReplicaFaultWeightDefault;
            this.RestartCodePackageFaultWeight = RestartCodePackageFaultWeightDefault;
            this.MovePrimaryFaultWeight = MovePrimaryFaultWeightDefault;
            this.MoveSecondaryFaultWeight = MoveSecondaryFaultWeightDefault;
        }

        public double RestartReplicaFaultWeight { get; set; }

        public double RemoveReplicaFaultWeight { get; set; }

        public double RestartCodePackageFaultWeight { get; set; }

        public double MovePrimaryFaultWeight { get; set; }

        public double MoveSecondaryFaultWeight { get; set; }

        /// <summary>
        /// This method will do a basic validation of the testParameters.
        /// </summary>
        /// <param name="errorMessage">Help message/hint if some parameters are not valid.</param>
        /// <returns>True iff all parameters in this class are valid.</returns>
        public bool ValidateParameters(out string errorMessage)
        {
            bool isValid = true;
            errorMessage = string.Empty;

            return isValid;
        }

        public override void Read(BinaryReader br)
        {
            this.RestartReplicaFaultWeight = br.ReadDouble();
            this.RemoveReplicaFaultWeight = br.ReadDouble();
            this.RestartCodePackageFaultWeight = br.ReadDouble();
            this.MovePrimaryFaultWeight = br.ReadDouble();
            this.MoveSecondaryFaultWeight = br.ReadDouble();
        }

        public override void Write(BinaryWriter bw)
        {
            bw.Write(this.RestartReplicaFaultWeight);
            bw.Write(this.RemoveReplicaFaultWeight);
            bw.Write(this.RestartCodePackageFaultWeight);
            bw.Write(this.MovePrimaryFaultWeight);
            bw.Write(this.MoveSecondaryFaultWeight);
        }

        public override string ToString()
        {
            return string.Format("RestartReplicaWeight: {0}, RemoveReplicaWeight: {1}, RestartCodePackageFaultWeight: {2}, MovePrimaryFaultWeight: {3}, MoveSecondaryFaultWeight: {4}",
                this.RestartReplicaFaultWeight,
                this.RemoveReplicaFaultWeight,
                this.RestartCodePackageFaultWeight,
                this.MovePrimaryFaultWeight,
                this.MoveSecondaryFaultWeight);
        }
    }
}