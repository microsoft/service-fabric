// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common.Model
{
    using System.Text;
    using System.Runtime.Serialization;
    using System.Fabric.Security;
    using System.Fabric.BackupRestore.Service;
    using BackupRestoreView = BackupRestoreTypes;

    [DataContract]
    internal class FileShareBackupStorageInfo : BackupStorage
    {
        [DataMember]
        internal string Path { get; private set; }

        [DataMember]
        internal string PrimaryUserName { get; private set; }

        [DataMember]
        internal string PrimaryPassword { get; private set; }

        [DataMember]
        internal bool IsPasswordEncrypted { get; private set; }

        [DataMember]
        internal string SecondaryUserName { get; private set; }

        [DataMember]
        internal string SecondaryPassword { get; private set; }

        internal FileShareBackupStorageInfo() : base(Enums.BackupStorageType.FileShare, IO.Path.DirectorySeparatorChar)
        {
        }

        internal FileShareBackupStorageInfo(FileShareBackupStorageInfo other)
            : this()
        {
            this.Path = other.Path;
            this.PrimaryUserName = other.PrimaryUserName;
            this.PrimaryPassword = other.PrimaryPassword;
            this.SecondaryUserName = other.SecondaryUserName;
            this.SecondaryPassword = other.SecondaryPassword;
            this.IsPasswordEncrypted = other.IsPasswordEncrypted;
        }

        internal override BackupStorage DeepCopy()
        {
            return new FileShareBackupStorageInfo(this);
        }

        internal static FileShareBackupStorageInfo FromfileShareBackupStorageInfoView(
            BackupRestoreView.FileShareBackupStorageInfo fileShareBackupStorageInfoView)
        {
            var fileShareBackupStorageInfo = new FileShareBackupStorageInfo
            {
                Path = fileShareBackupStorageInfoView.Path,
                PrimaryUserName = fileShareBackupStorageInfoView.PrimaryUserName,
                SecondaryUserName = fileShareBackupStorageInfoView.SecondaryUserName
            };

            string certThumbprint, certStore;
            EncryptionCertConfigHandler.GetEncryptionCertDetails(out certThumbprint, out certStore);

            // Encrypt the creds if cert configured
            if (!String.IsNullOrEmpty(certThumbprint))
            {
                if (!String.IsNullOrEmpty(fileShareBackupStorageInfoView.PrimaryPassword))
                {
                    fileShareBackupStorageInfo.PrimaryPassword = EncryptionUtility.EncryptText(fileShareBackupStorageInfoView.PrimaryPassword,
                        certThumbprint,
                        certStore);
                }
                
                if (!String.IsNullOrEmpty(fileShareBackupStorageInfoView.SecondaryPassword))
                {
                    fileShareBackupStorageInfo.SecondaryPassword =
                        EncryptionUtility.EncryptText(fileShareBackupStorageInfoView.SecondaryPassword, certThumbprint,
                            certStore);
                }

                fileShareBackupStorageInfo.IsPasswordEncrypted = true;
            }
            else
            {
                fileShareBackupStorageInfo.PrimaryPassword = fileShareBackupStorageInfoView.PrimaryPassword;
                fileShareBackupStorageInfo.SecondaryPassword = fileShareBackupStorageInfoView.SecondaryPassword;
                fileShareBackupStorageInfo.IsPasswordEncrypted = false;
            }
            
            return fileShareBackupStorageInfo;
        }

        internal BackupRestoreView.FileShareBackupStorageInfo TofileShareBackupStorageInfoView()
        {
            var fileShareBackupStorageInfoView = new BackupRestoreView.FileShareBackupStorageInfo
            {
                Path = this.Path,
                PrimaryUserName = this.PrimaryUserName,
                // [SuppressMessage("Microsoft.Security", "CS002:SecretInNextLine", Justification="Asterisks")]
                PrimaryPassword = "****",                   // Not returning back password
                SecondaryUserName = this.SecondaryUserName,
                // [SuppressMessage("Microsoft.Security", "CS002:SecretInNextLine", Justification="Asterisks")]
                SecondaryPassword = "****"                  // Not returning back password
            };
            return fileShareBackupStorageInfoView;
        }

        public static bool CompareStorage(FileShareBackupStorageInfo fileShareBackupStorageInfo, FileShareBackupStorageInfo fileShareBackupStorageInfo1)
        {
            if (fileShareBackupStorageInfo.Path== fileShareBackupStorageInfo1.Path)
            {
                return true;
            }
            return false;
        }

        internal Builder ToBuilder()
        {
            return new Builder(this);
        }

        internal class Builder
        {
            private FileShareBackupStorageInfo tempObject;

            internal Builder(FileShareBackupStorageInfo originalObject)
            {
                this.tempObject = new FileShareBackupStorageInfo(originalObject);
            }

            internal Builder WithPrimaryPassword(string value)
            {
                tempObject.PrimaryPassword = value;
                return this;
            }

            internal Builder WithSecondaryPassword(string value)
            {
                tempObject.SecondaryPassword = value;
                return this;
            }

            internal Builder SetIsPasswordEncrypted(bool value)
            {
                tempObject.IsPasswordEncrypted = value;
                return this;
            }

            internal FileShareBackupStorageInfo Build()
            {
                var updatedObject = tempObject;
                tempObject = null;     // So that build cannot be invoked again, not an ideal solution but works for us
                return updatedObject;
            }
        }

        #region Public Overrides

        public override string ToString()
        {
            var stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("Path {0}", this.Path).AppendLine();
            stringBuilder.AppendFormat("Primary UserName {0}", this.PrimaryUserName).AppendLine();
            stringBuilder.AppendFormat("Secondary UserName {0}", this.SecondaryUserName).AppendLine();
            return stringBuilder.ToString();
        }

        public override bool Equals(object obj)
        {
            var fileShareStorage = obj as FileShareBackupStorageInfo;
            return this.Path.Equals(fileShareStorage?.Path);
        }

        public override int GetHashCode()
        {
            return this.Path.GetHashCode();
        }

        #endregion
    }

    
}