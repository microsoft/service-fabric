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
    internal class AzureBlobBackupStorageInfo : BackupStorage
    {
        [DataMember]
        internal string ConnectionString { get; private set; }

        [DataMember]
        internal string ContainerName { get; private set; }

        [DataMember]
        internal bool IsConnectionStringEncrypted { get; private set; }

        internal AzureBlobBackupStorageInfo() : base(Enums.BackupStorageType.AzureBlobStore, '/')
        {
        }

        internal AzureBlobBackupStorageInfo(AzureBlobBackupStorageInfo other)
            : this()
        {
            this.ConnectionString = other.ConnectionString;
            this.IsConnectionStringEncrypted = other.IsConnectionStringEncrypted;
            this.ContainerName = other.ContainerName;
        }

        internal override BackupStorage DeepCopy()
        {
            return new AzureBlobBackupStorageInfo(this);
        }

        internal static AzureBlobBackupStorageInfo FromAzureBlobBackupStorageInfoView(
            BackupRestoreView.AzureBlobBackupStorageInfo azureBlobBackupStorageInfoView)
        {
            var azureBlobBackupStorageInfo = new AzureBlobBackupStorageInfo
            {
                ContainerName = azureBlobBackupStorageInfoView.ContainerName
            };

            string certThumbprint, certStore;
            EncryptionCertConfigHandler.GetEncryptionCertDetails(out certThumbprint, out certStore);

            // Encrypt the creds if cert configured
            if (!String.IsNullOrEmpty(certThumbprint))
            {
                azureBlobBackupStorageInfo.ConnectionString = EncryptionUtility.EncryptText(azureBlobBackupStorageInfoView.ConnectionString,
                    certThumbprint,
                    certStore);
                azureBlobBackupStorageInfo.IsConnectionStringEncrypted = true;
            }
            else
            {
                azureBlobBackupStorageInfo.ConnectionString = azureBlobBackupStorageInfoView.ConnectionString;
                azureBlobBackupStorageInfo.IsConnectionStringEncrypted = false;
            }

            return azureBlobBackupStorageInfo;
        }

        public static bool CompareStorage(AzureBlobBackupStorageInfo azureBlobBackupStorageInfo, AzureBlobBackupStorageInfo azureBlobBackupStorageInfo1)
        {
            if (azureBlobBackupStorageInfo.ConnectionString == azureBlobBackupStorageInfo1.ConnectionString &&
                azureBlobBackupStorageInfo.ContainerName == azureBlobBackupStorageInfo1.ContainerName)
            {
                return true;
            }
            return false;
        }

        internal BackupRestoreView.AzureBlobBackupStorageInfo ToAzureBlobBackupStorageInfoView()
        {
            var azureBlobBackupStorageInfoView =
                new BackupRestoreView.AzureBlobBackupStorageInfo
                {
                    ConnectionString = "****",                      // Not returning back the connection string
                    ContainerName = this.ContainerName
                };
            return azureBlobBackupStorageInfoView;
        }

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("ContainerName {0}", this.ContainerName).AppendLine();
            stringBuilder.AppendFormat("IsKeyEncrypted {0}", this.IsConnectionStringEncrypted).AppendLine();
            return stringBuilder.ToString();
        }

        public override bool Equals(object obj)
        {
            AzureBlobBackupStorageInfo azureBlobBackupStorageInfo = obj as AzureBlobBackupStorageInfo;
            if (this.ConnectionString.Equals(azureBlobBackupStorageInfo.ConnectionString))
            {
                return true;
            }
            return false;
        }

        public override int GetHashCode()
        {
            return this.ConnectionString.GetHashCode();
        }

        internal Builder ToBuilder()
        {
            return new Builder(this);
        }

        internal class Builder
        {
            private AzureBlobBackupStorageInfo tempObject;

            internal Builder(AzureBlobBackupStorageInfo originalObject)
            {
                this.tempObject = new AzureBlobBackupStorageInfo(originalObject);
            }

            internal Builder WithConnectionString(string value)
            {
                tempObject.ConnectionString = value;
                return this;
            }

            internal Builder WithContainerName(string value)
            {
                tempObject.ContainerName = value;
                return this;
            }

            internal Builder SetIsConnectionStringEncrypted(bool value)
            {
                tempObject.IsConnectionStringEncrypted = value;
                return this;
            }

            internal AzureBlobBackupStorageInfo Build()
            {
                var updatedObject = tempObject;
                tempObject = null;     // So that build cannot be invoked again, not an ideal solution but works for us
                return updatedObject;
            }
        }
    }
}