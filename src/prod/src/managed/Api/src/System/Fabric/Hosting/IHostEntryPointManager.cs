// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Hosting
{
    //// These types must be kept in sync with ClrRuntimeHost.idl
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Interop;
    using System.Runtime.InteropServices;

    [ComImport]
    [ComVisible(true)]
    [Guid("918C9669-A03B-4A1D-8FEB-C732B1C0AF73")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    [SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121:UseBuiltInTypeAlias", Justification = "Size of parameters is stated for clarity as this is an interop interface")]
    internal interface IHostEntryPointManager
    {
        void Start([In, MarshalAs(UnmanagedType.LPWStr)] string configurationFilePath);

        void Stop();

        Int32 GetOrCreateAppDomainForCodePackage(
            [In, MarshalAs(UnmanagedType.LPWStr)] string activationContextId,
            [In] Int32 isolationPolicy,
            [In, MarshalAs(UnmanagedType.LPWStr)] string logDirectory,
            [In, MarshalAs(UnmanagedType.LPWStr)] string workDirectory);

        [return: MarshalAs(UnmanagedType.Interface)]
        NativeCommon.IFabricAsyncOperationContext BeginActivate(
            [In, MarshalAs(UnmanagedType.LPWStr)] string activationContextId,
            [In, MarshalAs(UnmanagedType.LPWStr)] string logDirectory,
            [In, MarshalAs(UnmanagedType.LPWStr)] string workDirectory,
            [In, MarshalAs(UnmanagedType.LPWStr)] string codePackageNameToActivate,
            [In, MarshalAs(UnmanagedType.LPWStr)] string codePackageDirectory,
            [In] int isolationPolicy,
            [In] IntPtr codePackageActivationContext, // this is a COM interface marshaled as an IntPTr and passed directly into the child app domain that unmarshals it
            [In] IntPtr fabricRuntime, // this is a COM interface marshaled as an IntPtr and passed onto child app domains that unmarshal it
            [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback asyncOperationCallback);

        void EndActivate([In] IntPtr asyncOperationContext);

        [return: MarshalAs(UnmanagedType.Interface)]
        NativeCommon.IFabricAsyncOperationContext BeginDeactivate(
            [In, MarshalAs(UnmanagedType.LPWStr)] string activationContextId,
            [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback asyncOperationCallback);

        /// <summary>
        /// 
        /// </summary>
        /// <param name="appDomainId">the app domain id in which the context was running</param>
        /// <param name="codePackageDirectory">the directory of the code package for this activation context</param>
        /// <param name="workDirectory">the work directory for this activation context</param>
        /// <param name="asyncOperationContext">context for this deactivate async operation</param>
        void EndDeactivate(
            [In] IntPtr asyncOperationContext, 
            [Out] out Int32 appDomainId, 
            [Out, MarshalAs(UnmanagedType.LPWStr)] out string codePackageDirectory,
            [Out, MarshalAs(UnmanagedType.LPWStr)] out string workDirectory);
    }
}