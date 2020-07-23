// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Fabric;
    using System.Globalization;
    using System.IO;
    using System.Security;
    using System.Security.Cryptography;
    using Microsoft.Win32;

    internal class ProtectedAccountKeyPlatformWindows : IProtectedAccountKeyPlatform
    {
        public byte[] DecryptProtectedAccountKey(string protectedAccountKey)
        {
            byte[] protectedAccountKeyBytes = Convert.FromBase64String(protectedAccountKey);

            return ProtectedData.Unprotect(protectedAccountKeyBytes, null, DataProtectionScope.LocalMachine);
        }

        public bool TryGetProtectedAccountKey(string protectedAccountKeyName, out string protectedAccountKey)
        {
            protectedAccountKey = null;

            try
            {
                if (string.IsNullOrEmpty(protectedAccountKeyName))
                {
                    return false;
                }

                string fabricRegistryKeyFullPath = string.Format(CultureInfo.InvariantCulture, "{0}\\{1}", Registry.LocalMachine.Name, FabricConstants.FabricRegistryKeyPath);

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