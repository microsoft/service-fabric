// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Engine
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.FaultAnalysis.Service.Actions;
    using System.Fabric.FaultAnalysis.Service.Common;

    internal class NodeCommandSynchronizer
    {
        private readonly List<string> nodeCommandInProgress;

        public NodeCommandSynchronizer()
        {
            this.NodeCommandLock = new object();
            this.nodeCommandInProgress = new List<string>();
        }

        public object NodeCommandLock
        {
            get;
            private set;
        }

        public void Add(string nodeName)
        {
            TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "Enter Add '{0}'", nodeName);
            lock (this.NodeCommandLock)
            {
                if (this.nodeCommandInProgress.Contains(nodeName))
                {
                    throw FaultAnalysisServiceUtility.CreateException(StepBase.TraceType, Interop.NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NODE_TRANSITION_IN_PROGRESS, Strings.StringResources.Error_NodeTransitionInProgress);
                }

                this.nodeCommandInProgress.Add(nodeName);
                TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "Done - Add '{0}'", nodeName);
            }
        }

        public void Remove(string nodeName)
        {
            TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "Enter Remove '{0}'", nodeName);
            lock (this.NodeCommandLock)
            {
                if (!this.nodeCommandInProgress.Contains(nodeName))
                {
                    ReleaseAssert.Failfast("Tried to remove node '{0}', but it is not present", nodeName);
                }

                this.nodeCommandInProgress.Remove(nodeName);
                TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "Done - Remove '{0}'", nodeName);
            }
        }
    }
}