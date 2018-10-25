// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Globalization;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Security.Principal;

    internal static class AccountHelper
    {
        private const int InvalidUserNameOrPasswordError = 1326;
        private const string UPNDelimiter = "@";
        private const string DLNDelimiter = "\\";

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
            bool isManagedServiceAccount,
            NativeHelper.LogonType logonType = NativeHelper.LogonType.LOGON32_LOGON_NETWORK_CLEARTEXT,
            NativeHelper.LogonProvider logonProvider = NativeHelper.LogonProvider.LOGON32_PROVIDER_DEFAULT)
        {
            IntPtr passwordPtr = IntPtr.Zero;
            try
            {
#if !DotNetCoreClr
                passwordPtr = Marshal.SecureStringToGlobalAllocUnicode(password);
#else
                passwordPtr = SecureStringMarshal.SecureStringToGlobalAllocUnicode(password);
#endif
                return AccountHelper.CreateWindowsIdentity(
                    userName,
                    domainName,
                    passwordPtr,
                    isManagedServiceAccount,
                    logonType,
                    logonProvider);
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
            bool isManagedServiceAccount,
            NativeHelper.LogonType logonType = NativeHelper.LogonType.LOGON32_LOGON_NETWORK_CLEARTEXT,
            NativeHelper.LogonProvider logonProvider = NativeHelper.LogonProvider.LOGON32_PROVIDER_DEFAULT)
        {
            IntPtr passwordPtr = IntPtr.Zero;
            try
            {
                passwordPtr = Marshal.StringToHGlobalUni(password);
                
                return AccountHelper.CreateWindowsIdentity(
                    userName,
                    domainName,
                    passwordPtr,
                    isManagedServiceAccount,
                    logonType,
                    logonProvider);
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
            bool isManagedServiceAccount,
            NativeHelper.LogonType logonType = NativeHelper.LogonType.LOGON32_LOGON_NETWORK_CLEARTEXT,
            NativeHelper.LogonProvider logonProvider = NativeHelper.LogonProvider.LOGON32_PROVIDER_DEFAULT)
        {
            IntPtr handle = IntPtr.Zero;
            try
            {
                if(isManagedServiceAccount)
                {
                    logonType = NativeHelper.LogonType.LOGON32_LOGON_SERVICE;
                }

                bool success = NativeHelper.LogonUser(
                    userName,
                    domainName,
                    password,
                    logonType,
                    logonProvider,
                    out handle);
                if (success)
                {
                    return new WindowsIdentity(handle);
                }
                else
                {
                    int win32Err = Marshal.GetLastWin32Error();

                    throw new InvalidOperationException(
                        string.Format(
                        CultureInfo.InvariantCulture,
                        win32Err == InvalidUserNameOrPasswordError ?
                            "Incorrect user name or password. UserName: {0}, DomainName: {1}, IsManagedServiceAccount={2}. Error:{3}" :
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

        public static bool IsAdminUser()
        {
#if DotNetCoreClrLinux
            if (NativeHelper.geteuid()==0)
            {
                return true;
            }
            else
            {
                return false;
            }
#else
            using (WindowsIdentity identity = WindowsIdentity.GetCurrent())
            {
                WindowsPrincipal principal = new WindowsPrincipal(identity);
                SecurityIdentifier sidAdmin = new SecurityIdentifier(WellKnownSidType.BuiltinAdministratorsSid, null);
                return principal.IsInRole(sidAdmin);
            }
#endif
        }

        public static string SecureStringToString(SecureString secStr)
        {
            IntPtr secStrPtr = IntPtr.Zero;
            try
            {
#if !DotNetCoreClr
                secStrPtr = Marshal.SecureStringToGlobalAllocUnicode(secStr);
#else
                secStrPtr = SecureStringMarshal.SecureStringToGlobalAllocUnicode(secStr);
#endif
                return Marshal.PtrToStringUni(secStrPtr);
            }
            finally
            {
                Marshal.ZeroFreeGlobalAllocUnicode(secStrPtr);
            }
        }
    }
}