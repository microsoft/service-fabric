// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common
{
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Blob;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.BackupRestore.BackupRestoreTypes;
    using System.Fabric.BackupRestore.Enums;
    using System.Fabric.Common;
    using System.Fabric.Security;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Represents dSMS based Azure Blob Storage based recovery point store.
    /// </summary>
    internal class DsmsAzureBlobRecoveryPointStore : AzureBlobRecoveryPointStoreBase
    {
        private readonly Model.DsmsAzureBlobBackupStorageInfo _storeInformation;

        private object dsmsStorageHelper;
        private MethodInfo getStorageAccountMethodInfo;

        public DsmsAzureBlobRecoveryPointStore(Model.DsmsAzureBlobBackupStorageInfo dsmsAzureBlobStoreInformation) : base(dsmsAzureBlobStoreInformation)
        {
            this._storeInformation = dsmsAzureBlobStoreInformation;
            this.InitializeDsmsStorageHelper(this._storeInformation.StorageCredentialsSourceLocation);
            CloudStorageAccount cloudStorageAccount = (CloudStorageAccount)this.getStorageAccountMethodInfo.Invoke(this.dsmsStorageHelper, null);

            this.container = AzureBlobStoreHelper.GetContainer(cloudStorageAccount, this._storeInformation.ContainerName);
        }

        internal void InitializeDsmsStorageHelper(string sourceLocation)
        {
            var currentAssembly = typeof(DsmsAzureBlobRecoveryPointStore).GetTypeInfo().Assembly;

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
    }
}