// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupCopier
{
    internal static class StringConstants
    {
        // Command Line Arguments Key
        public const string OperationKeyName = "/operation";

        public const string StorageCredentialsSourceLocationKeyName = "/storageCredentialsSourceLocation";
        public const string ConnectionStringKeyName = "/connectionString";
        public const string IsConnectionStringEncryptedKeyName = "/isConnectionStringEncrypted";
        public const string ContainerNameKeyName = "/containerName";
        public const string BackupStoreBaseFolderPathKeyName = "/backupStoreBaseFolderPath";

        public const string FileSharePathKeyName = "/fileSharePath";
        public const string FileShareAccessTypeKeyName = "/fileShareAccessType";
        public const string IsPasswordEncryptedKeyName = "/isPasswordEncrypted";
        public const string PrimaryUserNameKeyName = "/primaryUserName";
        public const string PrimaryPasswordKeyName = "/primaryPassword";
        public const string SecondaryUserNameKeyName = "/secondaryUserName";
        public const string SecondaryPasswordKeyName = "/secondaryPassword";

        public const string FileShareAccessTypeValue_None = "None";
        public const string FileShareAccessTypeValue_DomainUser = "DomainUser";

        public const string BooleanStringValue_True = "true";
        public const string BooleanStringValue_False = "false";

        public const string SourceFileOrFolderPathKeyName = "/sourceFileOrFolderPath";
        public const string TargetFolderPathKeyName = "/targetFolderPath";
        public const string BackupMetadataFilePathKeyName = "/backupMetadataFilePath";

        public const string Timeout = "/timeout";
        public const string WorkingDir = "/workingDir";
        public const string ProgressFile = "/progressFile";
        public const string ErrorDetailsFile = "/errorDetailsFile";

        // Command Line Arguments Values for operation command
        public const string AzureBlobStoreUpload = "AzureBlobStoreUpload";
        public const string AzureBlobStoreDownload = "AzureBlobStoreDownload";
        public const string DsmsAzureBlobStoreUpload = "DsmsAzureBlobStoreUpload";
        public const string DsmsAzureBlobStoreDownload = "DsmsAzureBlobStoreDownload";
        public const string FileShareUpload = "FileShareUpload";
        public const string FileShareDownload = "FileShareDownload";

        public const string ZipExtension = ".zip";
    }
}