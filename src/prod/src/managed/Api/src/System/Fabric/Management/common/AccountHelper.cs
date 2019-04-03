// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.Common
{
    using System.Globalization;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Security.Principal;

    internal static class AccountHelper
    {
        static readonly string UPNDelimiter = "@";
        static readonly string DLNDelimiter = "\\";

        public static bool TryParseUserAccountName(string accountName, bool isManagedServiceAccount, out string userName, out string domainName)
        {
            userName = null;
            domainName = null;

            string[] tokens;
            if (accountName.Contains(UPNDelimiter))
            {
                tokens = accountName.Split(new string[] { UPNDelimiter }, StringSplitOptions.None);

                if (tokens.Length != 2)
                {
                    return false;
                }

                domainName = tokens[1];
                userName = tokens[0];
            }
            else
            {
                tokens = accountName.Split(new string[] { DLNDelimiter }, StringSplitOptions.None);

                if (tokens.Length != 2)
                {
                    return false;
                }

                domainName = tokens[0];
                userName = tokens[1];
            }

            if (isManagedServiceAccount && !userName.EndsWith("$", StringComparison.OrdinalIgnoreCase))
            {
                userName = userName + "$";
            }

            return true;
        }

        public static WindowsIdentity CreateWindowsIdentity(
            string userName,
            string domainName,
            SecureString password,
            bool isManagedServiceAccount)
        {
            IntPtr passwordPtr = IntPtr.Zero;
            try
            {
                passwordPtr = Marshal.SecureStringToGlobalAllocUnicode(password);
                return AccountHelper.CreateWindowsIdentity(
                    userName,
                    domainName,
                    passwordPtr,
                    isManagedServiceAccount);
            }
            finally
            {
                Marshal.ZeroFreeGlobalAllocUnicode(passwordPtr);
            }
        }

        public static WindowsIdentity CreateWindowsIdentity(
            string userName,
            string domainName,
            string password,
            bool isManagedServiceAccount)
        {
            IntPtr passwordPtr = IntPtr.Zero;
            try
            {
                passwordPtr = Marshal.StringToHGlobalUni(password);
                
                return AccountHelper.CreateWindowsIdentity(
                    userName,
                    domainName,
                    passwordPtr,
                    isManagedServiceAccount);
            }
            finally
            {                
                Marshal.FreeHGlobal(passwordPtr);
            }
        }


        public static WindowsIdentity CreateWindowsIdentity(
            string userName,
            string domainName,
            IntPtr password,
            bool isManagedServiceAccount)
        {
            IntPtr handle = IntPtr.Zero;
            try
            {
                NativeHelper.LogonType logonType =
                    isManagedServiceAccount ? NativeHelper.LogonType.LOGON32_LOGON_SERVICE : NativeHelper.LogonType.LOGON32_LOGON_NETWORK_CLEARTEXT;

                bool success = NativeHelper.LogonUser(
                    userName,
                    domainName,
                    password,
                    logonType,
                    NativeHelper.LogonProvider.LOGON32_PROVIDER_DEFAULT,
                    out handle);
                int win32Err = Marshal.GetLastWin32Error();

                if (success)
                {
                    return new WindowsIdentity(handle);
                }
                else
                {
                    throw new InvalidOperationException(
                        string.Format(
                        CultureInfo.InvariantCulture,
                        "Failed to get AccessToken. UserName: {0}, DomainName: {1}, IsManagedServiceAccount={2}. Error:{3}",
                        userName,
                        domainName,
                        isManagedServiceAccount,
                        win32Err));
                }
            }
            finally
            {
                if (handle != IntPtr.Zero)
                {
                    NativeHelper.CloseHandle(handle);
                }
            }
        }
    }
}