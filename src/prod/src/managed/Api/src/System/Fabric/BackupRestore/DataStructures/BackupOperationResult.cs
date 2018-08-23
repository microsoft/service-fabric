// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.BackupRestore.Interop;
using System.Fabric.Interop;
using System.Globalization;
using System.Text;
using HRESULT = System.Int32;

namespace System.Fabric.BackupRestore.DataStructures
{
    /// <summary>
    /// Structure to post backup operation result back to the Backup Restore service
    /// </summary>
    internal class BackupOperationResult
    {
        private const string TraceComponent = "BackupOperationResult";

        /// <summary>
        /// Name of the service in fabric:/AppName/SvcName format
        /// </summary>
        public string ServiceName { get; set; }

        /// <summary>
        /// Partition ID Guid
        /// </summary>
        public Guid PartitionId { get; set; }

        /// <summary>
        /// Status of this operation depicting using error code
        /// </summary>
        public HRESULT ErrorCode { get; set; }

        /// <summary>
        /// Operation result message
        /// </summary>
        public string Message { get; set; }
        
        /// <summary>
        /// TimeStamp when operation succeded or failed
        /// </summary>
        public DateTime TimeStampUtc { get; set; }

        /// <summary>
        /// Backup Operation ID
        /// </summary>
        public Guid OperationId { get; set; }

        /// <summary>
        /// Backup ID
        /// </summary>
        public Guid BackupId { get; set; }

        /// <summary>
        /// Backup location relative to the backup store
        /// </summary>
        public string BackupLocation { get; set; }

        /// <summary>
        /// Gets or sets the Epoch of last backed up record
        /// </summary>
        public Epoch EpochOfLastBackupRecord { get; set; }

        /// <summary>
        /// Gets or sets the LSN of last backed up record
        /// </summary>
        public long LsnOfLastBackupRecord { get; set; }

        /// <summary>
        /// Gets a string representation of the BackupOperationResult object.
        /// </summary>
        /// <returns>A string representation of the BackupOperationResult object.</returns>
        public override string ToString()
        {
            var sb = new StringBuilder();
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "ServiceName={0}", this.ServiceName));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "PartitionId={0}", this.PartitionId));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "ErrorCode={0}", this.ErrorCode));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "TimeStampUtc={0}", this.TimeStampUtc));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "Message={0}", this.Message == null ? "" : this.Message));

            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "OperationId={0}", this.OperationId));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "BackupId={0}", this.BackupId));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "BackupLocation={0}", this.BackupLocation));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "EpochOfLastBackupRecord={0}", this.EpochOfLastBackupRecord));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "LsnOfLastBackupRecord={0}", this.LsnOfLastBackupRecord));

            return sb.ToString();
        }

        #region Interop Helpers

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeBackupOperationResult = new NativeBackupRestoreTypes.FABRIC_BACKUP_OPERATION_RESULT
            {
                ServiceName = pinCollection.AddObject(this.ServiceName),
                PartitionId = this.PartitionId,
                ErrorCode = this.ErrorCode,
                TimeStampUtc = NativeTypes.ToNativeFILETIME(this.TimeStampUtc),
                Message = pinCollection.AddObject(this.Message),
                OperationId = this.OperationId,
                Reserved = IntPtr.Zero,
            };

            // Copy backup details in case of success
            if (this.ErrorCode == 0)
            {
                NativeTypes.FABRIC_EPOCH nativeEpoch;

                nativeBackupOperationResult.BackupId = this.BackupId;
                nativeBackupOperationResult.BackupLocation = pinCollection.AddObject(this.BackupLocation);
                nativeBackupOperationResult.LsnOfLastBackupRecord = this.LsnOfLastBackupRecord;

                this.EpochOfLastBackupRecord.ToNative(out nativeEpoch);

                unsafe
                {
                    nativeBackupOperationResult.EpochOfLastBackupRecord = nativeEpoch;
                }
            }

            return pinCollection.AddBlittable(nativeBackupOperationResult);
        }

        internal static unsafe BackupOperationResult FromNative(IntPtr pointer)
        {
            var nativeBackupOperationResult = *(NativeBackupRestoreTypes.FABRIC_BACKUP_OPERATION_RESULT*)pointer;
            var managedResult = new BackupOperationResult
            {
                PartitionId = nativeBackupOperationResult.PartitionId,
                ServiceName = NativeTypes.FromNativeString(nativeBackupOperationResult.ServiceName),
                ErrorCode = nativeBackupOperationResult.ErrorCode,
                TimeStampUtc = NativeTypes.FromNativeFILETIME(nativeBackupOperationResult.TimeStampUtc),
                Message = NativeTypes.FromNativeString(nativeBackupOperationResult.Message),
                OperationId = nativeBackupOperationResult.OperationId,
            };

            // Copy backup details in case of success
            if (managedResult.ErrorCode == 0)
            {
                managedResult.BackupId = nativeBackupOperationResult.BackupId;
                managedResult.BackupLocation = NativeTypes.FromNativeString(nativeBackupOperationResult.BackupLocation);
                managedResult.EpochOfLastBackupRecord = Epoch.FromNative(nativeBackupOperationResult.EpochOfLastBackupRecord);
                managedResult.LsnOfLastBackupRecord = nativeBackupOperationResult.LsnOfLastBackupRecord;
            }

            return managedResult;
        }

        #endregion
    }
}