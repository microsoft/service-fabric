// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Test
{
    using System.Fabric.BackupRestore;
    using System.Fabric.BackupRestore.DataStructures;

    /// <summary>
    /// Test code.
    /// </summary>
    public class Program
    {
        /// <summary>
        /// Main method.
        /// </summary>
        /// <param name="args"></param>
        /// <returns></returns>
        public static int Main(string[] args)
        {
            AzureBlobBackupStore azBlobBackupStore = new AzureBlobBackupStore();
            azBlobBackupStore.ConnectionString = "<To Be Populated>";
            azBlobBackupStore.ContainerName = "test";
            azBlobBackupStore.FolderPath = "TestFolder/pqr";
            azBlobBackupStore.IsAccountKeyEncrypted = false;

           IBackupStoreManager storeManager = BackupStoreManagerFactory.GetStoreManager(azBlobBackupStore);

            //AzureBlobBackupStoreManager storeManager = new AzureBlobBackupStoreManager(azBlobBackupStore);

            storeManager.Upload(@"D:\SfDev\BkpRestore\1e32b319-fe90-4bce-b8de-eda0bb1fb726", "1e32b319-fe90-4bce-b8de-eda0bb1fb726");
            storeManager.Upload(@"D:\SfDev\BkpRestore\1e32b319-fe90-4bce-b8de-eda0bb1fb726\2017-06-27 16.12.39.bkmetadata", "1e32b319-fe90-4bce-b8de-eda0bb1fb726-1", true);
            storeManager.Download("1e32b319-fe90-4bce-b8de-eda0bb1fb726", @"H:\1e32b319-fe90-4bce-b8de-eda0bb1fb726");
            storeManager.Download("1e32b319-fe90-4bce-b8de-eda0bb1fb726/2017-06-27 16.12.39.bkmetadata", @"H:\1e32b319-fe90-4bce-b8de-eda0bb1fb726-1", true);

            return 0;
        }
    }
}