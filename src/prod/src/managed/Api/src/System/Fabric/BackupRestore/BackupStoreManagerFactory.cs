// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.BackupRestore.DataStructures;

namespace System.Fabric.BackupRestore
{
    /// <summary>
    /// Factory class to manage the various supported backup store managers
    /// </summary>
    internal static class BackupStoreManagerFactory
    {
        /// <summary>
        /// Gets the store manager as per the policy
        /// </summary>
        /// <param name="storeInformation">Store information from the backup schedule policy</param>
        /// <returns>Returns IBackupStoreManager instance</returns>
        public static IBackupStoreManager GetStoreManager(BackupStoreInformation storeInformation)
        {
            switch (storeInformation.StoreType)
            {
                case BackupStoreType.AzureStorage:
                    return new AzureBlobBackupStoreManager(storeInformation as AzureBlobBackupStore);
                case BackupStoreType.FileShare:
                    return new FileShareBackupStoreManager(storeInformation as FileShareBackupStore);
                case BackupStoreType.DsmsAzureStorage:
                    return new DsmsAzureBlobBackupStoreManager(storeInformation as DsmsAzureBlobBackupStore);
                default:
                    throw new ArgumentException("StoreType");
            }
        }
    }
}