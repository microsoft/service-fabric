// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace System.Fabric.Test.Interop.Mscoree
{
    [Guid("CB2F6722-AB3A-11D2-9C40-00C04FA30A3E")]
    [InterfaceType((short)1)]
    [ComConversionLoss]
    [ComImport]
    internal interface ICorRuntimeHost
    {
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void CreateLogicalThreadState();

        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void DeleteLogicalThreadState();

        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void SwitchInLogicalThreadState([In] ref uint pFiberCookie);

        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void SwitchOutLogicalThreadState([Out] IntPtr pFiberCookie);

        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void LocksHeldByLogicalThread(out uint pCount);

        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void MapFile([In] IntPtr hFile, out IntPtr hMapAddress);

        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void GetConfiguration([MarshalAs(UnmanagedType.IUnknown)] out object pConfiguration);

        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void Start();

        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void Stop();

        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void CreateDomain([MarshalAs(UnmanagedType.LPWStr), In] string pwzFriendlyName, [MarshalAs(UnmanagedType.IUnknown), In] object pIdentityArray, [MarshalAs(UnmanagedType.IUnknown)] out object pAppDomain);

        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void GetDefaultDomain([MarshalAs(UnmanagedType.IUnknown)] out object pAppDomain);

        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void EnumDomains(out IntPtr hEnum);

        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void NextDomain([In] IntPtr hEnum, [MarshalAs(UnmanagedType.IUnknown)] out object pAppDomain);

        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void CloseEnum([In] IntPtr hEnum);

        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void CreateDomainEx([MarshalAs(UnmanagedType.LPWStr), In] string pwzFriendlyName, [MarshalAs(UnmanagedType.IUnknown), In] object pSetup, [MarshalAs(UnmanagedType.IUnknown), In] object pEvidence, [MarshalAs(UnmanagedType.IUnknown)] out object pAppDomain);

        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void CreateDomainSetup([MarshalAs(UnmanagedType.IUnknown)] out object pAppDomainSetup);

        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void CreateEvidence([MarshalAs(UnmanagedType.IUnknown)] out object pEvidence);

        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void UnloadDomain([MarshalAs(UnmanagedType.IUnknown), In] object pAppDomain);

        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void CurrentDomain([MarshalAs(UnmanagedType.IUnknown)] out object pAppDomain);
    }

    internal static class Ole32Methods
    {
        public const uint CLSCTX_INPROC_SERVER = 0x1;

        [DllImport("ole32.dll")]
        static public extern uint CoCreateInstance(ref Guid clsid,
           [MarshalAs(UnmanagedType.IUnknown)] object inner,
           uint context,
           ref Guid uuid,
           [MarshalAs(UnmanagedType.IUnknown)] out object rReturnedComObject);
    }
}