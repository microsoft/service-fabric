// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace CreatePhysicalLog
{
    using System;
    using System.Configuration;
    using System.Globalization;
    using System.IO;
    using System.Text;
    using System.Fabric.Data.Log;
    using System.Threading.Tasks;
    using System.Threading;
    using System.ComponentModel;
    using Microsoft.Win32.SafeHandles;
    using System.Runtime.InteropServices;

    public class ExtractLogicalLogClass
    {
        private static void PrintUsage()
        {
            Console.WriteLine("Usage:\n");
            Console.WriteLine("ExtractLogicalLog <SharedLogFilePath> <SharedLog {guid}> <Stream {guid}> <output file>");
            Console.WriteLine("\n\n");
            Console.WriteLine("How to find the parameters for this tool from the fabric traces:\n\n");
            Console.WriteLine("Look for a record similar to this one:\n");
            Console.WriteLine("    attempt to create logical log path \\??\\C:\\ProgramData\\Windows Fabric\\Public1\\Fabric\\work\\Applications\\StateManagerTestServiceApplication_App0\\work\\Log_ab1811e6-997f-4947-b4d4-f86c941ac6ec_130365533780049196_70ae0579-a578-4913-a07e-f257b8d583ed.log streamId 70ae0579-a578-4913-a07e-f257b8d583ed alias Log_ab1811e6-997f-4947-b4d4-f86c941ac6ec_130365533780049196, physical log path and id \\??\\C:\\ProgramData\\Windows Fabric\\Public1\\Fabric\\work\\Applications\\StateManagerTestServiceApplication_App0\\work\\Shared_ab1811e6-997f-4947-b4d4-f86c941ac6ec_01cf26c3-23e9-fd2c-0807-060504030201.log 01cf26c3-23e9-fd2c-0807-060504030201\n");
            Console.WriteLine("You would pass parameters:");
            Console.WriteLine("    ExtractLogicalLog \"\\??\\C:\\ProgramData\\Windows Fabric\\Public1\\Fabric\\work\\Applications\\StateManagerTestServiceApplication_App0\\work\\Shared_ab1811e6-997f-4947-b4d4-f86c941ac6ec_01cf26c3-23e9-fd2c-0807-060504030201.log\" 01cf26c3-23e9-fd2c-0807-060504030201 70ae0579-a578-4913-a07e-f257b8d583ed MyOutFile.log");
        }

        public static int Main(string[] args)
        {
            int ret;

            if (args.Length != 4)
            {
                PrintUsage();
                return(-1);
            }

            //
            // Parse input parameters
            //
            Guid logId = new Guid();
            Guid streamId = new Guid();
            string pathToLogContainer;
            string pathToOutFile;

            pathToLogContainer = args[0];
            pathToOutFile = args[3];

            try
            {
                logId = Guid.Parse(args[1]);
            }
            catch (Exception ex)
            {
                Console.WriteLine("Exception: {0}", ex.ToString());
                Console.WriteLine("Log Container Id {0} specified is in an incorrect format", args[1]);
                return (-1);
            }

            try
            {
                streamId = Guid.Parse(args[2]);
            }
            catch (Exception ex)
            {
                Console.WriteLine("Exception: {0}", ex.ToString());
                Console.WriteLine("Log Stream Id {0} specified is in an incorrect format", args[2]);
                return (-1);
            }

            ret = ExtractLogicalLog(pathToLogContainer, logId, streamId, pathToOutFile);

            return (ret);
        }

        public static int ExtractLogicalLog(
            string pathToLogContainer,
            Guid logId,
            Guid streamId,
            string pathToOutFile
            )
        {
            //
            // Open up the log manager
            //
            ILogManager logManager;
            try
            {
                Task<ILogManager> openLogManagerTask = LogManager.OpenAsync(LogManager.LoggerType.Default, CancellationToken.None);
                openLogManagerTask.Wait();
                logManager = openLogManagerTask.Result;
            }
            catch (Exception ex)    
            {
                Console.WriteLine("OpenLogManager failed");
                Console.WriteLine(ex.ToString());
                return (1);
            }

            //
            // Open up the physical log
            //
            IPhysicalLog logContainer;
            ILogicalLog logicalLog;


            try
            {
                Task<IPhysicalLog> openLogContainerTask = logManager.OpenPhysicalLogAsync(pathToLogContainer, logId, false, CancellationToken.None);
                openLogContainerTask.Wait();
                logContainer = openLogContainerTask.Result;
            }
            catch (Exception ex)
            {
                Console.WriteLine("Open logfilepath {0} logid {1} failed", pathToLogContainer, logId.ToString());
                Console.WriteLine(ex.ToString());
                return (-2);
            }

            //
            // Next open up the logical log stream
            //
            try
            {
                Task<ILogicalLog> openLogicalLogTask = logContainer.OpenLogicalLogAsync(streamId, "", CancellationToken.None);
                openLogicalLogTask.Wait();
                logicalLog = openLogicalLogTask.Result;
            }
            catch (Exception ex)
            {
                Console.WriteLine("Open logical log {0}", streamId.ToString());
                Console.WriteLine(ex.ToString());
                return (-2);
            }

            //
            // Open up the output file as a sparse file
            //
            int blockSize = 0x10000;
            FileStream fs = File.Create(pathToOutFile, blockSize, FileOptions.SequentialScan);
            SafeFileHandle logFileHandle = fs.SafeFileHandle;
            NativeMethods.SetSparseFileAttribute(logFileHandle);
            BinaryWriter bw = new BinaryWriter(fs);
            

            //
            // Build the output file
            //
            long head = logicalLog.HeadTruncationPosition;

            //
            // Write out the leading zeros; CONSIDER: a way to do this without actually writing
            //
            long blocks = head / blockSize;
            long lastBlockSize = head - (blocks * blockSize);
            byte[] buffer = new byte[blockSize];
            for (int i = 0; i < blockSize; i++)
            {
                buffer[i] = 0;
            }

            Console.WriteLine("Writing {0} bytes of zero at head of stream {1} to {2} in {3} blocks", head, streamId.ToString(), pathToOutFile, (blocks + 1));
            
            for (long i = 0; i < blocks; i++)
            {
                bw.Write(buffer, 0, blockSize);
                Console.Write(".");
            }
            Console.WriteLine();

            bw.Write(buffer, 0, (int)lastBlockSize);

            //
            // loop reading and writing
            //
            long length = logicalLog.Length;
            blocks = length / blockSize;
            lastBlockSize = length - (blocks * blockSize);
            int bytesRead;

            Console.WriteLine("Writing {0} bytes from stream {1} to {2} in {3} blocks", length, streamId.ToString(), pathToOutFile, (blocks + 1));
            
            for (long i = 0; i < (blocks+1); i++)
            {
                logicalLog.SeekForRead((i * blockSize) + head, SeekOrigin.Begin);
                Task<int> readTask = logicalLog.ReadAsync(buffer, 0, blockSize, 0, CancellationToken.None);
                readTask.Wait();
                bytesRead = readTask.Result;

                if (i < blocks)
                {
                    if (bytesRead != blockSize)
                    {
                        Console.WriteLine("Block {0} read {1} bytes and expected {2}", i, bytesRead, blockSize);
                    }
                }
                else
                {
                    if (bytesRead != lastBlockSize)
                    {
                        Console.WriteLine("Last block {0} read {1} bytes and expected {2}", i, bytesRead, lastBlockSize);
                    }
                }

                bw.Write(buffer, 0, bytesRead);
                Console.Write(".");
            }
            Console.WriteLine();

            //
            // Make the head of the file sparse. CONSIDER: is this needed ?
            //
            // NativeMethods.ZeroFile(logFileHandle, (ulong)head);

            bw.Close();
            fs.Close();

            return (0);
        }
    }

    //
    // Native methods class stolen from replicator sources
    //
    internal static class NativeMethods
    {
        internal static unsafe void SetSparseFileAttribute(SafeFileHandle handle)
        {
            uint temp;
            bool success = SparseFileDeviceIoControl(handle, FSCTL_SET_SPARSE, IntPtr.Zero, 0, IntPtr.Zero, 0, out temp, IntPtr.Zero);
            if (success == false)
                throw new Win32Exception(Marshal.GetLastWin32Error());

            return;
        }

        internal static unsafe void ZeroFile(SafeFileHandle handle, ulong uptoOffset)
        {
            uint temp;
            FILE_ZERO_DATA_INFORMATION fzdi = new FILE_ZERO_DATA_INFORMATION(0, (long)uptoOffset);
            bool success = ZeroFileDeviceIoControl(handle, FSCTL_SET_ZERO_DATA, ref fzdi, (uint)sizeof(FILE_ZERO_DATA_INFORMATION), IntPtr.Zero, 0, out temp, IntPtr.Zero);
            if (success == false)
                throw new Win32Exception(Marshal.GetLastWin32Error());

            return;
        }

        private const uint ERROR_UNABLE_TO_MOVE_REPLACEMENT = 0x498;
        private const uint ERROR_UNABLE_TO_MOVE_REPLACEMENT_2 = 0x499;

        internal static unsafe void RenameFileAtomically(string replacedFileName, string replacementFileName, string backupFileName)
        {
            do
            {
                bool success = ReplaceFile(replacedFileName, replacementFileName, backupFileName, 0, IntPtr.Zero, IntPtr.Zero);
                if (success == true)
                    break;

                int error = Marshal.GetLastWin32Error();
                if (error == ERROR_UNABLE_TO_MOVE_REPLACEMENT)
                    return;
                if (error == ERROR_UNABLE_TO_MOVE_REPLACEMENT_2)
                    continue;

                throw new Win32Exception(error);
            } while (true);

            return;
        }

        [DllImport("Kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        private extern static bool ReplaceFile(string replacedFileName, string replacementFileName, string backupFileName,
                                                uint replaceFlags, IntPtr pExclude, IntPtr pReserved);


        private const uint FSCTL_SET_SPARSE = 0x000900c4;
        private const uint FSCTL_SET_ZERO_DATA = 0x000980c8;

        [DllImport("Kernel32.dll", SetLastError = true, EntryPoint = "DeviceIoControl")]
        private extern static bool SparseFileDeviceIoControl(SafeFileHandle handle, uint ioControlCode,
                                                             IntPtr pv1, uint inBufferSize,
                                                             IntPtr pv2, uint outBufferSize,
                                                             out uint bytesReturned,
                                                             IntPtr pv3);

        [DllImport("Kernel32.dll", SetLastError = true, EntryPoint = "DeviceIoControl")]
        private extern static bool ZeroFileDeviceIoControl(SafeFileHandle handle, uint ioControlCode,
                                                           [In]
                                                           ref FILE_ZERO_DATA_INFORMATION fzdi, uint inBufferSize,
                                                           IntPtr pv1, uint outBufferSize,
                                                           out uint bytesReturned,
                                                           IntPtr pv2);

        [StructLayout(LayoutKind.Sequential)]
        private struct FILE_ZERO_DATA_INFORMATION
        {
            public FILE_ZERO_DATA_INFORMATION(long zeroOffset, long nonZeroOffset)
            {
                this.zeroOffset = zeroOffset;
                this.nonZeroOffset = nonZeroOffset;
            }

            private long zeroOffset;
            private long nonZeroOffset;
        }
    }


}