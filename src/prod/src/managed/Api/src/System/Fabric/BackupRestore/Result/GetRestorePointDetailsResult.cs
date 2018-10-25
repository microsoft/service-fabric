// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore
{
    using System.Fabric.Interop;
    using System.Fabric.BackupRestore.DataStructures;
    using System.Fabric.BackupRestore.Interop;

    internal class GetRestorePointDetailsResult : NativeBackupRestoreRuntime.IFabricGetRestorePointDetailsResult
    {
        private readonly RestorePointDetails restorePoint;

        public GetRestorePointDetailsResult(RestorePointDetails recoveryPoint)
        {
            this.restorePoint = recoveryPoint;
        }

        public unsafe IntPtr get_RestorePointDetails()
        {
            using (var pin = new PinCollection())
            {
                var nativeResult = this.restorePoint.ToNative(pin);
                return nativeResult;
            }
        }
    }
}