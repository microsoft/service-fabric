// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Interop
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Runtime.InteropServices;
    using BOOLEAN = System.SByte;

    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1201:ElementsMustAppearInTheCorrectOrder", Justification = "Matches order in IDL.")]
    [SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121:UseBuiltInTypeAlias", Justification = "It is important here to explicitly state the size of integer parameters.")]
    internal static class NativeContainerActivatorService
    {
        //// ----------------------------------------------------------------------------
        //// Interfaces

        [ComImport]
        [Guid("111d2880-5b3e-48b3-895c-c62036830a14")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricContainerActivatorService
        {
            void StartEventMonitoring(
                [In] BOOLEAN isContainerServiceManaged,
                [In] UInt64 sinceTime);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginActivateContainer(
                [In] IntPtr activationParams,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            NativeCommon.IFabricStringResult EndActivateContainer(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginDeactivateContainer(
                [In] IntPtr deactivationParams,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndDeactivateContainer(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginDownloadImages(
                [In] IntPtr images,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndDownloadImages(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginDeleteImages(
                [In] IntPtr images,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndDeleteImages(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginInvokeContainerApi(
                [In] IntPtr apiExecArgs,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            IFabricContainerApiExecutionResult EndInvokeContainerApi(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("8fdba659-674c-4464-ac64-21d410313b96")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricContainerActivatorService2 : IFabricContainerActivatorService
        {
            // ----------------------------------------------------------------
            // IFabricContainerActivatorService methods
            // Base interface methods must come first to reserve vtable slots
            new void StartEventMonitoring(
                [In] BOOLEAN isContainerServiceManaged,
                [In] UInt64 sinceTime);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginActivateContainer(
                [In] IntPtr activationParams,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new NativeCommon.IFabricStringResult EndActivateContainer(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeactivateContainer(
                [In] IntPtr deactivationParams,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeactivateContainer(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDownloadImages(
                [In] IntPtr images,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDownloadImages(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeleteImages(
                [In] IntPtr images,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeleteImages(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginInvokeContainerApi(
                [In] IntPtr apiExecArgs,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricContainerApiExecutionResult EndInvokeContainerApi(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginContainerUpdateRoutes(
                [In] IntPtr updateRouteArgs,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndContainerUpdateRoutes(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("b40b7396-5d2a-471a-b6bc-6cfad1cb2061")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricContainerActivatorServiceAgent
        {
            void ProcessContainerEvents(
                [In] IntPtr notifiction);

            void RegisterContainerActivatorService(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricContainerActivatorService activatorService);
        }

        [ComImport]
        [Guid("ac2bcdde-3987-4bf0-ac91-989948faac85")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)] 
        internal interface IFabricContainerActivatorServiceAgent2 : IFabricContainerActivatorServiceAgent
        {
            // ----------------------------------------------------------------
            // IFabricContainerActivatorServiceAgent methods
            // Base interface methods must come first to reserve vtable slots
            new void ProcessContainerEvents(
                [In] IntPtr notifiction);

            new void RegisterContainerActivatorService(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricContainerActivatorService activatorService);

            void RegisterContainerActivatorService(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricContainerActivatorService2 activatorService);
        }

        [ComImport]
        [Guid("24e88e38-deb8-4bd9-9b55-ca67d6134850")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricHostingSettingsResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Result();
        }

        [ComImport]
        [Guid("c40d5451-32c4-481a-b340-a9e771cc6937")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricContainerApiExecutionResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Result();
        }

        //// ----------------------------------------------------------------------------
        //// Entry Points

#if DotNetCoreClrLinux
        [DllImport("libFabricContainerActivatorService.so", PreserveSig = false)]
#else
        [DllImport("FabricContainerActivatorService.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricContainerActivatorServiceAgent2 CreateFabricContainerActivatorServiceAgent(
            ref Guid iid);

#if DotNetCoreClrLinux
        [DllImport("libFabricContainerActivatorService.so", PreserveSig = false)]
#else
        [DllImport("FabricContainerActivatorService.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricHostingSettingsResult LoadHostingSettings();
    }
}