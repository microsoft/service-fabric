// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    internal class NativeWindowsCredentialConverter : INativeCredentialConverter
    {
        private WindowsCredentials windowsCredentials;

        public NativeWindowsCredentialConverter(WindowsCredentials windowsCredentials)
        {
            this.windowsCredentials = windowsCredentials;
        }

        public IntPtr ToNative(PinCollection pin)
        {
            var nativeWindowsCredential = new NativeTypes.FABRIC_WINDOWS_CREDENTIALS[1];
            nativeWindowsCredential[0].Reserved = IntPtr.Zero;

            nativeWindowsCredential[0].RemoteSpn = pin.AddBlittable(this.windowsCredentials.RemoteSpn);

            nativeWindowsCredential[0].RemoteIdentityCount = (uint) this.windowsCredentials.RemoteIdentities.Count;

            var remoteIdentityArray = new IntPtr[this.windowsCredentials.RemoteIdentities.Count];
            for (var i = 0; i < this.windowsCredentials.RemoteIdentities.Count; ++i)
            {
                remoteIdentityArray[i] = pin.AddBlittable(this.windowsCredentials.RemoteIdentities[i]);
            }

            nativeWindowsCredential[0].RemoteIdentities = pin.AddBlittable(remoteIdentityArray);

            nativeWindowsCredential[0].ProtectionLevel = (NativeTypes.FABRIC_PROTECTION_LEVEL) this.windowsCredentials.ProtectionLevel;

            return pin.AddBlittable(nativeWindowsCredential);
        }
    }
}