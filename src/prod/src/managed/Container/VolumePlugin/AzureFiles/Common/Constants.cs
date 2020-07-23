//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace AzureFilesVolumePlugin
{
    static class Constants
    {
        internal const string ConfigurationPackageName = "Config";
        internal const string SectionName = "AzureFilesVolumePlugin";
        internal const string OsParameterName = "OperatingSystem";
        internal const string HttpEndpointName = "ServiceHttpEndpoint";
        internal const string Mounts = "mounts";
        internal const string ShareNameOption = "shareName";
        internal const string StorageAccountNameOption = "storageAccountName";
        internal const string StorageAccountKeyOption = "storageAccountKey";
        internal const string StorageAccountFQDNOption = "storageAccountFQDN";
        internal const string StorageAccountKeyIsEncryptedOption = "storageAccountKeyIsEncrypted";
        internal const string DefaultFQDNSuffix = ".file.core.windows.net";
    }
}
