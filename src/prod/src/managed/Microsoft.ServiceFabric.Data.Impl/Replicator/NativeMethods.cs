// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.ComponentModel;
    using System.Runtime.InteropServices;
    using Microsoft.Win32.SafeHandles;

    internal static class NativeMethods
    {
        private const uint ErrorUnableToMoveReplacement = 0x498;
        private const uint ErrorUnableToMoveReplacement2 = 0x499;
        private const uint FsctlSetSparse = 0x000900c4;
        private const uint FsctlSetZeroData = 0x000980c8;

        internal static void ExitProcess()
        {
            ExitProcess(0);
        }

        internal static ulong GetSparseFileSize(string filename)
        {
            uint high;
            uint low;
            low = GetCompressedFileSize(filename, out high);
            var error = Marshal.GetLastWin32Error();
            if (low == 0xFFFFFFFF && error != 0)
            {
                throw new Win32Exception(error);
            }
            else
            {
                return ((ulong) high << 32) + low;
            }
        }

        internal static unsafe void SetSparseFileAttribute(SafeFileHandle handle)
        {
            uint temp;
            var success = SparseFileDeviceIoControl(
                handle,
                FsctlSetSparse,
                IntPtr.Zero,
                0,
                IntPtr.Zero,
                0,
                out temp,
                IntPtr.Zero);
            if (success == false)
            {
                throw new Win32Exception(Marshal.GetLastWin32Error());
            }

            return;
        }

        internal static unsafe void ZeroFile(SafeFileHandle handle, ulong uptoOffset)
        {
            uint temp;
            var fzdi = new FILE_ZERO_DATA_INFORMATION(0, (long) uptoOffset);
            var success = ZeroFileDeviceIoControl(
                handle,
                FsctlSetZeroData,
                ref fzdi,
                (uint) sizeof(FILE_ZERO_DATA_INFORMATION),
                IntPtr.Zero,
                0,
                out temp,
                IntPtr.Zero);
            if (success == false)
            {
                throw new Win32Exception(Marshal.GetLastWin32Error());
            }

            return;
        }

        internal static unsafe void RenameFileAtomically(
            string replacedFileName,
            string replacementFileName,
            string backupFileName)
        {
            do
            {
                var success = ReplaceFile(
                    replacedFileName,
                    replacementFileName,
                    backupFileName,
                    0,
                    IntPtr.Zero,
                    IntPtr.Zero);
                if (success == true)
                {
                    break;
                }

                var error = Marshal.GetLastWin32Error();
                if (error == ErrorUnableToMoveReplacement)
                {
                    return;
                }

                if (error == ErrorUnableToMoveReplacement2)
                {
                    continue;
                }

                throw new Win32Exception(error);
            } while (true);

            return;
        }

        [DllImport("kernel32.dll")]
        private static extern void ExitProcess(uint exitCode);

        [DllImport("kernel32.dll")]
        private static extern uint GetCompressedFileSize(string lpFileName, out uint lpFileSizeHigh);

        [DllImport("Kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        private static extern bool ReplaceFile(
            string replacedFileName,
            string replacementFileName,
            string backupFileName,
            uint replaceFlags,
            IntPtr pExclude,
            IntPtr pReserved);

        [DllImport("Kernel32.dll", SetLastError = true, EntryPoint = "DeviceIoControl")]
        private static extern bool SparseFileDeviceIoControl(
            SafeFileHandle handle,
            uint ioControlCode,
            IntPtr pv1,
            uint inBufferSize,
            IntPtr pv2,
            uint outBufferSize,
            out uint bytesReturned,
            IntPtr pv3);

        [DllImport("Kernel32.dll", SetLastError = true, EntryPoint = "DeviceIoControl")]
        private static extern bool ZeroFileDeviceIoControl(
            SafeFileHandle handle,
            uint ioControlCode,
            [In] ref FILE_ZERO_DATA_INFORMATION fzdi,
            uint inBufferSize,
            IntPtr pv1,
            uint outBufferSize,
            out uint bytesReturned,
            IntPtr pv2);

        [StructLayout(LayoutKind.Sequential)]
        private struct FILE_ZERO_DATA_INFORMATION
        {
            private long zeroOffset;

            private long nonZeroOffset;

            public FILE_ZERO_DATA_INFORMATION(long zeroOffset, long nonZeroOffset)
            {
                this.zeroOffset = zeroOffset;
                this.nonZeroOffset = nonZeroOffset;
            }
        }
    }
}