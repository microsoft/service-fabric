// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Interop
{
    using System.Diagnostics.CodeAnalysis;
    using System.Runtime.InteropServices;
    using BOOLEAN = System.SByte;

    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1201:ElementsMustAppearInTheCorrectOrder", Justification = "Matches order in IDL.")]
    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1202:ElementsMustBeOrderedByAccess", Justification = "Matches order in IDL.")]
    [SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121:UseBuiltInTypeAlias", Justification = "It is important here to explicitly state the size of integer parameters")]
    internal static class NativeClientInternal
    {
        //// ----------------------------------------------------------------------------
        //// Entry Points

#if DotNetCoreClrLinux
        [DllImport("libFabricClient.so", PreserveSig = false)]
#else
        [DllImport("FabricClient.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern NativeClient.IFabricClientSettingsResult GetFabricClientDefaultSettings();
    }
}