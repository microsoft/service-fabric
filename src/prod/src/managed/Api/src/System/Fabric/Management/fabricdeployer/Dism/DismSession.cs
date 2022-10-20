// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#if !DotNetCoreClrLinux

namespace System.Fabric.FabricDeployer
{    
    using Microsoft.Win32.SafeHandles;

    internal sealed class DismSession : SafeHandleZeroOrMinusOneIsInvalid
    {
        public DismSession()
            : base(ownsHandle: true)
        {
        }

        protected override bool ReleaseHandle()
        {
            if (IsInvalid)
            {
                return true;
            }

            var error = DismNativeMethods.DismCloseSession(DangerousGetHandle());
            return error == DismNativeMethods.ERROR_SUCCESS;
        }
    }
}

#endif