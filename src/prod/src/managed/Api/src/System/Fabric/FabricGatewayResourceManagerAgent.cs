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
    internal sealed class FabricGatewayResourceManagerAgent
    {
        #region Fields & Properties

        internal static readonly TimeSpan DefaultTimeout = TimeSpan.FromMinutes(1);

        // This should only be constructed once per process, so making it static.  Without this, there is a situation where it could be constructed more than once.  For example, if 
        // the primary moves somewhere else then comes back to its former position without a host deactivation.
        private static readonly Lazy<FabricGatewayResourceManagerAgent> agent = new Lazy<FabricGatewayResourceManagerAgent>(() => new FabricGatewayResourceManagerAgent());

        private NativeGatewayResourceManager.IFabricGatewayResourceManagerAgent nativeAgent;

        #endregion

        #region Constructors & Initialization

        public static FabricGatewayResourceManagerAgent Instance
        {
            get { return agent.Value; }
        }

        private FabricGatewayResourceManagerAgent()
        {
            Utility.WrapNativeSyncInvokeInMTA(this.CreateNativeAgent, "GatewayResourceManagerAgent.CreateNativeAgent");
        }

        private void CreateNativeAgent()
        {
            Guid iid = typeof(NativeGatewayResourceManager.IFabricGatewayResourceManagerAgent).GUID;

            //// PInvoke call
            this.nativeAgent = NativeGatewayResourceManager.CreateFabricGatewayResourceManagerAgent(ref iid);
        }

        #endregion

        #region API

        public string RegisterGatewayResourceManager(Guid partitionId, long replicaId, IGatewayResourceManager service)
        {
            Requires.Argument<Guid>("partitionId", partitionId).NotNull();

            return Utility.WrapNativeSyncInvokeInMTA(() => this.RegisterGatewayResourceManagerHelper(partitionId, replicaId, service), "GatewayResourceManagerAgent.RegisterGatewayResourceManager");
        }

        public void UnregisterGatewayResourceManager(Guid partitionId, long replicaId)
        {
            Requires.Argument<Guid>("partitionId", partitionId).NotNull();

            Utility.WrapNativeSyncInvokeInMTA(() => this.UnregisterGatewayResourceManagerHelper(partitionId, replicaId), "GatewayResourceManagerAgent.UnregisterGatewayResourceManager");
        }

        #endregion

        #region Register/Unregister FaultAnalysis Service

        private string RegisterGatewayResourceManagerHelper(Guid partitionId, long replicaId, IGatewayResourceManager service)
        {
            GatewayResourceManagerBroker broker = new GatewayResourceManagerBroker(service);

            NativeCommon.IFabricStringResult nativeString = this.nativeAgent.RegisterGatewayResourceManager(
                partitionId,
                replicaId,
                broker);

            return StringResult.FromNative(nativeString);
        }

        private void UnregisterGatewayResourceManagerHelper(Guid partitionId, long replicaId)
        {
            this.nativeAgent.UnregisterGatewayResourceManager(
                partitionId,
                replicaId);
        }

        #endregion
    }
}