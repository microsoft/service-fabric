// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.BackupRestore.Interop;
using System.Fabric.Interop;
using System.Globalization;
using System.Text;

    namespace System.Fabric.BackupRestore.DataStructures
{
    /// <summary>
    /// Backup Partition Info structure
    /// </summary>
    internal class BackupPartitionInfo
    {
        private const string TraceComponent = "BackupPartitionInfo";

        /// <summary>
        /// Name of the service in fabric:/AppName/SvcName format
        /// </summary>
        public string ServiceName { get; set; }

        /// <summary>
        /// Partition ID Guid
        /// </summary>
        public Guid PartitionId { get; set; }

        /// <summary>
        /// Gets a string representation of the backup partition info object.
        /// </summary>
        /// <returns>A string representation of the backup partition info object.</returns>
        public override string ToString()
        {
            var sb = new StringBuilder();
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "ServiceName={0}", this.ServiceName));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "PartitionId={0}", this.PartitionId));

            return sb.ToString();
        }

        #region Interop Helpers
        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativePartitionInfo = new NativeBackupRestoreTypes.FABRIC_BACKUP_PARTITION_INFO
            {
                ServiceName = pinCollection.AddObject(ServiceName),
                PartitionId = PartitionId,
                Reserved = IntPtr.Zero,
            };

            return pinCollection.AddBlittable(nativePartitionInfo);
        }

        internal static unsafe BackupPartitionInfo FromNative(IntPtr pointer)
        {
            var nativePartitionInfo = *(NativeBackupRestoreTypes.FABRIC_BACKUP_PARTITION_INFO*) pointer;
            return new BackupPartitionInfo
            {
                PartitionId = nativePartitionInfo.PartitionId,
                ServiceName = NativeTypes.FromNativeString(nativePartitionInfo.ServiceName),
            };
        }
        #endregion
    }
}