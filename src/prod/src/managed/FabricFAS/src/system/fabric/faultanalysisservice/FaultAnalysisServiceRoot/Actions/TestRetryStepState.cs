// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Actions
{
    using System.Fabric.FaultAnalysis.Service.Test;
    using System.IO;

    internal class TestRetryStepState : ActionStateBase
    {
        // For deserialization only
        public TestRetryStepState()
            : base(Guid.Empty, ActionType.TestRetryStep, null)  // Default values that will be overwritten
        {
            this.RetryStepWithoutRollingBackOnFailure = true;            
        }

        public TestRetryStepState(Guid operationId, ServiceInternalFaultInfo serviceInternalFaultInfo)
            : base(operationId, ActionType.TestRetryStep, serviceInternalFaultInfo)
        {            
            this.RetryStepWithoutRollingBackOnFailure = true;            
        }

        internal static TestRetryStepState FromBytes(BinaryReader br)
        {
            TestRetryStepState state = new TestRetryStepState();

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
            bw.Write((int)ActionType.TestRetryStep);

            base.Write(bw);
        }

        internal override void Read(BinaryReader br)
        {
            base.Read(br);

            return;
        }        
    } 
}  