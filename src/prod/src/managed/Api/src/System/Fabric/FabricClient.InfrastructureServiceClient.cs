// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Fabric.Interop;
    using System.Threading;
    using System.Threading.Tasks;

    public partial class FabricClient
    {
        /// <summary>
        /// <para>Provides methods for performing infrastructure-specific operations.</para>
        /// <para>This class supports the Service Fabric platform; it is not meant to be called directly from your code.</para>
        /// </summary>
        /// <remarks>
        /// <para>The InfrastructureService must be enabled before this client can be used. The InfrastructureService is only supported on some infrastructures.</para>
        /// </remarks>
        public sealed class InfrastructureServiceClient
        {
            #region Fields

            private readonly FabricClient fabricClient;
            private readonly NativeClient.IFabricInfrastructureServiceClient nativeInfraServiceClient;

            #endregion

            #region Constructor

            internal InfrastructureServiceClient(FabricClient fabricClient, NativeClient.IFabricInfrastructureServiceClient nativeInfraServiceClient)
            {
                this.fabricClient = fabricClient;
                this.nativeInfraServiceClient = nativeInfraServiceClient;
            }

            #endregion

            #region API

            /// <summary>
            /// <para>Asynchronously invokes an administrative command on the given infrastructure service instance.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The name of the target infrastructure service instance.</para>
            /// </param>
            /// <param name="command">
            /// <para>The text of the command to be invoked.  The content of the command is infrastructure-specific.</para>
            /// </param>
            /// <returns>
            /// <para>The response from the infrastructure service. The response format is a JSON string. The contents of the response depend on which command was issued.</para>
            /// </returns>
            public Task<string> InvokeInfrastructureCommandAsync(Uri serviceName, string command)
            {
                return this.InvokeInfrastructureCommandAsync(serviceName, command, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Asynchronously invokes an administrative command on an infrastructure service.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The name of the target infrastructure service instance.</para>
            /// </param>
            /// <param name="command">
            /// <para>The text of the command to be invoked.  The content of the command is infrastructure-specific.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation may still be completed even if it is cancelled.</para>
            /// </param>
            /// <returns>
            /// <para>The response from the infrastructure service. The response format is a JSON string. The contents of the response depend on which command was issued.</para>
            /// </returns>
            public Task<string> InvokeInfrastructureCommandAsync(Uri serviceName, string command, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.InvokeInfrastructureCommandAsyncHelper(serviceName, command, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Asynchronously invokes a read-only query on the given infrastructure service instance.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The name of the target infrastructure service instance.</para>
            /// </param>
            /// <param name="command">
            /// <para>The text of the command to be invoked.  The content of the command is infrastructure-specific.</para>
            /// </param>
            /// <returns>
            /// <para>The response from the infrastructure service. The response format is a JSON string. The contents of the response depend on which command was issued.</para>
            /// </returns>
            public Task<string> InvokeInfrastructureQueryAsync(Uri serviceName, string command)
            {
                return this.InvokeInfrastructureQueryAsync(serviceName, command, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Asynchronously invokes a read-only query on the given infrastructure service instance.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The name of the target infrastructure service instance.</para>
            /// </param>
            /// <param name="command">
            /// <para>The text of the command to be invoked.  The content of the command is infrastructure-specific.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation may still be completed even if it is cancelled.</para>
            /// </param>
            /// <returns>
            /// <para>The response from the infrastructure service. The response format is a JSON string. The contents of the response depend on which command was issued.</para>
            /// </returns>
            public Task<string> InvokeInfrastructureQueryAsync(Uri serviceName, string command, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.InvokeInfrastructureQueryAsyncHelper(serviceName, command, timeout, cancellationToken);
            }

            #endregion

            #region Helpers

            private Task<string> InvokeInfrastructureCommandAsyncHelper(Uri serviceName, string command, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<string>(
                    (callback) => this.InvokeInfrastructureCommandAsyncBeginWrapper(serviceName, command, timeout, callback),
                    this.InvokeInfrastructureCommandAsyncEndWrapper,
                    cancellationToken,
                    "InfrastructureService.InvokeInfrastructureCommand");
            }

            private NativeCommon.IFabricAsyncOperationContext InvokeInfrastructureCommandAsyncBeginWrapper(Uri serviceName, string command, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeInfraServiceClient.BeginInvokeInfrastructureCommand(
                        pin.AddObject(serviceName),
                        pin.AddObject(command),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private string InvokeInfrastructureCommandAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return NativeTypes.FromNativeString(this.nativeInfraServiceClient.EndInvokeInfrastructureCommand(context));
            }

            private Task<string> InvokeInfrastructureQueryAsyncHelper(Uri serviceName, string command, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<string>(
                    (callback) => this.InvokeInfrastructureQueryAsyncBeginWrapper(serviceName, command, timeout, callback),
                    this.InvokeInfrastructureQueryAsyncEndWrapper,
                    cancellationToken,
                    "InfrastructureService.InvokeInfrastructureQuery");
            }

            private NativeCommon.IFabricAsyncOperationContext InvokeInfrastructureQueryAsyncBeginWrapper(Uri serviceName, string command, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeInfraServiceClient.BeginInvokeInfrastructureQuery(
                        pin.AddObject(serviceName),
                        pin.AddObject(command),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private string InvokeInfrastructureQueryAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return NativeTypes.FromNativeString(this.nativeInfraServiceClient.EndInvokeInfrastructureQuery(context));
            }

            #endregion
        }
    }
}