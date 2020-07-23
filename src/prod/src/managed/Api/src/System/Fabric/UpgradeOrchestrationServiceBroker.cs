// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Fabric.Interop;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Fabric.Description;

namespace System.Fabric
{
    internal class UpgradeOrchestrationServiceBroker : NativeUpgradeOrchestrationService.IFabricUpgradeOrchestrationService
    {
        private static readonly InteropApi ThreadErrorMessageSetter = new InteropApi
        {
            CopyExceptionDetailsToThreadErrorMessage = true,
            ShouldIncludeStackTraceInThreadErrorMessage = false
        };

        private readonly IUpgradeOrchestrationService service;

        public UpgradeOrchestrationServiceBroker(IUpgradeOrchestrationService service)
        {
            this.service = service;
        }

        internal IUpgradeOrchestrationService Service
        {
            get
            {
                return this.service;
            }
        }

        #region StartClusterConfigurationUpgrade
        public NativeCommon.IFabricAsyncOperationContext BeginUpgradeConfiguration(IntPtr startUpgradeDescription, uint timeoutMilliseconds, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var managedStartUpgradeDescription = ConfigurationUpgradeDescription.CreateFromNative(startUpgradeDescription);

            TimeSpan managedTimeout = TimeSpan.FromMilliseconds(timeoutMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) =>
                    this.StartClusterConfigurationUpgradeAsync(
                    managedStartUpgradeDescription,
                    managedTimeout,
                    cancellationToken),
                    callback,
                    "UpgradeOrchestrationServiceBroker.StartClusterConfigurationUpgrade",
                    ThreadErrorMessageSetter);
        }

        public void EndUpgradeConfiguration(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        private Task StartClusterConfigurationUpgradeAsync(ConfigurationUpgradeDescription startUpgradeDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.service.StartClusterConfigurationUpgrade(startUpgradeDescription, timeout, cancellationToken);
        }
        #endregion StartClusterConfigurationUpgrade

        #region GetClusterConfigurationUpgradeeStatus
        public NativeCommon.IFabricAsyncOperationContext BeginGetClusterConfigurationUpgradeStatus(uint timeoutMilliseconds, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            TimeSpan managedTimeout = TimeSpan.FromMilliseconds(timeoutMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) =>
                    this.GetClusterConfigurationUpgradeStatusAsync(
                        managedTimeout,
                        cancellationToken),
                    callback,
                    "UpgradeOrchestrationServiceBroker.GetClusterConfigurationUpgradeStatus",
                    ThreadErrorMessageSetter);
        }

        public NativeClient.IFabricOrchestrationUpgradeStatusResult EndGetClusterConfigurationUpgradeStatus(NativeCommon.IFabricAsyncOperationContext context)
        {
            var progress = AsyncTaskCallInAdapter.End<FabricOrchestrationUpgradeProgress>(context);
            return new OrchestrationUpgradeResult(progress);
        }

        private Task<FabricOrchestrationUpgradeProgress> GetClusterConfigurationUpgradeStatusAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.service.GetClusterConfigurationUpgradeStatus(timeout, cancellationToken);
        }

        #endregion GetClusterConfigurationUpgradeStatus

        #region GetClusterConfiguration
        public NativeCommon.IFabricAsyncOperationContext BeginGetClusterConfiguration(IntPtr apiVersion, uint timeoutMilliseconds, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            TimeSpan managedTimeout = TimeSpan.FromMilliseconds(timeoutMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) =>
                    this.GetClusterConfigurationAsync(
                        NativeTypes.FromNativeString(apiVersion),
                        managedTimeout,
                        cancellationToken),
                    callback,
                    "UpgradeOrchestrationServiceBroker.GetClusterConfiguration",
                    ThreadErrorMessageSetter);
        }

        public NativeCommon.IFabricStringResult EndGetClusterConfiguration(NativeCommon.IFabricAsyncOperationContext context)
        {
            var clusterConfiguration = AsyncTaskCallInAdapter.End<String>(context);
            return new StringResult(clusterConfiguration);
        }

        private Task<string> GetClusterConfigurationAsync(string apiVersion, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.service.GetClusterConfiguration(apiVersion, timeout, cancellationToken);
        }

        #endregion GetClusterConfiguration

        #region GetUpgradesPendingApproval
        public NativeCommon.IFabricAsyncOperationContext BeginGetUpgradesPendingApproval(uint timeoutMilliseconds, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            TimeSpan managedTimeout = TimeSpan.FromMilliseconds(timeoutMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) =>
                    this.GetUpgradesPendingApprovalAsync(
                        managedTimeout,
                        cancellationToken),
                    callback,
                    "UpgradeOrchestrationServiceBroker.GetUpgradeStatus",
                    ThreadErrorMessageSetter);
        }

        public void EndGetUpgradesPendingApproval(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        private Task GetUpgradesPendingApprovalAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.service.GetUpgradesPendingApproval(timeout, cancellationToken);
        }

        #endregion GetUpgradesPendingApproval

        #region StartApprovedUpgrades
        public NativeCommon.IFabricAsyncOperationContext BeginStartApprovedUpgrades(uint timeoutMilliseconds, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            TimeSpan managedTimeout = TimeSpan.FromMilliseconds(timeoutMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) =>
                    this.StartApprovedUpgradesAsync(
                    managedTimeout,
                    cancellationToken),
                    callback,
                    "UpgradeOrchestrationServiceBroker.StartApprovedUpgrades",
                    ThreadErrorMessageSetter);
        }

        public void EndStartApprovedUpgrades(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        private Task StartApprovedUpgradesAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.service.StartApprovedUpgrades(timeout, cancellationToken);
        }

        #endregion StartApprovedUpgrades

        #region CallSystemService
        public NativeCommon.IFabricAsyncOperationContext BeginCallSystemService(
            IntPtr action,
            IntPtr inputBlob,
            UInt32 timeoutMilliseconds,
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            TimeSpan managedTimeout = TimeSpan.FromMilliseconds(timeoutMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) =>
                    this.CallSystemServiceAsync(
                        NativeTypes.FromNativeString(action),
                        NativeTypes.FromNativeString(inputBlob),
                        managedTimeout,
                        cancellationToken),
                    callback,
                    "UpgradeOrchestrationServiceBroker.CallSystemService",
                    ThreadErrorMessageSetter);
        }

        public NativeCommon.IFabricStringResult EndCallSystemService(NativeCommon.IFabricAsyncOperationContext context)
        {
            var outputBlob = AsyncTaskCallInAdapter.End<String>(context);
            return new StringResult(outputBlob);
        }

        private Task<string> CallSystemServiceAsync(string action, string inputBlob, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.service.CallSystemService(action, inputBlob, timeout, cancellationToken);
        }

        #endregion
    }
}