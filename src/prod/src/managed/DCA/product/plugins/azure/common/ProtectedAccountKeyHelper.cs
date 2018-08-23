// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Globalization;
    using System.Security.Cryptography;
    using System.Text;

    internal static class ProtectedAccountKeyHelper
    {
        internal static bool TryDecryptProtectedAccountKey(string protectedAccountKeyName, out char[] accountKeyCharArray, out string errorMessage)
        {
#if DotNetCoreClrLinux
            IProtectedAccountKeyPlatform protectedAccountKeyPlatform = new ProtectedAccountKeyPlatformLinux();
#else
            IProtectedAccountKeyPlatform protectedAccountKeyPlatform = new ProtectedAccountKeyPlatformWindows();
#endif

            accountKeyCharArray = null;
            errorMessage = null;

            string protectedAccountKey;
            if (!protectedAccountKeyPlatform.TryGetProtectedAccountKey(protectedAccountKeyName, out protectedAccountKey))
            {
                errorMessage = string.Format(CultureInfo.InvariantCulture, "Failed to retrieve protected account key named {0}.", protectedAccountKeyName);

                return false;
            }

            byte[] accountKeyBytes;
            try
            {
                accountKeyBytes = protectedAccountKeyPlatform.DecryptProtectedAccountKey(protectedAccountKey);
            }
            catch (FormatException e)
            {
                errorMessage = string.Format(CultureInfo.InvariantCulture, "Protected account key named {0} is not a valid base-64 string : {1}.", protectedAccountKeyName, e);

                return false;
            }
            catch (CryptographicException e)
            {
                errorMessage = string.Format(CultureInfo.InvariantCulture, "Failed to decrypt protected account key named {0} : {1}.", protectedAccountKeyName, e);

                return false;
            }

            accountKeyCharArray = Encoding.Unicode.GetChars(accountKeyBytes);

            return true;
        }
    }
}