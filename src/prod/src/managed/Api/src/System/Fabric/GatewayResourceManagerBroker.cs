// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Description;
    using System.Threading;
    using System.Threading.Tasks;

    [SuppressMessage(FxCop.Category.Design, FxCop.Rule.TypesThatOwnDisposableFieldsShouldBeDisposable, Justification = "// TODO: Implement an IDisposable, or ensure cleanup.")]
    internal sealed class GatewayResourceManagerBroker : NativeGatewayResourceManager.IFabricGatewayResourceManager
    {
        private const string TraceType = "GatewayResourceManagerBroker";

        private static readonly InteropApi ThreadErrorMessageSetter = 
            new InteropApi
                {
                    CopyExceptionDetailsToThreadErrorMessage = true,
                    ShouldIncludeStackTraceInThreadErrorMessage = false
                };

        private readonly IGatewayResourceManager service;

        internal GatewayResourceManagerBroker(IGatewayResourceManager service)
        {
            this.service = service;
        }

        internal IGatewayResourceManager Service
        {
            get
            {
                return this.service;
            }
        }

        NativeCommon.IFabricAsyncOperationContext NativeGatewayResourceManager.IFabricGatewayResourceManager.BeginUpdateOrCreateGatewayResource(
            IntPtr resourceDescription,
            UInt32 timeoutInMilliseconds, 
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var gatewayResourceDescription = NativeTypes.FromNativeString(resourceDescription);
            TimeSpan managedTimeout = TimeSpan.FromMilliseconds(timeoutInMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation((cancellationToken) => 
                this.UpdateOrCreateAsync(gatewayResourceDescription, managedTimeout, cancellationToken), callback, "GatewayResourceManagerBroker.UpdateOrCreateAsync", ThreadErrorMessageSetter);
        }

        NativeCommon.IFabricStringResult NativeGatewayResourceManager.IFabricGatewayResourceManager.EndUpdateOrCreateGatewayResource(
            NativeCommon.IFabricAsyncOperationContext context)
        {
            string gatewayResourceDescription = AsyncTaskCallInAdapter.End<string>(context);
            return new StringResult(gatewayResourceDescription);
        }

        private Task<string> UpdateOrCreateAsync(string resourceDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.service.UpdateOrCreateAsync(resourceDescription, timeout, cancellationToken);
        }

        NativeCommon.IFabricAsyncOperationContext NativeGatewayResourceManager.IFabricGatewayResourceManager.BeginQueryGatewayResources(
            IntPtr queryDescription,
            uint timeoutInMilliseconds, 
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            string gatewayQueryDescription = NativeTypes.FromNativeString(queryDescription);
            TimeSpan managedTimeout = TimeSpan.FromMilliseconds(timeoutInMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation((cancellationToken) => 
                this.QueryGatewayResourcesAsync(gatewayQueryDescription, managedTimeout, cancellationToken), callback, "GatewayResourceManagerBroker.QueryGatewayResourcesAsync", ThreadErrorMessageSetter);
        }

        NativeCommon.IFabricStringListResult NativeGatewayResourceManager.IFabricGatewayResourceManager.EndQueryGatewayResources(
            NativeCommon.IFabricAsyncOperationContext context)
        {
            var queryResult = AsyncTaskCallInAdapter.End<List<string>>(context);
            return new StringListResult(queryResult);
        }

        private Task<List<string>> QueryGatewayResourcesAsync(string queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.service.QueryAsync(queryDescription, timeout, cancellationToken);
        }

        NativeCommon.IFabricAsyncOperationContext NativeGatewayResourceManager.IFabricGatewayResourceManager.BeginDeleteGatewayResource(
            IntPtr resourceName,
            uint timeoutInMilliseconds, 
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var gatewayResourceName = NativeTypes.FromNativeString(resourceName);
            TimeSpan managedTimeout = TimeSpan.FromMilliseconds(timeoutInMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation((cancellationToken) => 
                this.DeleteGatewayResourceAsync(gatewayResourceName, managedTimeout, cancellationToken), callback, "GatewayResourceManagerBroker.DeleteGatewayResourceAsync", ThreadErrorMessageSetter);
        }

        void NativeGatewayResourceManager.IFabricGatewayResourceManager.EndDeleteGatewayResource(
            NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        private Task DeleteGatewayResourceAsync(string description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.service.DeleteAsync(description, timeout, cancellationToken);
        }
    }
}