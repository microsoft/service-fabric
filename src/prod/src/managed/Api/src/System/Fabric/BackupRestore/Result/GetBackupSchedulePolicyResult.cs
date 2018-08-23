// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore
{
    using System.Fabric.Interop;
    using System.Fabric.BackupRestore.DataStructures;
    using System.Fabric.BackupRestore.Interop;

    internal class GetBackupSchedulePolicyResult : NativeBackupRestoreRuntime.IFabricGetBackupSchedulePolicyResult
    {
        private BackupPolicy policy;

        public GetBackupSchedulePolicyResult(BackupPolicy policy)
        {
            this.policy = policy;
        }

        public unsafe IntPtr get_BackupSchedulePolicy()
        {
            using (PinCollection pin = new PinCollection())
            {
                var nativeResult = this.policy.ToNative(pin);
                return nativeResult;
            }
        }
    }
}