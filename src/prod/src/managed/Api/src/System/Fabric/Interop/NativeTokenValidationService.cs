// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Interop
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Runtime.CompilerServices;
    using System.Runtime.InteropServices;
    using BOOLEAN = System.SByte;
    using FABRIC_PARTITION_ID = System.Guid;
    using FABRIC_REPLICA_ID = System.Int64;

    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1201:ElementsMustAppearInTheCorrectOrder", Justification = "Matches order in IDL.")]
    [SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121:UseBuiltInTypeAlias", Justification = "It is important here to explicitly state the size of integer parameters.")]
    internal static class NativeTokenValidationService
    {
        //// ----------------------------------------------------------------------------
        //// Interfaces

        [ComImport]
        [Guid("a70f8024-32f8-48ab-84cf-2119a25513a9")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricTokenValidationService
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginValidateToken(
                [In] IntPtr authToken,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            IFabricTokenClaimResult EndValidateToken(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            IFabricTokenServiceMetadataResult GetTokenServiceMetadata();
        }

        [ComImport]
        [Guid("45898312-084e-4792-b234-52efb8d79cd8")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricTokenValidationServiceAgent
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult RegisterTokenValidationService(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] FABRIC_REPLICA_ID replicaId,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTokenValidationService service);

            void UnregisterTokenValidationService(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] FABRIC_REPLICA_ID replicaId);
        }

        [ComImport]
        [Guid("931ff709-4604-4e6a-bc10-a38056cbf3b4")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricTokenClaimResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Result();
        }

        [ComImport]
        [Guid("316ea660-56ec-4459-a4ad-8170787c5c48")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricTokenServiceMetadataResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Result();
        }

        //// ----------------------------------------------------------------------------
        //// Entry Points

#if DotNetCoreClrLinux
        [DllImport("libFabricTokenValidationService.so", PreserveSig = false)]
#else
        [DllImport("FabricTokenValidationService.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricTokenValidationServiceAgent CreateFabricTokenValidationServiceAgent(
            ref Guid iid);

#if DotNetCoreClrLinux
        [DllImport("libFabricTokenValidationService.so", PreserveSig = false)]
#else
        [DllImport("FabricTokenValidationService.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern NativeCommon.IFabricStringMapResult GetDefaultAzureActiveDirectoryConfigurations();
    }
}