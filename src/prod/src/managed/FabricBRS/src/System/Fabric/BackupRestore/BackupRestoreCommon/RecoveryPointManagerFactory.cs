// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.BackupRestore.Enums;

namespace System.Fabric.BackupRestore.Common
{
    /// <summary>
    /// Factory class to manage the various supported recovery point managers
    /// </summary>
    internal static class RecoveryPointManagerFactory
    {
        /// <summary>
        /// Gets the store manager as per the policy
        /// </summary>
        /// <param name="storeInformation">Store information from the backup schedule policy</param>
        /// <returns>Returns IRecoveryPointManager instance</returns>
        public static IRecoveryPointManager GetRecoveryPointManager(Model.BackupStorage storeInformation)
        {
            switch (storeInformation.BackupStorageType)
            {
                case BackupStorageType.AzureBlobStore:
                    return new AzureBlobStoreRecoveryPointManager(storeInformation as Model.AzureBlobBackupStorageInfo);
                case BackupStorageType.DsmsAzureBlobStore:
                    return new DsmsAzureBlobStoreRecoveryPointManager(storeInformation as Model.DsmsAzureBlobBackupStorageInfo);
                case BackupStorageType.FileShare:
                    return new FileShareRecoveryPointManager(storeInformation as Model.FileShareBackupStorageInfo);
                default:
                    throw new ArgumentException("StoreType");
            }
        }
    }
}