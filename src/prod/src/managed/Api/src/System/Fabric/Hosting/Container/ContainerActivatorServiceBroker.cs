// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;
    using BOOLEAN = System.SByte;

    internal sealed class ContainerActivatorServiceBroker : NativeContainerActivatorService.IFabricContainerActivatorService, NativeContainerActivatorService.IFabricContainerActivatorService2
    {
        private readonly IContainerActivatorService activatorService;

        private static readonly InteropApi ValidateAsyncApi = new InteropApi
        {
            CopyExceptionDetailsToThreadErrorMessage = true,
            ShouldIncludeStackTraceInThreadErrorMessage = false
        };

        internal ContainerActivatorServiceBroker(IContainerActivatorService activator)
        {
            this.activatorService = activator;
        }

        void NativeContainerActivatorService.IFabricContainerActivatorService.StartEventMonitoring(
            BOOLEAN isContainerServiceManaged,
            UInt64 sinceTime)
        {
            var isManaged = NativeTypes.FromBOOLEAN(isContainerServiceManaged);

            Utility.WrapNativeSyncMethodImplementation(
                () => this.activatorService.StartEventMonitoring(isManaged, sinceTime),
                "FabricContainerActivatorServiceBroker.StartEventMonitoring",
                ValidateAsyncApi);
        }

        NativeCommon.IFabricAsyncOperationContext NativeContainerActivatorService.IFabricContainerActivatorService.BeginActivateContainer(
            IntPtr activationParams,
            uint timeoutMilliseconds,
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var activationArgs = ContainerActivationArgs.CreateFromNative(activationParams);
            var timeout = TimeSpan.FromMilliseconds(timeoutMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) => this.activatorService.ActivateContainerAsync(activationArgs, timeout),
                callback,
                "FabricContainerActivatorServiceBroker.ActivateContainerAsync",
                ValidateAsyncApi);
        }

        NativeCommon.IFabricStringResult NativeContainerActivatorService.IFabricContainerActivatorService.EndActivateContainer(
            NativeCommon.IFabricAsyncOperationContext context)
        {
            var containerId = AsyncTaskCallInAdapter.End<string>(context);
            return new StringResult(containerId);
        }

        NativeCommon.IFabricAsyncOperationContext NativeContainerActivatorService.IFabricContainerActivatorService.BeginDeactivateContainer(
            IntPtr deactivationParams,
            uint timeoutMilliseconds,
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var deactivationArgs = ContainerDeactivationArgs.CreateFromNative(deactivationParams);
            var timeout = TimeSpan.FromMilliseconds(timeoutMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) => this.activatorService.DeactivateContainerAsync(deactivationArgs, timeout),
                callback,
                "FabricContainerActivatorServiceBroker.DeactivateContainerAsync",
                ValidateAsyncApi);
        }

        void NativeContainerActivatorService.IFabricContainerActivatorService.EndDeactivateContainer(
            NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        NativeCommon.IFabricAsyncOperationContext NativeContainerActivatorService.IFabricContainerActivatorService.BeginDownloadImages(
            IntPtr images,
            uint timeoutMilliseconds,
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var imagesList = ContainerImageDescription.CreateFromNativeList(images);
            var timeout = TimeSpan.FromMilliseconds(timeoutMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) => this.activatorService.DownloadImagesAsync(imagesList, timeout),
                callback,
                "FabricContainerActivatorServiceBroker.DownloadImagesAsync",
                ValidateAsyncApi);
        }

        void NativeContainerActivatorService.IFabricContainerActivatorService.EndDownloadImages(
            NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        NativeCommon.IFabricAsyncOperationContext NativeContainerActivatorService.IFabricContainerActivatorService.BeginDeleteImages(
            IntPtr images,
            uint timeoutMilliseconds,
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var imagesList = NativeTypes.FromNativeStringList(images);
            var timeout = TimeSpan.FromMilliseconds(timeoutMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) => this.activatorService.DeleteImagesAsync(imagesList, timeout),
                callback,
                "FabricContainerActivatorServiceBroker.DeleteImagesAsync",
                ValidateAsyncApi);
        }

        void NativeContainerActivatorService.IFabricContainerActivatorService.EndDeleteImages(
            NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        NativeCommon.IFabricAsyncOperationContext NativeContainerActivatorService.IFabricContainerActivatorService.BeginInvokeContainerApi(
            IntPtr apiExecArgs,
            uint timeoutMilliseconds,
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var execArgs = ContainerApiExecutionArgs.CreateFromNative(apiExecArgs);
            var timeout = TimeSpan.FromMilliseconds(timeoutMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) => this.activatorService.InvokeContainerApiAsync(execArgs, timeout),
                callback,
                "ContainerActivatorServiceBroker.InvokeContainerApiAsync",
                ValidateAsyncApi);
        }

        NativeContainerActivatorService.IFabricContainerApiExecutionResult NativeContainerActivatorService.IFabricContainerActivatorService.EndInvokeContainerApi(
            NativeCommon.IFabricAsyncOperationContext context)
        {
            var apiExecResponse = AsyncTaskCallInAdapter.End<ContainerApiExecutionResponse>(context);
            return new FabricContainerApiExecutionResult(apiExecResponse);
        }

        void NativeContainerActivatorService.IFabricContainerActivatorService2.StartEventMonitoring(
            BOOLEAN isContainerServiceManaged,
            UInt64 sinceTime)
        {
            var isManaged = NativeTypes.FromBOOLEAN(isContainerServiceManaged);

            Utility.WrapNativeSyncMethodImplementation(
                () => this.activatorService.StartEventMonitoring(isManaged, sinceTime),
                "FabricContainerActivatorServiceBroker.StartEventMonitoring",
                ValidateAsyncApi);
        }

        NativeCommon.IFabricAsyncOperationContext NativeContainerActivatorService.IFabricContainerActivatorService2.BeginActivateContainer(
            IntPtr activationParams,
            uint timeoutMilliseconds,
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var activationArgs = ContainerActivationArgs.CreateFromNative(activationParams);
            var timeout = TimeSpan.FromMilliseconds(timeoutMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) => this.activatorService.ActivateContainerAsync(activationArgs, timeout),
                callback,
                "FabricContainerActivatorServiceBroker.ActivateContainerAsync",
                ValidateAsyncApi);
        }

        NativeCommon.IFabricStringResult NativeContainerActivatorService.IFabricContainerActivatorService2.EndActivateContainer(
            NativeCommon.IFabricAsyncOperationContext context)
        {
            var containerId = AsyncTaskCallInAdapter.End<string>(context);
            return new StringResult(containerId);
        }

        NativeCommon.IFabricAsyncOperationContext NativeContainerActivatorService.IFabricContainerActivatorService2.BeginDeactivateContainer(
            IntPtr deactivationParams,
            uint timeoutMilliseconds,
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var deactivationArgs = ContainerDeactivationArgs.CreateFromNative(deactivationParams);
            var timeout = TimeSpan.FromMilliseconds(timeoutMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) => this.activatorService.DeactivateContainerAsync(deactivationArgs, timeout),
                callback,
                "FabricContainerActivatorServiceBroker.DeactivateContainerAsync",
                ValidateAsyncApi);
        }

        void NativeContainerActivatorService.IFabricContainerActivatorService2.EndDeactivateContainer(
            NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        NativeCommon.IFabricAsyncOperationContext NativeContainerActivatorService.IFabricContainerActivatorService2.BeginDownloadImages(
            IntPtr images,
            uint timeoutMilliseconds,
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var imagesList = ContainerImageDescription.CreateFromNativeList(images);
            var timeout = TimeSpan.FromMilliseconds(timeoutMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) => this.activatorService.DownloadImagesAsync(imagesList, timeout),
                callback,
                "FabricContainerActivatorServiceBroker.DownloadImagesAsync",
                ValidateAsyncApi);
        }

        void NativeContainerActivatorService.IFabricContainerActivatorService2.EndDownloadImages(
            NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        NativeCommon.IFabricAsyncOperationContext NativeContainerActivatorService.IFabricContainerActivatorService2.BeginDeleteImages(
            IntPtr images,
            uint timeoutMilliseconds,
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var imagesList = NativeTypes.FromNativeStringList(images);
            var timeout = TimeSpan.FromMilliseconds(timeoutMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) => this.activatorService.DeleteImagesAsync(imagesList, timeout),
                callback,
                "FabricContainerActivatorServiceBroker.DeleteImagesAsync",
                ValidateAsyncApi);
        }

        void NativeContainerActivatorService.IFabricContainerActivatorService2.EndDeleteImages(
            NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        NativeCommon.IFabricAsyncOperationContext NativeContainerActivatorService.IFabricContainerActivatorService2.BeginInvokeContainerApi(
            IntPtr apiExecArgs,
            uint timeoutMilliseconds,
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var execArgs = ContainerApiExecutionArgs.CreateFromNative(apiExecArgs);
            var timeout = TimeSpan.FromMilliseconds(timeoutMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) => this.activatorService.InvokeContainerApiAsync(execArgs, timeout),
                callback,
                "ContainerActivatorServiceBroker.InvokeContainerApiAsync",
                ValidateAsyncApi);
        }

        NativeContainerActivatorService.IFabricContainerApiExecutionResult NativeContainerActivatorService.IFabricContainerActivatorService2.EndInvokeContainerApi(
            NativeCommon.IFabricAsyncOperationContext context)
        {
            var apiExecResponse = AsyncTaskCallInAdapter.End<ContainerApiExecutionResponse>(context);
            return new FabricContainerApiExecutionResult(apiExecResponse);
        }

        NativeCommon.IFabricAsyncOperationContext NativeContainerActivatorService.IFabricContainerActivatorService2.BeginContainerUpdateRoutes(
           IntPtr updateRouteArgs,
           uint timeoutMilliseconds,
           NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var updateRoutes = ContainerUpdateRouteArgs.CreateFromNative(updateRouteArgs);
            var timeout = TimeSpan.FromMilliseconds(timeoutMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) => this.activatorService.ExecuteUpdateRoutesAsync(updateRoutes, timeout),
                callback,
                "FabricContainerActivatorServiceBroker.ExecuteUpdateRoutesAsync",
                ValidateAsyncApi);
        }

        void NativeContainerActivatorService.IFabricContainerActivatorService2.EndContainerUpdateRoutes(
            NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }
    }
}