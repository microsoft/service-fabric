// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.ImageStore
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Security.Principal;

    /// <summary>
    /// This is the factory class to produce different types of image stores.
    /// </summary>
    internal class ImageStoreFactory
    {
        /// <summary>
        /// Create a new image store.
        /// </summary>
        /// <param name="imageStoreUri">The uri of the image store.</param>
        /// <param name="workingDirectory">The temporary working directory.</param>
        /// <param name="localRoot">The local path of the working folder.</param>
        /// <param name="isInternal">Indicates whether or not it's for internal use.</param>
        /// <returns>An object of Image Store pointing to the given uri.</returns>
        internal static IImageStore CreateImageStore(
            string imageStoreUri,            
            string localRoot = null,             
            string workingDirectory = null,
            bool isInternal = false)
        {
            if (imageStoreUri == null)
            {
                throw new ArgumentException(StringResources.Error_InvalidImageStoreConnectionString);
            }

            string[] connectionStrings = null;
            SecurityCredentials credentials = null;

            return CreateImageStore(imageStoreUri, localRoot, connectionStrings, credentials, workingDirectory, isInternal);
        }

        internal static IImageStore CreateImageStore(
            string imageStoreUri,            
            string localRoot,
            string[] connectionStrings,
            SecurityCredentials credentials,
            string workingDirectory,
            bool isInternal)
        {
            if (imageStoreUri == null)
            {
                throw new ArgumentException(StringResources.Error_InvalidImageStoreConnectionString);
            }

            if (FileImageStore.IsFileStoreUri(imageStoreUri))
            {
                string rootUri;
                string accountName, accountPassword;
                bool isManagedServiceAccount;

                ImageStoreFactory.ParseFileImageStoreConnectionString(
                    imageStoreUri,
                    out rootUri,
                    out accountName,
                    out accountPassword,
                    out isManagedServiceAccount);
                
                ImageStoreAccessDescription accessDescription = null;

#if !DotNetCoreClrLinux
                WindowsIdentity windowsIdentity = null;

                windowsIdentity = GetWindowsIdentity(accountName, accountPassword, isManagedServiceAccount);

                if (windowsIdentity != null)
                {
                    accessDescription = new ImageStoreAccessDescription(windowsIdentity, localRoot);
                }
#endif

                return new FileImageStore(rootUri, localRoot, accessDescription);
            }
            else if (XStoreProxy.IsXStoreUri(imageStoreUri))
            {
                return new XStoreProxy(imageStoreUri, localRoot);
            }
            else if (NativeImageStoreClient.IsNativeImageStoreUri(imageStoreUri))
            {
                return new NativeImageStoreClient(connectionStrings, isInternal, workingDirectory, credentials);
            }
            else
            {
                throw new ArgumentException(StringResources.ImageStoreError_InvalidImageStoreUriScheme);
            }
        }

#if !DotNetCoreClrLinux
        private static WindowsIdentity GetWindowsIdentity(
            string accountName,
            string accountPassword,
            bool isManagedServiceAccount)
        {
            WindowsIdentity windowsIdentity = null;

            if (!string.IsNullOrEmpty(accountName))
            {
                string userName, domainName;
                if (!AccountHelper.TryParseUserAccountName(accountName, isManagedServiceAccount, out userName, out domainName))
                {
                    throw new ArgumentException(
                        string.Format(
                            CultureInfo.InvariantCulture,
                            "AccountName={0} in ImageStoreConnectionString is invalid",
                            accountName));
                }

                windowsIdentity = AccountHelper.CreateWindowsIdentity(userName, domainName, accountPassword, isManagedServiceAccount);
            }

            return windowsIdentity;
        }
#endif

        public static void ParseFileImageStoreConnectionString(
            string imageStoreConnectionString, 
            out string rootUri, 
            out string accountName, 
            out string accountPassword,
            out bool isManagedServiceAccount)
        {
            rootUri = accountName = accountPassword = null;
            isManagedServiceAccount = false;
            
            string[] tokens = imageStoreConnectionString.Split('|');
            rootUri = tokens[0];

            if (tokens.Length == 1)
            {
                return;
            }

            string accountType = ImageStoreConstants.DomainUser_AccountType;
            foreach (string token in tokens)
            {
                if (token.StartsWith(ImageStoreConstants.AccountTypeKey, StringComparison.OrdinalIgnoreCase))
                {
                    accountType = token.Substring(ImageStoreConstants.AccountTypeKey.Length);
                }
                else if (token.StartsWith(ImageStoreConstants.AccountNameKey, StringComparison.OrdinalIgnoreCase))
                {
                    accountName = token.Substring(ImageStoreConstants.AccountNameKey.Length);
                }
                else if (token.StartsWith(ImageStoreConstants.AccountPasswordKey, StringComparison.OrdinalIgnoreCase))
                {
                    accountPassword = token.Substring(ImageStoreConstants.AccountPasswordKey.Length);
                }
                else if (token.StartsWith(FileImageStore.SchemeTag, StringComparison.OrdinalIgnoreCase))
                {
                    continue;
                }
                else
                {
                    throw new ArgumentException(
                        string.Format(
                        CultureInfo.InvariantCulture,
                        "ImageStoreConnectionString is invalid. {0} is not expected.",
                        token));
                }
            }

            if (accountType.Equals(ImageStoreConstants.DomainUser_AccountType, StringComparison.OrdinalIgnoreCase))
            {
                isManagedServiceAccount = false;
            }
            else if (accountType.Equals(ImageStoreConstants.ManagedServiceAccount_AccountType, StringComparison.OrdinalIgnoreCase))
            {
                isManagedServiceAccount = true;
            }
            else
            {
                throw new ArgumentException(
                    string.Format(
                    CultureInfo.InvariantCulture,
                    "The AccountType={0} in ImageStoreConnectionString is invalid. Valid values are {1} and {2}",
                    accountType,
                    ImageStoreConstants.DomainUser_AccountType,
                    ImageStoreConstants.ManagedServiceAccount_AccountType));
            }
            

            if(string.IsNullOrEmpty(accountName))
            {
                throw new ArgumentException("ImageStoreConnectionString is invalid. AccountName is missing.");
            }

            if (isManagedServiceAccount)
            {
                if (!string.IsNullOrEmpty(accountPassword))
                {
                    throw new ArgumentException(
                        string.Format(
                        CultureInfo.InvariantCulture,
                        "ImageStoreConnectionString is invalid. AccountPassword should not be set for AccountType={0}. AccountName={1}.",
                        accountType,
                        accountName));
                }

                accountPassword = NativeHelper.SERVICE_ACCOUNT_PASSWORD;
            }
            else
            {
                if(string.IsNullOrEmpty(accountPassword))
                {
                    throw new ArgumentException(
                        string.Format(
                        CultureInfo.InvariantCulture,
                        "ImageStoreConnectionString is invalid. AccountPassword should be set for AccountType={0}. AccountName={1}.",
                        accountType,
                        accountName));
                }
            }           
        }
    }
}