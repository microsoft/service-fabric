//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace AzureFilesVolumePlugin
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Description;
    using System.Fabric.Security;
    using System.IO;

    internal static class Utilities
    {
        internal static void EnsureFolder(string folderPath)
        {
            if (!Directory.Exists(folderPath))
            {
                Directory.CreateDirectory(folderPath);
            }
        }

        internal static ConfigurationSection GetConfigSection(ICodePackageActivationContext cpac)
        {
            var cpo = cpac.GetConfigurationPackageObject(Constants.ConfigurationPackageName);
            return cpo.Settings.Sections[Constants.SectionName];
        }

        internal static AzureFilesVolumePluginSupportedOs GetOperatingSystem(ICodePackageActivationContext cpac)
        {
            var section = GetConfigSection(cpac);
            return GetOperatingSystem(section);
        }

        internal static AzureFilesVolumePluginSupportedOs GetOperatingSystem(ConfigurationSection section)
        {
            var paramValue = section.Parameters[Constants.OsParameterName].Value;
            return (AzureFilesVolumePluginSupportedOs)Enum.Parse(typeof(AzureFilesVolumePluginSupportedOs), paramValue);
        }

        // TODO: Avoid returning a string because strings are immutable and hence we cannot clear out the
        // secret from memory when we are done using it.
        internal static string GetStorageAccountKey(Dictionary<string, string> options)
        {
            string key = options[Constants.StorageAccountKeyOption].Trim();
            bool keyIsEncrypted = options.ContainsKey(Constants.StorageAccountKeyIsEncryptedOption) &&
                Boolean.Parse(options[Constants.StorageAccountKeyIsEncryptedOption]);
            if (keyIsEncrypted)
            {
                key = EncryptionUtility.DecryptText(key).ToString();
            }
            return key;
        }

        // If StorageAccountFQDNOption is not in the options, use <StorageAccountName> if it represents FQDN or form the FQDN using the default suffix with the account name
        // Example: aftest.file.core.windows.net | aftest2.file.core.chinacloudapi.cn
        internal static string GetStorageAccountFQDN(Dictionary<string, string> options)
        {
            if (options.ContainsKey(Constants.StorageAccountFQDNOption) && !String.IsNullOrEmpty(options[Constants.StorageAccountFQDNOption].Trim()))
            {
                return options[Constants.StorageAccountFQDNOption].Trim();
            }
            else
            {
                string storageAccountName = options[Constants.StorageAccountNameOption].Trim();
                if (storageAccountName.Contains("."))
                {
                    return storageAccountName;
                }
                else
                {
                    return String.Concat(storageAccountName, Constants.DefaultFQDNSuffix);
                }
            }
        }

        internal static string GetStorageAccountName(Dictionary<string, string> options)
        {
            string storageAccountName = options[Constants.StorageAccountNameOption].Trim();
            if (storageAccountName.Contains("."))
            {
                return storageAccountName.Substring(0, storageAccountName.IndexOf("."));
            }
            else
            {
                return storageAccountName;
            }
        }
    }
}
