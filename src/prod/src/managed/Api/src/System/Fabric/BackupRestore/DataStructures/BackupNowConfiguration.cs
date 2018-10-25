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
    /// Backup Now configuration
    /// </summary>
    internal class BackupNowConfiguration
    {
        private const string TraceComponent = "BackupNowConfiguration";

        /// <summary>
        /// Overall operation timeout in milliseconds
        /// </summary>
        public int OperationTimeoutMilliseconds { get; set; }

        /// <summary>
        /// Store details where to backup
        /// </summary>
        public BackupStoreInformation StoreInformation { get; set; }

        /// <summary>
        /// Gets a string representation of the BackupNowConfiguration object.
        /// </summary>
        /// <returns>A string representation of the BackupNowConfiguration object.</returns>
        public override string ToString()
        {
            var sb = new StringBuilder();
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "OperationTimeoutMilliseconds={0}", OperationTimeoutMilliseconds));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "StoreInformation:"));
            sb.AppendLine(StoreInformation.ToString());

            return sb.ToString();
        }

        #region Interop Helpers

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeBackupConfiguration = new NativeBackupRestoreTypes.FABRIC_BACKUP_CONFIGURATION
            {
                OperationTimeoutMilliseconds = (uint)OperationTimeoutMilliseconds,
                StoreInformation = StoreInformation.ToNative(pinCollection),
                Reserved = IntPtr.Zero,
            };

            return pinCollection.AddBlittable(nativeBackupConfiguration);
        }

        internal static unsafe BackupNowConfiguration FromNative(IntPtr pointer)
        {
            var nativeBackupConfiguration = *(NativeBackupRestoreTypes.FABRIC_BACKUP_CONFIGURATION*)pointer;
            return new BackupNowConfiguration
            {
                OperationTimeoutMilliseconds = (int)nativeBackupConfiguration.OperationTimeoutMilliseconds,
                StoreInformation = BackupStoreInformation.FromNative(nativeBackupConfiguration.StoreInformation),
            };
        }

        #endregion
    }
}