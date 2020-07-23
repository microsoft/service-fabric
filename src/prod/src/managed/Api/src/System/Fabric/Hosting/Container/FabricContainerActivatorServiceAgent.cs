// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Interop;
    using System.Reflection;

    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1201:ElementsMustAppearInTheCorrectOrder", Justification = "Preserve order of public members from V1.")]
    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1204:StaticElementsMustAppearBeforeInstanceElements", Justification = "Current grouping improves readability.")]
    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1202:ElementsMustBeOrderedByAccess", Justification = "Current grouping improves readability.")]
    internal sealed class FabricContainerActivatorServiceAgent
    {
        #region Fields & Properties

        internal static readonly TimeSpan DefaultTimeout = TimeSpan.FromMinutes(1);

        private NativeContainerActivatorService.IFabricContainerActivatorServiceAgent2 nativeManager;

        #endregion

        #region Constructors & Initialization

        public FabricContainerActivatorServiceAgent()
        {
            Utility.WrapNativeSyncInvokeInMTA(
                () => this.CreateNativeAgent(), 
                "FabricContainerActivatorServiceAgent.CreateNativeAgent");
        }

        private void CreateNativeAgent()
        {
            var iid = typeof(NativeContainerActivatorService.IFabricContainerActivatorServiceAgent2).GetTypeInfo().GUID;

            this.nativeManager = NativeContainerActivatorService.CreateFabricContainerActivatorServiceAgent(ref iid);
        }

        #endregion

        #region API

        public void RegisterContainerActivatorService(IContainerActivatorService service)
        {
            Utility.WrapNativeSyncInvokeInMTA(
                () => this.RegisterContainerActivatorServiceHelper(service),
                "FabricContainerActivatorServiceAgent.RegisterContainerActivatorService");
        }

        public void ProcessContainerEvents(ContainerEventNotification notification)
        {
            Utility.WrapNativeSyncInvokeInMTA(
                () => this.ProcessContainerEventsHelper(notification),
                "FabricContainerActivatorServiceAgent.ProcessContainerEvents");
        }

        #endregion

        #region Helper Methods

        private void RegisterContainerActivatorServiceHelper(IContainerActivatorService service)
        {
            var broker = new ContainerActivatorServiceBroker(service);
            this.nativeManager.RegisterContainerActivatorService(broker);
        }

        private void ProcessContainerEventsHelper(ContainerEventNotification notification)
        {
            using (var pin = new PinCollection())
            {
                var notificationPtr = notification.ToNative(pin);
                this.nativeManager.ProcessContainerEvents(notificationPtr);
            }
        }

        #endregion
    }
}