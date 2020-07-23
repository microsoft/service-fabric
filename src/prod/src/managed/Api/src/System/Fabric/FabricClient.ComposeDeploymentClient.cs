// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Interop;
    using System.Threading;
    using System.Threading.Tasks;

    public partial class FabricClient
    {
        /// <summary>
        /// <para>Provides the functionality to manage Service Fabric compose deployments.</para>
        /// </summary>
        public sealed class ComposeDeploymentClient
        {
            #region Fields

            private readonly FabricClient fabricClient;
            private readonly NativeClient.IFabricApplicationManagementClient10 nativeManagementClient;
            private NativeClient.IInternalFabricApplicationManagementClient2 internalNativeApplicationClient;

            #endregion

            #region Constructor

            internal ComposeDeploymentClient(FabricClient fabricClient, NativeClient.IFabricApplicationManagementClient10 nativeManagementClient)
            {
                this.fabricClient = fabricClient;
                this.nativeManagementClient = nativeManagementClient;
                Utility.WrapNativeSyncInvokeInMTA(() =>
                {
                    this.internalNativeApplicationClient = (NativeClient.IInternalFabricApplicationManagementClient2)this.nativeManagementClient;
                },
                "ComposeDeploymentClient.ctor");
            }

            #endregion

            #region API
            
            internal Task CreateComposeDeploymentAsync(ComposeDeploymentDescriptionWrapper composeApplicationDescription)
            {
                return this.CreateComposeDeploymentAsync(composeApplicationDescription, FabricClient.DefaultTimeout); 
            }

            internal Task CreateComposeDeploymentAsync(ComposeDeploymentDescriptionWrapper composeApplicationDescription, TimeSpan timeout)
            {
                return this.CreateComposeDeploymentAsync(composeApplicationDescription, timeout, CancellationToken.None); 
            }

            internal Task CreateComposeDeploymentAsync(ComposeDeploymentDescriptionWrapper composeApplicationDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.CreateComposeDeploymentAsyncHelper(composeApplicationDescription, timeout, cancellationToken);
            }

            internal Task DeleteComposeDeploymentWrapperAsync(DeleteComposeDeploymentDescriptionWrapper description)
            {
                return this.DeleteComposeDeploymentWrapperAsync(description, FabricClient.DefaultTimeout);
            }

            internal Task DeleteComposeDeploymentWrapperAsync(DeleteComposeDeploymentDescriptionWrapper description, TimeSpan timeout)
            {
                return this.DeleteComposeDeploymentWrapperAsync(description, timeout, CancellationToken.None);
            }

            internal Task DeleteComposeDeploymentWrapperAsync(DeleteComposeDeploymentDescriptionWrapper description, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.DeleteComposeDeploymentAsyncHelper(description, timeout, cancellationToken);
            }

            internal Task UpgradeComposeDeploymentAsync(ComposeDeploymentUpgradeDescriptionWrapper upgradeDescription)
            {
                return this.UpgradeComposeDeploymentAsync(upgradeDescription, FabricClient.DefaultTimeout); 
            }

            internal Task UpgradeComposeDeploymentAsync(ComposeDeploymentUpgradeDescriptionWrapper upgradeDescription, TimeSpan timeout)
            {
                return this.UpgradeComposeDeploymentAsync(upgradeDescription, timeout, CancellationToken.None); 
            }

            internal Task UpgradeComposeDeploymentAsync(ComposeDeploymentUpgradeDescriptionWrapper upgradeDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.UpgradeComposeDeploymentAsyncHelper(upgradeDescription, timeout, cancellationToken);
            }

            internal Task RollbackComposeDeploymentUpgradeAsync(ComposeDeploymentRollbackDescriptionWrapper description)
            {
                return this.RollbackComposeDeploymentUpgradeAsync(description, FabricClient.DefaultTimeout);
            }

            internal Task RollbackComposeDeploymentUpgradeAsync(ComposeDeploymentRollbackDescriptionWrapper description, TimeSpan timeout)
            {
                return this.RollbackComposeDeploymentUpgradeAsync(description, timeout, CancellationToken.None);
            }

            internal Task RollbackComposeDeploymentUpgradeAsync(ComposeDeploymentRollbackDescriptionWrapper description, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.RollbackComposeDeploymentUpgradeAsyncHelper(description, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Retrieves the upgrade progress of the specified compose deployment.</para>
            /// </summary>
            /// <param name="deploymentName">
            /// <para>The name of the deployment.</para>
            /// </param>
            /// <returns>
            /// <para>A <see cref="System.Threading.Tasks.Task" /> whose result is the upgrade progress of the specified application instance.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationNotFound" />: The application does not exist.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>The request timed out but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>The request was canceled before the timeout expired but may have already been accepted for processing by the system.</para>
            /// </exception>
            internal Task<ComposeDeploymentUpgradeProgress> GetComposeDeploymentUpgradeProgressAsync(string deploymentName)
            {
                return this.GetComposeDeploymentUpgradeProgressAsync(deploymentName, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Retrieves the upgrade progress of the specified compose deployment.</para>
            /// </summary>
            /// <param name="deploymentName">
            /// <para>The name of the deployment.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A <see cref="System.Threading.Tasks.Task" /> whose result is the upgrade progress of the specified application instance.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationNotFound" />: The application does not exist.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>The request timed out but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>The request was canceled before the timeout expired but may have already been accepted for processing by the system.</para>
            /// </exception>
            internal Task<ComposeDeploymentUpgradeProgress> GetComposeDeploymentUpgradeProgressAsync(string deploymentName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<string>("deploymentName", deploymentName).NotNullOrEmpty();

                return this.GetComposeDeploymentUpgradeProgressAsyncHelper(deploymentName, timeout, cancellationToken);
            }
            #endregion

            #region Helpers & Callbacks
            #region Create Compose Application Async

            private Task CreateComposeDeploymentAsyncHelper(ComposeDeploymentDescriptionWrapper applicationDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.CreateComposeDeploymentBeginWrapper(applicationDescription, timeout, callback),
                    this.CreateComposeDeploymentEndWrapper,
                    cancellationToken,
                    "ApplicationManager.CreateComposeDeploymentAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext CreateComposeDeploymentBeginWrapper(
                ComposeDeploymentDescriptionWrapper applicationDescription, 
                TimeSpan timeout, 
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.internalNativeApplicationClient.BeginCreateComposeDeployment(
                        applicationDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void CreateComposeDeploymentEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.internalNativeApplicationClient.EndCreateComposeDeployment(context);
            }

            #endregion

            #region Delete Compose Application Async

            private Task DeleteComposeDeploymentAsyncHelper(DeleteComposeDeploymentDescriptionWrapper description, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.DeleteComposeDeploymentBeginWrapper(description, timeout, callback),
                    this.DeleteComposeDeploymentEndWrapper,
                    cancellationToken,
                    "ApplicationManager.DeleteComposeDeploymentAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext DeleteComposeDeploymentBeginWrapper(DeleteComposeDeploymentDescriptionWrapper description, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.internalNativeApplicationClient.BeginDeleteComposeDeployment(
                        description.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void DeleteComposeDeploymentEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.internalNativeApplicationClient.EndDeleteComposeDeployment(context);
            }

            #endregion

            #region Upgrade Compose Deployment Async

            private Task UpgradeComposeDeploymentAsyncHelper(ComposeDeploymentUpgradeDescriptionWrapper upgradeDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.UpgradeComposeDeploymentBeginWrapper(upgradeDescription, timeout, callback),
                    this.UpgradeComposeDeploymentEndWrapper,
                    cancellationToken,
                    "ApplicationManager.UpgradeComposeDeploymentAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext UpgradeComposeDeploymentBeginWrapper(
                ComposeDeploymentUpgradeDescriptionWrapper upgradeDescription, 
                TimeSpan timeout, 
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.internalNativeApplicationClient.BeginUpgradeComposeDeployment(
                        upgradeDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void UpgradeComposeDeploymentEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.internalNativeApplicationClient.EndUpgradeComposeDeployment(context);
            }

            #endregion

            #region Get Compose Deployment Upgrade Progress Async
            private Task<ComposeDeploymentUpgradeProgress> GetComposeDeploymentUpgradeProgressAsyncHelper(string deploymentName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<ComposeDeploymentUpgradeProgress>(
                    (callback) => this.GetComposeDeploymentUpgradeProgressBeginWrapper(deploymentName, timeout, callback),
                    this.GetComposeDeploymentUpgradeProgressEndWrapper,
                    cancellationToken,
                    "ApplicationManager.GetComposeDeploymentUpgradeProgressEndWrapper");
            }

            private NativeCommon.IFabricAsyncOperationContext GetComposeDeploymentUpgradeProgressBeginWrapper(string deploymentName, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pinName = PinBlittable.Create(deploymentName))
                {
                    return this.internalNativeApplicationClient.BeginGetComposeDeploymentUpgradeProgress(
                        pinName.AddrOfPinnedObject(),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private ComposeDeploymentUpgradeProgress GetComposeDeploymentUpgradeProgressEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return ComposeDeploymentUpgradeProgress.CreateFromNative(this.internalNativeApplicationClient.EndGetComposeDeploymentUpgradeProgress(context));
            }
            #endregion

            #region Rollback Compose Deployment Async

            private Task RollbackComposeDeploymentUpgradeAsyncHelper(ComposeDeploymentRollbackDescriptionWrapper description, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.RollbackComposeDeploymentBeginWrapper(description, timeout, callback),
                    this.RollbackComposeDeploymentEndWrapper,
                    cancellationToken,
                    "ApplicationManager.RollbackComposeDeploymentUpgradeAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext RollbackComposeDeploymentBeginWrapper(
                ComposeDeploymentRollbackDescriptionWrapper description,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.internalNativeApplicationClient.BeginRollbackComposeDeployment(
                        description.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void RollbackComposeDeploymentEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.internalNativeApplicationClient.EndRollbackComposeDeployment(context);
            }

            #endregion

            #endregion
        }
    }
}