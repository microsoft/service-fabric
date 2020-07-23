// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeOrchestration.Service
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;

    internal class UpgradeOrchestrationServiceAgent
    {
        // This should only be constructed once per process, so making it static.  Without this, there is a situation where it could be constructed more than once.  For example, if
        // the primary moves somewhere else then comes back to its former position without a host deactivation.
        private static readonly Lazy<UpgradeOrchestrationServiceAgent> Agent = new Lazy<UpgradeOrchestrationServiceAgent>(() => new UpgradeOrchestrationServiceAgent());

        private NativeUpgradeOrchestrationService.IFabricUpgradeOrchestrationServiceAgent nativeAgent;

        private UpgradeOrchestrationServiceAgent()
        {
            Utility.WrapNativeSyncInvokeInMTA(this.CreateNativeAgent, "UpgradeOrchestrationServiceServiceAgent.CreateNativeAgent");
        }

        public static UpgradeOrchestrationServiceAgent Instance
        {
            get
            {
                return Agent.Value;
            }
        }

        public string RegisterService(Guid partitionId, long replicaId, IUpgradeOrchestrationService service)
        {
            Requires.Argument<Guid>("partitionId", partitionId).NotNull();
            return Utility.WrapNativeSyncInvokeInMTA(() => this.RegisterServiceHelper(partitionId, replicaId, service), "UpgradeOrchestrationServiceServiceAgent.RegisterUpgradeOrchestrationService");
        }

        public void UnregisterService(Guid partitionId, long replicaId)
        {
            Requires.Argument<Guid>("partitionId", partitionId).NotNull();
            Utility.WrapNativeSyncInvokeInMTA(() => this.UnregisterServiceHelper(partitionId, replicaId), "UpgradeOrchestrationServiceServiceAgent.UnregisterUpgradeOrchestrationService");
        }

        #region Register Service Helper

        private string RegisterServiceHelper(Guid partitionId, long replicaId, IUpgradeOrchestrationService service)
        {
            var broker = new UpgradeOrchestrationServiceBroker(service);
            NativeCommon.IFabricStringResult nativeString = this.nativeAgent.RegisterUpgradeOrchestrationService(partitionId, replicaId, broker);
            return StringResult.FromNative(nativeString);
        }

        private void UnregisterServiceHelper(Guid partitionId, long replicaId)
        {
            this.nativeAgent.UnregisterUpgradeOrchestrationService(partitionId, replicaId);
        }

        private void CreateNativeAgent()
        {
            Guid iid = typeof(NativeUpgradeOrchestrationService.IFabricUpgradeOrchestrationServiceAgent).GUID;

            //// PInvoke call
            this.nativeAgent = NativeUpgradeOrchestrationService.CreateFabricUpgradeOrchestrationServiceAgent(ref iid);
        }

        #endregion
    }
}