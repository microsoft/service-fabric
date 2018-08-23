// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.IO;
    using System.Linq;

    internal static class FileShareCommon
    {
        internal static bool GetDestinationPath(
                                FabricEvents.ExtensionsEvents traceSource,
                                string logSourceId,
                                ConfigReader configReader,
                                string sectionName,
                                out bool destinationIsLocalAppFolder,
                                out string destinationPath)
        {
            bool success = true;
            destinationIsLocalAppFolder = false;
            if (configReader.IsReadingFromApplicationManifest)
            {
                // Get the folder path from the application manifest
                string relativePath = configReader.GetUnencryptedConfigValue(
                                          sectionName,
                                          FileShareUploaderConstants.AppRelativeFolderPathParamName,
                                          string.Empty);
                string destinationPathAsUri = configReader.GetUnencryptedConfigValue(
                                                  sectionName,
                                                  FileShareUploaderConstants.AppDestinationPathParamName,
                                                  string.Empty);
                if (string.IsNullOrEmpty(relativePath) && string.IsNullOrEmpty(destinationPathAsUri))
                {
                    // Local folder
                    destinationIsLocalAppFolder = true;
                    destinationPath = configReader.GetApplicationLogFolder();
                }
                else if (false == string.IsNullOrEmpty(relativePath))
                {
                    // Sub-folder under local folder
                    destinationIsLocalAppFolder = true;
                    destinationPath = Path.Combine(
                                        configReader.GetApplicationLogFolder(),
                                        relativePath);
                }
                else
                {
                    // Remote folder
                    success = GetDestinationPathFromUri(
                                  traceSource,
                                  logSourceId,
                                  sectionName,
                                  destinationPathAsUri,
                                  out destinationPath);
                }
            }
            else
            {
                // Get the destination path from the cluster manifest
                string destinationPathAsUri = configReader.GetUnencryptedConfigValue(
                                                  sectionName,
                                                  FileShareUploaderConstants.StoreConnectionStringParamName,
                                                  string.Empty);
                destinationPathAsUri = Environment.ExpandEnvironmentVariables(destinationPathAsUri);
                if (string.IsNullOrWhiteSpace(destinationPathAsUri))
                {
                    destinationPath = string.Empty;
                    return false;
                }

                success = GetDestinationPathFromUri(
                              traceSource,
                              logSourceId,
                              sectionName,
                              destinationPathAsUri,
                              out destinationPath);
            }

            return success;
        }

        internal static bool GetAccessInfo(
                                FabricEvents.ExtensionsEvents traceSource,
                                string logSourceId,
                                ConfigReader configReader,
                                string sectionName,
                                out FileShareUploader.AccessInformation accessInfo)
        {
            accessInfo = new FileShareUploader.AccessInformation
            {
                AccountType = FileShareUploader.FileShareAccessAccountType.None
            };

            string accountType = configReader.GetUnencryptedConfigValue(
                                     sectionName,
                                     FileShareUploaderConstants.StoreAccessAccountType,
                                     string.Empty);
            if (string.IsNullOrEmpty(accountType))
            {
                // Access information has not been specified
                return true;
            }

            if (false == Enum.TryParse(
                            accountType,
                            out accessInfo.AccountType))
            {
                traceSource.WriteError(
                    logSourceId,
                    "Account type {0} in section {1} is not supported.",
                    accountType,
                    sectionName);
                return false;
            }

            string accountName = configReader.GetUnencryptedConfigValue(
                                     sectionName,
                                     FileShareUploaderConstants.StoreAccessAccountName,
                                     string.Empty);
            if (false == AccountHelper.TryParseUserAccountName(
                            accountName,
                            accessInfo.AccountType == FileShareUploader.FileShareAccessAccountType.ManagedServiceAccount,
                            out accessInfo.UserName,
                            out accessInfo.DomainName))
            {
                traceSource.WriteError(
                    logSourceId,
                    "Domain name and user name could not be obtained from the account name {0} in section {1}.",
                    accountName,
                    sectionName);
                return false;
            }

            if (FileShareUploader.FileShareAccessAccountType.ManagedServiceAccount == accessInfo.AccountType)
            {
                accessInfo.AccountPassword = NativeHelper.SERVICE_ACCOUNT_PASSWORD;
                accessInfo.AccountPasswordIsEncrypted = false;
            }
            else if (FileShareUploader.FileShareAccessAccountType.DomainUser == accessInfo.AccountType)
            {
                if (configReader.IsReadingFromApplicationManifest)
                {
                    // When reading from the application manifest, the config reader does
                    // cannot automatically figure out whether or not the value is encrypted.
                    // So we figure it out ourselves.
                    accessInfo.AccountPassword = configReader.GetUnencryptedConfigValue(
                                                     sectionName,
                                                     FileShareUploaderConstants.StoreAccessAccountPassword,
                                                     string.Empty);
                    string passwordEncrypted = configReader.GetUnencryptedConfigValue(
                                                     sectionName,
                                                     FileShareUploaderConstants.StoreAccessAccountPasswordIsEncrypted,
                                                     FileShareUploaderConstants.PasswordEncryptedDefaultValue);
                    if (false == bool.TryParse(
                                    passwordEncrypted,
                                    out accessInfo.AccountPasswordIsEncrypted))
                    {
                        traceSource.WriteError(
                            logSourceId,
                            "Unable to determine whether the account password in section {0} is encrypted because {1} could not be parsed as a boolean value.",
                            sectionName,
                            passwordEncrypted);
                        return false;
                    }
                }
                else
                {
                    accessInfo.AccountPassword = configReader.ReadString(
                                                    sectionName,
                                                    FileShareUploaderConstants.StoreAccessAccountPassword,
                                                    out accessInfo.AccountPasswordIsEncrypted);
                }
            }

            return true;
        }

        internal static bool ShouldUseOldWorkLocations(string logDirectory)
        {
            return FabricDirectory.GetDirectories(
                            logDirectory,
                            "*",
                            SearchOption.AllDirectories)
                        .Where(dirFullName =>
                        {
                            string dirName = Path.GetFileName(dirFullName);
                            return dirName.StartsWith(
                                        FileShareUploaderConstants.V2FileShareUploaderPrefix,
                                        StringComparison.OrdinalIgnoreCase);
                        }).Any();
        }

        private static bool GetDestinationPathFromUri(
                                FabricEvents.ExtensionsEvents traceSource,
                                string logSourceId,
                                string sectionName,
                                string destinationPathAsUri,
                                out string destinationPath)
        {
            destinationPath = string.Empty;
            try
            {
                Uri uri = new Uri(destinationPathAsUri);
                destinationPath = uri.LocalPath;
            }
            catch (UriFormatException e)
            {
                traceSource.WriteError(
                    logSourceId,
                    "The value '{0}' specified as the file share destination in section {1} could not be parsed as a URI. Exception information: {2}.",
                    destinationPathAsUri,
                    sectionName,
                    e);
                return false;
            }

            return true;
        }
    }
}