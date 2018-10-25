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
    /// Structure to post restore operation result back to the Backup Restore service
    /// </summary>
    internal class RestoreOperationResult
    {
        private const string TraceComponent = "RestoreOperationResult";

        /// <summary>
        /// Name of the service in fabric:/AppName/SvcName format
        /// </summary>
        public string ServiceName { get; set; }

        /// <summary>
        /// Partition ID Guid
        /// </summary>
        public Guid PartitionId { get; set; }

        /// <summary>
        /// Operation ID
        /// </summary>
        public Guid OperationId { get; set; }

        /// <summary>
        /// Status of this operation depicting using error code
        /// </summary>
        public HRESULT ErrorCode { get; set; }

        /// <summary>
        /// TimeStamp when operation succeded or failed
        /// </summary>
        public DateTime TimeStampUtc { get; set; }

        /// <summary>
        /// Operation result message
        /// </summary>
        public string Message { get; set; }

        /// <summary>
        /// Gets a string representation of the RestoreOperationResult object.
        /// </summary>
        /// <returns>A string representation of the RestoreOperationResult object.</returns>
        public override string ToString()
        {
            var sb = new StringBuilder();
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "ServiceName={0}", this.ServiceName));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "PartitionId={0}", this.PartitionId));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "OperationId={0}", this.OperationId));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "ErrorCode={0}", this.ErrorCode));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "TimeStampUtc={0}", this.TimeStampUtc));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "Message={0}", this.Message == null ? "" : this.Message));

            return sb.ToString();
        }

        #region Interop Helpers

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeRestoreOperationResult = new NativeBackupRestoreTypes.FABRIC_RESTORE_OPERATION_RESULT
            {
                ServiceName = pinCollection.AddObject(ServiceName),
                PartitionId = PartitionId,
                OperationId = OperationId,
                ErrorCode = ErrorCode,
                TimeStampUtc = NativeTypes.ToNativeFILETIME(TimeStampUtc),
                Message = pinCollection.AddObject(Message),
                Reserved = IntPtr.Zero,
            };

            return pinCollection.AddBlittable(nativeRestoreOperationResult);
        }

        internal static unsafe RestoreOperationResult FromNative(IntPtr pointer)
        {
            var nativeRestoreOperationResult = *(NativeBackupRestoreTypes.FABRIC_RESTORE_OPERATION_RESULT*)pointer;
            return new RestoreOperationResult
            {
                PartitionId = nativeRestoreOperationResult.PartitionId,
                OperationId = nativeRestoreOperationResult.OperationId,
                ServiceName = NativeTypes.FromNativeString(nativeRestoreOperationResult.ServiceName),
                ErrorCode = nativeRestoreOperationResult.ErrorCode,
                TimeStampUtc = NativeTypes.FromNativeFILETIME(nativeRestoreOperationResult.TimeStampUtc),
                Message = NativeTypes.FromNativeString(nativeRestoreOperationResult.Message),
            };
        }

        #endregion
    }
}