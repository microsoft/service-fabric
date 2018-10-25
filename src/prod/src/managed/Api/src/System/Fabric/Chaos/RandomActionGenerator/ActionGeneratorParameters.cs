// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System;
    using System.Fabric.Common;
    using System.IO;

    /// <summary>
    /// This class adds new test parameters which are relevant to RandomSession.
    /// </summary>
    [Serializable]
    internal class ActionGeneratorParameters : ByteSerializable
    {
        public const double NodeFaultActionWeightDefault = 100;
        public const double ServiceFaultActionWeightDefault = 100;
        public const double WorkloadActionWeightDefault = 10;

        /// <summary>
        /// Default weight to control the probability with which a system fault will be chosen
        /// </summary>
        public const double SystemFaultActionWeightDefault = 0;

        public ActionGeneratorParameters()
        {
            this.NodeFaultActionWeight = NodeFaultActionWeightDefault;
            this.ServiceFaultActionWeight = ServiceFaultActionWeightDefault;
            this.WorkloadActionWeight = WorkloadActionWeightDefault;
            this.SystemFaultActionWeight = SystemFaultActionWeightDefault;

            // Add other parameters
            this.NodeFaultActionsParameters = new NodeFaultActionGeneratorParameters();
            this.ServiceFaultActionsParameters = new ServiceFaultActionGeneratorParameters();
            this.WorkloadParameters = new WorkloadActionGeneratorParameters();
            this.SystemServiceFaultParameters = new SystemFaultActionGeneratorParameters();
        }
        
        public long MaxConcurrentFaults { get; set; }

        public double NodeFaultActionWeight { get; set; }

        public double ServiceFaultActionWeight { get; set; }

        public double WorkloadActionWeight { get; set; }

        /// <summary>
        /// Weight to control the probability with which a system fault is chosen
        /// </summary>
        public double SystemFaultActionWeight { get; set; }

        public NodeFaultActionGeneratorParameters NodeFaultActionsParameters { get; set; }

        public ServiceFaultActionGeneratorParameters ServiceFaultActionsParameters { get; set; }

        public WorkloadActionGeneratorParameters WorkloadParameters { get; set; }

        /// <summary>
        /// Parameters for system faults, e.g., FM Rebuild
        /// </summary>
        public SystemFaultActionGeneratorParameters SystemServiceFaultParameters { get; set; }

        /// <summary>
        /// This method will do a basic validation of the testParameters.
        /// </summary>
        /// <param name="errorMessage">Help message/hint if some parameters are not valid.</param>
        /// <returns>True iff all parameters in this class are valid.</returns>
        public bool ValidateParameters(out string errorMessage)
        {
            errorMessage = string.Empty;
            string errorMessageTemp;
            bool isValid = true;

            isValid &= this.NodeFaultActionsParameters.ValidateParameters(out errorMessageTemp);
            errorMessage += errorMessageTemp;

            isValid &= this.WorkloadParameters.ValidateParameters(out errorMessageTemp);
            errorMessage += errorMessageTemp;

            return isValid;
        }

        public sealed override void Read(BinaryReader br)
        {
            TestabilityTrace.TraceSource.WriteInfo("###6c 5 1");

            this.MaxConcurrentFaults = br.ReadInt64();
            this.NodeFaultActionWeight = br.ReadDouble();
            this.NodeFaultActionsParameters.Read(br);
            this.ServiceFaultActionWeight = br.ReadDouble();
            this.ServiceFaultActionsParameters.Read(br);

            TestabilityTrace.TraceSource.WriteInfo("###6c 5 2");
        }

        public sealed override void Write(BinaryWriter bw)
        {
            bw.Write(this.MaxConcurrentFaults);
            bw.Write(this.NodeFaultActionWeight);
            this.NodeFaultActionsParameters.Write(bw);
            bw.Write(this.ServiceFaultActionWeight);
            this.ServiceFaultActionsParameters.Write(bw);
        }

        public override string ToString()
        {
            return string.Format(
                @"MaxConcurrentFaults: {0}, 
                  NodeFaultActionWeight: {1}, 
                  NodeFaultParameters: {2}, 
                  ServiceFaultActionWeight: {3}, 
                  ServiceFaultParameters: {4}",
                this.MaxConcurrentFaults,
                this.NodeFaultActionWeight,
                this.NodeFaultActionsParameters.ToString(),
                this.ServiceFaultActionWeight,
                this.ServiceFaultActionsParameters.ToString());
        }
    }
}