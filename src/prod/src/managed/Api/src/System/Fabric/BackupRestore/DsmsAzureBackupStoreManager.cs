// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore
{
    using System.Fabric.BackupRestore;
    using System.Fabric.BackupRestore.DataStructures;
    using System.Fabric.Common;
    using System.Reflection;

    using Microsoft.WindowsAzure.Storage;

    /// <summary>
    /// Azure blob BackupStoreManager
    /// </summary>
    internal class DsmsAzureBlobBackupStoreManager : AzureBlobBackupStoreManagerBase
    {
        private const string TraceType = "DsmsAzureBlobBackupStore";

        private readonly DsmsAzureBlobBackupStore dsmsAzureBlobBackupStore;

        private object dsmsStorageHelper;

        private MethodInfo getStorageAccountMethodInfo;

        public DsmsAzureBlobBackupStoreManager(DsmsAzureBlobBackupStore dsmsAzureBlobBackupStore) : base (TraceType)
        {
            this.dsmsAzureBlobBackupStore = dsmsAzureBlobBackupStore;
            this.InitializeDsmsStorageHelper(dsmsAzureBlobBackupStore.StorageCredentialsSourceLocation);
        }

        #region Base class abstract methods
        protected override string GetContainerName()
        {
            return this.dsmsAzureBlobBackupStore.ContainerName;
        }

        protected override string GetFolderPath()
        {
            return this.dsmsAzureBlobBackupStore.FolderPath;
        }

        protected override CloudStorageAccount GetCloudStorageAccount()
        {
            CloudStorageAccount cloudStorageAccount = (CloudStorageAccount)this.getStorageAccountMethodInfo.Invoke(this.dsmsStorageHelper, null);
            return cloudStorageAccount;
        }

        internal void InitializeDsmsStorageHelper(string sourceLocation)
        {
            var currentAssembly = typeof(DsmsAzureBlobBackupStoreManager).GetTypeInfo().Assembly;

            var systemFabricBackupRestoreDsmsAssembly = new AssemblyName
            {
                Name = BackupRestoreContants.SystemFabricBackupRestoreDsmsAssemblyName,
                Version = currentAssembly.GetName().Version,
#if !DotNetCoreClr
                CultureInfo = currentAssembly.GetName().CultureInfo,
#endif
                ProcessorArchitecture = currentAssembly.GetName().ProcessorArchitecture
            };

            systemFabricBackupRestoreDsmsAssembly.SetPublicKeyToken(currentAssembly.GetName().GetPublicKeyToken());

            var dsmsAzureStorageHelperTypeName = Helpers.CreateQualifiedNameForAssembly(
                systemFabricBackupRestoreDsmsAssembly.FullName,
                BackupRestoreContants.DsmsAzureStorageHelperClassFullName);

            var dsmsAzureStorageHelperType = Type.GetType(dsmsAzureStorageHelperTypeName, true);
            this.dsmsStorageHelper = Activator.CreateInstance(dsmsAzureStorageHelperType, sourceLocation);
            this.getStorageAccountMethodInfo = dsmsAzureStorageHelperType.GetMethod(BackupRestoreContants.DsmsGetStorageAccountMethodName);
        }
        #endregion
    }
}