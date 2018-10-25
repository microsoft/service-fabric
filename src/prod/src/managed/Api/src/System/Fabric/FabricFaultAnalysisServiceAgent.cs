// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1201:ElementsMustAppearInTheCorrectOrder", Justification = "Preserve order of public members from V1.")]
    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1204:StaticElementsMustAppearBeforeInstanceElements", Justification = "Current grouping improves readability.")]
    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1202:ElementsMustBeOrderedByAccess", Justification = "Current grouping improves readability.")]
    internal sealed class FabricFaultAnalysisServiceAgent
    {
        #region Fields & Properties

        internal static readonly TimeSpan DefaultTimeout = TimeSpan.FromMinutes(1);

        // This should only be constructed once per process, so making it static.  Without this, there is a situation where it could be constructed more than once.  For example, if 
        // the primary moves somewhere else then comes back to its former position without a host deactivation.
        private static readonly Lazy<FabricFaultAnalysisServiceAgent> agent = new Lazy<FabricFaultAnalysisServiceAgent>(() => new FabricFaultAnalysisServiceAgent());

        private NativeFaultAnalysisService.IFabricFaultAnalysisServiceAgent nativeAgent;

        #endregion

        #region Constructors & Initialization

        public static FabricFaultAnalysisServiceAgent Instance
        {
            get { return agent.Value; }
        }

        private FabricFaultAnalysisServiceAgent()
        {
            Utility.WrapNativeSyncInvokeInMTA(this.CreateNativeAgent, "FaultAnalysisServiceAgent.CreateNativeAgent");
        }

        private void CreateNativeAgent()
        {
            Guid iid = typeof(NativeFaultAnalysisService.IFabricFaultAnalysisServiceAgent).GUID;

            //// PInvoke call
            this.nativeAgent = NativeFaultAnalysisService.CreateFabricFaultAnalysisServiceAgent(ref iid);
        }

        #endregion

        #region API

        public string RegisterFaultAnalysisService(Guid partitionId, long replicaId, IFaultAnalysisService service)
        {
            Requires.Argument<Guid>("partitionId", partitionId).NotNull();

            return Utility.WrapNativeSyncInvokeInMTA(() => this.RegisterFaultAnalysisServiceHelper(partitionId, replicaId, service), "FaultAnalysisServiceAgent.RegisterFaultAnalysisService");
        }

        public void UnregisterFaultAnalysisService(Guid partitionId, long replicaId)
        {
            Requires.Argument<Guid>("partitionId", partitionId).NotNull();

            Utility.WrapNativeSyncInvokeInMTA(() => this.UnregisterFaultAnalysisServiceHelper(partitionId, replicaId), "FaultAnalysisServiceAgent.UnregisterFaultAnalysisService");
        }

        #endregion

        #region Register/Unregister FaultAnalysis Service

        private string RegisterFaultAnalysisServiceHelper(Guid partitionId, long replicaId, IFaultAnalysisService service)
        {
            FaultAnalysisServiceBroker broker = new FaultAnalysisServiceBroker(service);

            NativeCommon.IFabricStringResult nativeString = this.nativeAgent.RegisterFaultAnalysisService(
                partitionId,
                replicaId,
                broker);

            return StringResult.FromNative(nativeString);
        }

        private void UnregisterFaultAnalysisServiceHelper(Guid partitionId, long replicaId)
        {
            this.nativeAgent.UnregisterFaultAnalysisService(
                partitionId,
                replicaId);
        }

        #endregion
    }
}