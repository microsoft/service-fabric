// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Text;

    internal static class NativeHelper
    {
        public enum LogonType
        {
            LOGON32_LOGON_SERVICE = 5,
            LOGON32_LOGON_NETWORK_CLEARTEXT = 8,
            /*
             *This logon type allows the caller to clone its current token and specify new credentials for outbound connections. 
             *The new logon session has the same local identifier but uses different credentials for other network connections.
             */
            LOGON32_LOGON_NEW_CREDENTIALS = 9
        }

        public enum LogonProvider
        {
            LOGON32_PROVIDER_DEFAULT = 0,
            LOGON32_PROVIDER_WINNT50 = 3
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

#if DotNetCoreClrLinux
        [DllImport("libc.so.6")]
        public static extern int system(string command);

        //Linux Specific PInvoke methods.
        [DllImport("libc.so.6")]
        public static extern int chmod(string path, int mode);

        //Linux Specific PInvoke methods
        [DllImport("libc.so.6")]
        public static extern int geteuid();
#endif

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

        //
        // PInvoke methods to parse CAB file.
        //
        [StructLayout(LayoutKind.Sequential)]
        public class CabError
        {
            public int erfOper;
            public int erfType;
            public int fError;
        }

        public enum FCIStatusType
        {
            File = 0,
            Folder = 1,
            Cabinet = 2
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public class CompressionInfo
        {
            int cabSize = int.MaxValue;
            int folderThreshhold;

            int sizeReserveCFHeader;
            int sizeReserveCFFolder;
            int sizeReserveCFData;

            int cabSequence;
            int diskNum = 1;
            int failIfUncompressible;

            short setId = 1;

            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
            string diskName = "Disk1";

            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
            string cabName;

            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
            string cabPath;

            public CompressionInfo(string fileName)
            {
                this.cabPath = Path.GetDirectoryName(fileName);
                this.cabName = Path.GetFileName(fileName);

                if (!this.cabPath.EndsWith(Path.DirectorySeparatorChar.ToString()
#if !DotNetCoreClr
                    // Do we really need to specify culture here?
                    , StringComparison.InvariantCulture
#endif
                    ))
                {
                    this.cabPath += Path.DirectorySeparatorChar.ToString();
                }
            }
        }

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate bool FCIGetNextCabMethod(
            [In, Out] [MarshalAs(UnmanagedType.LPStruct)] CompressionInfo currentCab,
            int sizePreviousCab,
            IntPtr data);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate int FCIFilePlacedMethod(
            [In, Out] [MarshalAs(UnmanagedType.LPStruct)] CompressionInfo currentCab,
            string fileName,
            Int32 fileSize,
            bool continuation,
            IntPtr data);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate IntPtr FCIGetOpenInfoMethod(
            string fileName,
            ref int date,
            ref int time,
            ref short attributes,
            ref int error,
            IntPtr data);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate Int32 FCIStatusMethod(FCIStatusType statusType, int size1, int size2, IntPtr data);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate bool FCIGetTempFileMethod(IntPtr tempFileName, int tempNameSize, IntPtr data);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate IntPtr MemAllocMethod(int size);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void MemFreeMethod(IntPtr buffer);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate IntPtr FCIOpenMethod(string file, int flag, int mode, ref int error, IntPtr data);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate int FCIReadMethod(
            IntPtr descriptor, [In, Out] [MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 2, ArraySubType = UnmanagedType.U1)] byte[] buffer,
            int count,
            ref int error,
            IntPtr data);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate int FCIWriteMethod(
            IntPtr descriptor,
            [MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 2, ArraySubType = UnmanagedType.U1)] byte[] buffer,
            int count,
            ref int error,
            IntPtr data);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate int FCICloseMethod(IntPtr descriptor, ref int error, IntPtr data);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate int FCISeekMethod(IntPtr descriptor, int dist, int seekType, ref int error, IntPtr data);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate int FCIDeleteMethod(string file, ref int error, IntPtr data);

        [DllImport("cabinet.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "FCICreate", CharSet = CharSet.Ansi)]
        public static extern IntPtr FCICreate(
            [MarshalAs(UnmanagedType.LPStruct)] CabError erf,
            FCIFilePlacedMethod filePlaced,
            MemAllocMethod malloc,
            MemFreeMethod free,
            FCIOpenMethod open,
            FCIReadMethod read,
            FCIWriteMethod write,
            FCICloseMethod close,
            FCISeekMethod seek,
            FCIDeleteMethod delete,
            FCIGetTempFileMethod getTempFile,
            [In, Out] [MarshalAs(UnmanagedType.LPStruct)] CompressionInfo cab,
            IntPtr data);

        [DllImport("cabinet.dll", EntryPoint = "FCIAddFile", CallingConvention = CallingConvention.Cdecl)]
        public static extern bool FCIAddFile(
            IntPtr fciHandle,
            string sourceFileName,
            string destFileName,
            bool execute,
            FCIGetNextCabMethod getNextCabinet,
            FCIStatusMethod status,
            FCIGetOpenInfoMethod getOpenInfo,
            short compressionFlags);

        [DllImport("cabinet.dll", EntryPoint = "FCIFlushCabinet", CallingConvention = CallingConvention.Cdecl)]
        public static extern bool FCIFlushCabinet(
            IntPtr fciHandle,
            bool getNextCab,
            FCIGetNextCabMethod getNextCabinet,
            FCIStatusMethod status);

        [DllImport("cabinet.dll", EntryPoint = "FCIFlushFolder", CallingConvention = CallingConvention.Cdecl)]
        public static extern bool FCIFlushFolder(IntPtr fciHandle, FCIGetNextCabMethod getNextCabinet, FCIStatusMethod status);

        [DllImport("cabinet.dll", EntryPoint = "FCIDestroy", CallingConvention = CallingConvention.Cdecl)]
        public static extern bool FCIDestroy(IntPtr fciHandle);

        public enum PdhStatus : uint
        {
            PDH_SUCCESS = 0x0,
            PDH_CSTATUS_NO_INSTANCE = 0x800007D1,
            PDH_CSTATUS_NO_COUNTER = 0xC0000BB9,
            PDH_CSTATUS_NO_OBJECT = 0xC0000BB8,
            PDH_CSTATUS_NO_MACHINE = 0x800007D0,
            PDH_CSTATUS_BAD_COUNTERNAME = 0xC0000BC0,
            PDH_MEMORY_ALLOCATION_FAILURE = 0xC0000BBB,            
        };

        [DllImport("pdh.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern PdhStatus PdhValidatePath(String counterPath);

    }
}