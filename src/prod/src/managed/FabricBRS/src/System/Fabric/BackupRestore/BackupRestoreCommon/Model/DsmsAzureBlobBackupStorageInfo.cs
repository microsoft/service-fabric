// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common.Model
{
    using System.Text;
    using System.Runtime.Serialization;
    using BackupRestoreView = BackupRestoreTypes;
    using System.Fabric.BackupRestore.Service;
    using System.Fabric.Security;

    [DataContract]
    internal class DsmsAzureBlobBackupStorageInfo : BackupStorage
    {
        /// <summary>
        /// Source location of the storage credentials.
        /// </summary>
        [DataMember]
        internal string StorageCredentialsSourceLocation { get; private set; }

        [DataMember]
        internal string ContainerName { get; private set; }

        internal DsmsAzureBlobBackupStorageInfo() : base(Enums.BackupStorageType.DsmsAzureBlobStore, '/')
        {
        }

        internal DsmsAzureBlobBackupStorageInfo(DsmsAzureBlobBackupStorageInfo other)
            : this()
        {
            this.StorageCredentialsSourceLocation = other.StorageCredentialsSourceLocation;
            this.ContainerName = other.ContainerName;
        }

        internal override BackupStorage DeepCopy()
        {
            return new DsmsAzureBlobBackupStorageInfo(this);
        }

        internal static DsmsAzureBlobBackupStorageInfo FromDsmsAzureBlobBackupStorageInfoView(
            BackupRestoreView.DsmsAzureBlobBackupStorageInfo DsmsAzureBlobBackupStorageInfoView)
        {
            var DsmsAzureBlobBackupStorageInfo = new DsmsAzureBlobBackupStorageInfo
            {
                ContainerName = DsmsAzureBlobBackupStorageInfoView.ContainerName,
                StorageCredentialsSourceLocation = DsmsAzureBlobBackupStorageInfoView.StorageCredentialsSourceLocation
            };

            return DsmsAzureBlobBackupStorageInfo;
        }

        internal BackupRestoreView.DsmsAzureBlobBackupStorageInfo ToDsmsAzureBlobBackupStorageInfoView()
        {
            var DsmsAzureBlobBackupStorageInfoView =
                new BackupRestoreView.DsmsAzureBlobBackupStorageInfo
                {
                    ContainerName = this.ContainerName,
                    StorageCredentialsSourceLocation = this.StorageCredentialsSourceLocation
                };

            return DsmsAzureBlobBackupStorageInfoView;
        }

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("StorageCredentialsSourceLocation : {0}", this.StorageCredentialsSourceLocation).AppendLine();
            stringBuilder.AppendFormat("ContainerName : {0}", this.ContainerName).AppendLine();
            return stringBuilder.ToString();
        }

        public static bool CompareStorage(DsmsAzureBlobBackupStorageInfo azureBlobBackupStorageInfo, DsmsAzureBlobBackupStorageInfo azureBlobBackupStorageInfo1)
        {
            if (azureBlobBackupStorageInfo.StorageCredentialsSourceLocation == azureBlobBackupStorageInfo1.StorageCredentialsSourceLocation &&
                azureBlobBackupStorageInfo.ContainerName == azureBlobBackupStorageInfo1.ContainerName)
            {
                return true;
            }
            return false;
        }

        public override bool Equals(object obj)
        {
            DsmsAzureBlobBackupStorageInfo DsmsAzureBlobBackupStorageInfo = obj as DsmsAzureBlobBackupStorageInfo;
            if (this.StorageCredentialsSourceLocation.Equals(DsmsAzureBlobBackupStorageInfo.StorageCredentialsSourceLocation))
            {
                return true;
            }
            return false;
        }

        public override int GetHashCode()
        {
            return this.StorageCredentialsSourceLocation.GetHashCode();
        }

        internal Builder ToBuilder()
        {
            return new Builder(this);
        }

        internal class Builder
        {
            private DsmsAzureBlobBackupStorageInfo tempObject;

            internal Builder(DsmsAzureBlobBackupStorageInfo originalObject)
            {
                this.tempObject = new DsmsAzureBlobBackupStorageInfo(originalObject);
            }

            internal Builder WithStorageCredentialsSourceLocation(string value)
            {
                tempObject.StorageCredentialsSourceLocation = value;
                return this;
            }

            internal Builder WithContainerName(string value)
            {
                tempObject.ContainerName = value;
                return this;
            }

            internal DsmsAzureBlobBackupStorageInfo Build()
            {
                var updatedObject = tempObject;
                tempObject = null;     // So that build cannot be invoked again, not an ideal solution but works for us
                return updatedObject;
            }
        }
    }
}