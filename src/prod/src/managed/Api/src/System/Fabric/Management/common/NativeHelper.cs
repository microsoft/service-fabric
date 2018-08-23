// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.Common
{
    using System.Runtime.InteropServices;
    using System.Text;

    internal static class NativeHelper
    {
        public enum LogonType
        {
            LOGON32_LOGON_SERVICE = 5,
            LOGON32_LOGON_NETWORK_CLEARTEXT = 8
        }

        public enum LogonProvider
        {
            LOGON32_PROVIDER_DEFAULT = 0
        }

        public static readonly string SERVICE_ACCOUNT_PASSWORD = "_SA_{262E99C9-6160-4871-ACEC-4E61736B6F21}";

        [DllImport("advapi32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern bool LogonUser(
            String userName, 
            String domainName, 
            IntPtr password,
            LogonType logonType, 
            LogonProvider logonProvider, 
            out IntPtr token);

        [DllImport("kernel32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool CloseHandle(IntPtr handle);

        //
        // PInvoke methods to get MSI Product version
        //
        [DllImport("msi.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern uint MsiOpenDatabase(string szDatabasePath, IntPtr phPersist, out IntPtr phDatabase);

        [DllImport("msi.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern int MsiDatabaseOpenViewW(IntPtr hDatabase, [MarshalAs(UnmanagedType.LPWStr)] string szQuery, out IntPtr phView);

        [DllImport("msi.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern int MsiViewExecute(IntPtr hView, IntPtr hRecord);

        [DllImport("msi.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern uint MsiViewFetch(IntPtr hView, out IntPtr hRecord);

        [DllImport("msi.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern int MsiRecordGetString(IntPtr hRecord, int iField, [Out] StringBuilder szValueBuf, ref int pcchValueBuf);

        [DllImport("msi.dll", ExactSpelling = true)]
        public static extern IntPtr MsiCreateRecord(uint cParams);

        [DllImport("msi.dll", ExactSpelling = true)]
        public static extern uint MsiCloseHandle(IntPtr hAny);

#if DotNetCoreClrLinux
        [DllImport("libc.so.6")]
        public static extern int system(string command);

        [DllImport("libc.so.6")]
        public static extern IntPtr popen(string command, string type);

        [DllImport("libc.so.6")]
        public static extern int pclose(IntPtr stream);

        [DllImport("libc.so.6")]
        public static extern IntPtr fgets(StringBuilder s, int size, IntPtr stream);
#endif
    }
}