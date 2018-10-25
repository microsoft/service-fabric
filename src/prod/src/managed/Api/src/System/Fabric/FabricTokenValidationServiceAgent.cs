// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Reflection;

    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1201:ElementsMustAppearInTheCorrectOrder", Justification = "Preserve order of public members from V1.")]
    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1204:StaticElementsMustAppearBeforeInstanceElements", Justification = "Current grouping improves readability.")]
    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1202:ElementsMustBeOrderedByAccess", Justification = "Current grouping improves readability.")]
    internal sealed class FabricTokenValidationServiceAgent
    {
        #region Fields & Properties

        internal static readonly TimeSpan DefaultTimeout = TimeSpan.FromMinutes(1);

        private NativeTokenValidationService.IFabricTokenValidationServiceAgent nativeAgent;

        #endregion

        #region Constructors & Initialization

        public FabricTokenValidationServiceAgent()
        {
            Utility.WrapNativeSyncInvokeInMTA(() => this.CreateNativeAgent(), "TokenValidationServiceAgent.CreateNativeAgent");
        }

        private void CreateNativeAgent()
        {
            Guid iid = typeof(NativeTokenValidationService.IFabricTokenValidationServiceAgent).GetTypeInfo().GUID;

           //// PInvoke call
            this.nativeAgent = NativeTokenValidationService.CreateFabricTokenValidationServiceAgent(ref iid);
        }

        #endregion

        #region API

        public string RegisterTokenValidationService(Guid partitionId, long replicaId, ITokenValidationService service)
        {
            Requires.Argument<Guid>("partitionId", partitionId).NotNull();

            return Utility.WrapNativeSyncInvokeInMTA(() => this.RegisterTokenValidationServiceHelper(partitionId, replicaId, service), "TokenValidationServiceAgent.RegisterTokenValidationService");
        }

        public void UnregisterTokenValidationService(Guid partitionId, long replicaId)
        {
            Requires.Argument<Guid>("partitionId", partitionId).NotNull();

            Utility.WrapNativeSyncInvokeInMTA(() => this.UnregisterTokenValidationServiceHelper(partitionId, replicaId), "TokenValidationServiceAgent.UnregisterTokenValidationService");
        }

        #endregion

        #region Register/Unregister Infrastructure Service

        private string RegisterTokenValidationServiceHelper(Guid partitionId, long replicaId, ITokenValidationService service)
        {
            TokenValidationServiceBroker broker = new TokenValidationServiceBroker(service);

            NativeCommon.IFabricStringResult nativeString = this.nativeAgent.RegisterTokenValidationService(
                partitionId,
                replicaId,
                broker);

            return StringResult.FromNative(nativeString);
        }

        private void UnregisterTokenValidationServiceHelper(Guid partitionId, long replicaId)
        {
            this.nativeAgent.UnregisterTokenValidationService(
                partitionId,
                replicaId);
        }

        #endregion
    }
}