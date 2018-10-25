// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using System.Runtime.InteropServices;

    internal class NativeMethods
    {
        #region Service Api

        public const uint SERVICE_NO_CHANGE = 0xffffffff;
        public const uint SERVICE_QUERY_CONFIG = 0x00000001;
        public const uint SERVICE_CHANGE_CONFIG = 0x00000002;
        public const uint SC_MANAGER_ALL_ACCESS = 0x000F003F;
        public const uint SC_MANAGER_CONNECT = 0x00000001;

        public const Int32 ERROR_INSUFFICIENT_BUFFER = 122;
        public const Int32 ERROR_SERVICE_DOES_NOT_EXIST = 1060;
        public const Int32 ERROR_SERVICE_EXISTS = 1073;

        public class SERVICE_ACCESS
        {
            public const uint SERVICE_QUERY_STATUS = 0x00004;
            public const uint SERVICE_ENUMERATE_DEPENDENTS = 0x00008;
            public const uint SERVICE_START = 0x00010;
            public const uint SERVICE_STOP = 0x00020;
            public const uint SERVICE_PAUSE_CONTINUE = 0x00040;
            public const uint SERVICE_INTERROGATE = 0x00080;
            public const uint SERVICE_USER_DEFINED_CONTROL = 0x00100;
            public const uint STANDARD_RIGHTS_REQUIRED = 0x000F0000;
            public const uint SERVICE_ALL_ACCESS = (STANDARD_RIGHTS_REQUIRED |
            SERVICE_QUERY_CONFIG |
            SERVICE_CHANGE_CONFIG |
            SERVICE_QUERY_STATUS |
            SERVICE_ENUMERATE_DEPENDENTS |
            SERVICE_START |
            SERVICE_STOP |
            SERVICE_PAUSE_CONTINUE |
            SERVICE_INTERROGATE |
            SERVICE_USER_DEFINED_CONTROL);
        }

        public class SERVICE_TYPE
        {
            public const uint SERVICE_WIN32_OWN_PROCESS = 0x00000010;
        }

        public class SERVICE_START
        {
            public const uint SERVICE_AUTO_START = 0x00000002;
        }

        public class SERVICE_ERROR
        {
            public const uint SERVICE_ERROR_NORMAL = 0x00000001;
        }

        public class SC_ACTION_TYPE
        {
            public const int SC_ACTION_RESTART = 1;
        };

        public class SERVICE_INFO_LEVELS
        {
            public const uint SERVICE_CONFIG_DESCRIPTION = 0x00000001;
            public const uint SERVICE_CONFIG_FAILURE_ACTIONS = 0x00000002;
            public const uint SERVICE_CONFIG_DELAYED_AUTO_START_INFO = 0x00000003;
        }

        [StructLayout(LayoutKind.Sequential)]
        public class QUERY_SERVICE_CONFIG
        {
            [MarshalAs(System.Runtime.InteropServices.UnmanagedType.U4)]
            public UInt32 dwServiceType;
            [MarshalAs(System.Runtime.InteropServices.UnmanagedType.U4)]
            public UInt32 dwStartType;
            [MarshalAs(System.Runtime.InteropServices.UnmanagedType.U4)]
            public UInt32 dwErrorControl;
            [MarshalAs(System.Runtime.InteropServices.UnmanagedType.LPWStr)]
            public String lpBinaryPathName;
            [MarshalAs(System.Runtime.InteropServices.UnmanagedType.LPWStr)]
            public String lpLoadOrderGroup;
            [MarshalAs(System.Runtime.InteropServices.UnmanagedType.U4)]
            public UInt32 dwTagID;
            [MarshalAs(System.Runtime.InteropServices.UnmanagedType.LPWStr)]
            public String lpDependencies;
            [MarshalAs(System.Runtime.InteropServices.UnmanagedType.LPWStr)]
            public String lpServiceStartName;
            [MarshalAs(System.Runtime.InteropServices.UnmanagedType.LPWStr)]
            public String lpDisplayName;
        };

        [StructLayout(LayoutKind.Sequential)]
        public class SC_ACTION
        {
            [MarshalAs(System.Runtime.InteropServices.UnmanagedType.I4)]
            public Int32 Type;
            [MarshalAs(System.Runtime.InteropServices.UnmanagedType.U4)]
            public UInt32 Delay;
        };

        [StructLayout(LayoutKind.Sequential)]
        public class SERVICE_DESCRIPTION
        {
            [MarshalAs(System.Runtime.InteropServices.UnmanagedType.LPWStr)]
            public String lpDescription;
        }

        [StructLayout(LayoutKind.Sequential)]
        public class SERVICE_DELAYED_AUTO_START_INFO
        {
            [MarshalAs(System.Runtime.InteropServices.UnmanagedType.I4)]
            public Int32 fDelayedAutostart;
        }

        [StructLayout(LayoutKind.Sequential)]
        public class SERVICE_FAILURE_ACTIONS
        {
            [MarshalAs(System.Runtime.InteropServices.UnmanagedType.U4)]
            public UInt32 dwResetPeriod;
            [MarshalAs(System.Runtime.InteropServices.UnmanagedType.LPWStr)]
            public String lpRebootMsg;
            [MarshalAs(System.Runtime.InteropServices.UnmanagedType.LPWStr)]
            public String lpCommand;
            [MarshalAs(System.Runtime.InteropServices.UnmanagedType.U4)]
            public UInt32 cActions;
            public IntPtr lpsaActions;
        };

        [DllImport("advapi32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern IntPtr OpenService(IntPtr hSCManager, string lpServiceName, uint dwDesiredAccess);

        [DllImport("advapi32.dll", EntryPoint = "OpenSCManagerW", ExactSpelling = true, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern IntPtr OpenSCManager(string machineName, string databaseName, uint dwAccess);

        [DllImport("advapi32.dll", EntryPoint = "CloseServiceHandle")]
        public static extern int CloseServiceHandle(IntPtr hSCObject);

        [DllImport("advapi32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern Boolean QueryServiceConfig(IntPtr hService, IntPtr intPtrQueryConfig, int cbBufSize, out int pcbBytesNeeded);

        [DllImport("advapi32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern Boolean ChangeServiceConfig(IntPtr hService, UInt32 nServiceType, UInt32 nStartType, UInt32 nErrorControl, String lpBinaryPathName, String lpLoadOrderGroup, IntPtr lpdwTagId, String lpDependencies, String lpServiceStartName, String lpPassword, String lpDisplayName);

        [DllImport("advapi32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool ChangeServiceConfig2(IntPtr hService, uint dwInfoLevel, IntPtr lpInfo);

        [DllImport("advapi32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern IntPtr CreateService(IntPtr hSCManager, string lpServiceName, string lpDisplayName, uint dwDesiredAccess, uint dwServiceType, uint dwStartType, uint dwErrorControl, string lpBinaryPathName, string lpLoadOrderGroup, string lpdwTagId, string lpDependencies, string lpServiceStartName, string lpPassword);

        [DllImport("advapi32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool DeleteService(IntPtr hService);

        #endregion

        #region Network Api

        public const Int32 ERROR_SUCCESS = 0;
        public const Int32 ERROR_OBJECT_ALREADY_EXISTS = 5010;

        [ComVisible(false), StructLayout(LayoutKind.Sequential)]
        internal struct IPFORWARDTABLE
        {
            public uint Size;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 1)]
            public IPFORWARDROW[] Table;
        };

        [ComVisible(false), StructLayout(LayoutKind.Sequential)]
        internal struct IPFORWARDROW
        {
            internal uint dwForwardDest;
            internal uint dwForwardMask;
            internal uint dwForwardPolicy;
            internal uint dwForwardNextHop;
            internal uint dwForwardIfIndex;
            internal uint dwForwardType;
            internal uint dwForwardProto;
            internal uint dwForwardAge;
            internal uint dwForwardNextHopAS;
            internal uint dwForwardMetric1;
            internal uint dwForwardMetric2;
            internal uint dwForwardMetric3;
            internal uint dwForwardMetric4;
            internal uint dwForwardMetric5;
        };

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto)]
        internal struct FILE_INFO_3
        {
            public uint fi3_id;
            public uint fi3_permission;
            public uint fi3_num_locks;
            [MarshalAs(UnmanagedType.LPWStr)]
            public string fi3_pathname;
            [MarshalAs(UnmanagedType.LPWStr)]
            public string fi3_username;
        }

#if DotNetCoreClrIOT
        [DllImport("iphlpapi", CharSet = CharSet.Unicode)]
        internal extern static int GetIpForwardTable(SafeHandle pIpForwardTable, ref int pdwSize, bool bOrder);

        [DllImport("iphlpapi", CharSet = CharSet.Unicode)]
        internal extern static int CreateIpForwardEntry(SafeInteropStructureToPtrMemoryHandle pRoute);
#else
        [DllImport("iphlpapi", CharSet = CharSet.Auto)]
        internal extern static int GetIpForwardTable(SafeHandle pIpForwardTable, ref int pdwSize, bool bOrder);

        [DllImport("iphlpapi", CharSet = CharSet.Auto)]
        internal extern static int CreateIpForwardEntry(SafeInteropStructureToPtrMemoryHandle pRoute);
#endif

        [DllImport("netapi32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        internal static extern int NetFileEnum(
             string servername,
             string basepath,
             string username,
             int level,
             ref IntPtr bufptr,
             int prefmaxlen,
             out int entriesread,
             out int totalentries,
             IntPtr resume_handle
        );

        [DllImport("Netapi32.dll", SetLastError = true)]
        internal static extern int NetApiBufferFree(IntPtr Buffer);

        [DllImport("netapi32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        internal static extern int NetFileClose(string servername, uint id);

        #endregion
    }
}