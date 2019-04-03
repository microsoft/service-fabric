// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsReader.ConfigReader
{
    using System;
    using System.Fabric;
    using System.Globalization;
    using System.IO;
    using System.Security;
    using System.Security.Cryptography;
    using System.Text;
    using Microsoft.Win32;

    internal static class ProtectedAccountKeyHelper
    {
        internal static bool TryDecryptProtectedAccountKey(string protectedAccountKeyName, out char[] accountKeyCharArray, out string errorMessage)
        {
            accountKeyCharArray = null;
            errorMessage = null;

            string protectedAccountKey;
            if (!TryGetProtectedAccountKey(protectedAccountKeyName, out protectedAccountKey))
            {
                errorMessage = string.Format(CultureInfo.InvariantCulture, "Failed to retrieve protected account key named {0}.", protectedAccountKeyName);

                return false;
            }

            byte[] accountKeyBytes;
            try
            {
                accountKeyBytes = DecryptProtectedAccountKey(protectedAccountKey);
            }
            catch (FormatException e)
            {
                errorMessage = string.Format(
                    CultureInfo.InvariantCulture,
                    "Protected account key named {0} is not a valid base-64 string : {1}.",
                    protectedAccountKeyName,
                    e);

                return false;
            }
            catch (CryptographicException e)
            {
                errorMessage = string.Format(
                    CultureInfo.InvariantCulture,
                    "Failed to decrypt protected account key named {0} : {1}.",
                    protectedAccountKeyName,
                    e);

                return false;
            }

            accountKeyCharArray = Encoding.Unicode.GetChars(accountKeyBytes);

            return true;
        }

        private static byte[] DecryptProtectedAccountKey(string protectedAccountKey)
        {
            byte[] protectedAccountKeyBytes = Convert.FromBase64String(protectedAccountKey);

            return ProtectedData.Unprotect(protectedAccountKeyBytes, null, DataProtectionScope.LocalMachine);
        }

        private static bool TryGetProtectedAccountKey(string protectedAccountKeyName, out string protectedAccountKey)
        {
            protectedAccountKey = null;

            try
            {
                if (string.IsNullOrEmpty(protectedAccountKeyName))
                {
                    return false;
                }

                string fabricRegistryKeyFullPath = string.Format(
                    CultureInfo.InvariantCulture,
                    "{0}\\{1}",
                    Registry.LocalMachine.Name,
                    FabricConstants.FabricRegistryKeyPath);

                protectedAccountKey = Registry.GetValue(fabricRegistryKeyFullPath, protectedAccountKeyName, null) as string;

                return !string.IsNullOrEmpty(protectedAccountKey);
            }
            catch (Exception e)
            {
                if (e is SecurityException || e is IOException || e is ArgumentException)
                {
                    return false;
                }

                throw;
            }
        }
    }
}