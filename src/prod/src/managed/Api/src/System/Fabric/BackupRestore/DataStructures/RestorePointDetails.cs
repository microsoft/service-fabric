// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Collections.Generic;
using System.Fabric.BackupRestore.Interop;
using System.Fabric.Interop;
using System.Globalization;
using System.Text;

namespace System.Fabric.BackupRestore.DataStructures
{
    internal class RestorePointDetails
    {
        /// <summary>
        /// Operation ID
        /// </summary>
        public Guid OperationId { get; set; }
        /// <summary>
        /// List of backup Locations (comprising of full and incremental backups)
        /// </summary>
        public List<string> BackupLocations { get; set; }

        /// <summary>
        /// Store details
        /// </summary>
        public BackupStoreInformation StoreInformation { get; set; }

        /// <summary>
        /// Whether the restore operation was user initiated or not
        /// </summary>
        public bool UserInitiatedOperation { get; set; }

        /// <summary>
        /// Gets a string representation of the recovery point details object
        /// </summary>
        /// <returns>A string represenation of the recovery point details object</returns>
        public override string ToString()
        {
            var sb = new StringBuilder();
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "OperationId={0}", this.OperationId));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "BackupLocations={0}", String.Join(", ", this.BackupLocations)));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "UserInitiatedOperation={0}", this.UserInitiatedOperation));
            sb.AppendLine("StoreInformation:");
            sb.AppendLine(StoreInformation.ToString());

            return sb.ToString();
        }

        #region Interop Helpers

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeRestorePointDetails = new NativeBackupRestoreTypes.FABRIC_RESTORE_POINT_DETAILS
            {
                OperationId = OperationId,
                BackupLocations = NativeTypes.ToNativeStringList(pinCollection, BackupLocations),
                StoreInformation = StoreInformation.ToNative(pinCollection),
                UserInitiatedOperation = NativeTypes.ToBOOLEAN(UserInitiatedOperation),
                Reserved = IntPtr.Zero,
            };

            return pinCollection.AddBlittable(nativeRestorePointDetails);
        }

        internal static unsafe RestorePointDetails FromNative(IntPtr pointer)
        {
            var nativeRestorePointDetails = *(NativeBackupRestoreTypes.FABRIC_RESTORE_POINT_DETAILS*)pointer;
            var managedRecoveryPointDetails = new RestorePointDetails
            {
                OperationId = nativeRestorePointDetails.OperationId,
                UserInitiatedOperation = NativeTypes.FromBOOLEAN(nativeRestorePointDetails.UserInitiatedOperation),
                StoreInformation = BackupStoreInformation.FromNative(nativeRestorePointDetails.StoreInformation),
            };

            var nativeBackupLocations = *(NativeTypes.FABRIC_STRING_LIST*)nativeRestorePointDetails.BackupLocations;
            managedRecoveryPointDetails.BackupLocations = NativeTypes.FromNativeStringList(nativeBackupLocations);
            return managedRecoveryPointDetails;
        }

        #endregion
    }
}