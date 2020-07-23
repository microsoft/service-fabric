// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.Replicator
{
    using System;
    using System.Fabric.Interop;

    public sealed class ReplicatorLogSettings
    {
        public override string ToString()
        {
            return "LogSettings";
        }

        internal IntPtr ToNative(PinCollection pin)
        {
            NativeTypes.FABRIC_REPLICATOR_LOG_SETTINGS nativeObj = new NativeTypes.FABRIC_REPLICATOR_LOG_SETTINGS();
            
            return pin.AddBlittable(nativeObj);
        }
    }
}