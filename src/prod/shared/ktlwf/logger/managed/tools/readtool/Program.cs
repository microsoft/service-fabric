// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ReadTool
{
    using System;
    using System.IO;
    using System.Collections.Generic;
    using System.Threading;
    using System.Diagnostics;
    using System.Threading.Tasks;
    using System.Runtime.InteropServices;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Test;
    using System.Linq;
    using System.Security.AccessControl;
    using System.Fabric.Data.Log;
    using System.Fabric.Data.Log.Interop;
    using System.Runtime.CompilerServices;
    using VS = Microsoft.VisualStudio.TestTools.UnitTesting;
    using System.Xml;


    class Program
    {
        public string _Drive;
        public string _SharedLogFile;
        public Guid _SharedGuid;
        public string _DedicatedLogFile;
        public Guid _DedicatedGuid;
        public string _FileLogFile;
        public bool _IsRead;

        public const uint _RecordOverhead = 4 * 422; // 1088;
        public uint _MagicRecordSize = 1024 * 1024;  // 1032192;  // This is 4 complete records Why 16K short of 1MB ?
        public long _LogSize = 8 * 1024L * 1024L * 1024L;
        public long _LlogSize = 2 * 1024L * 1024L * 1024L;
        public bool _DedicatedLogOnly = false;
        public int _NumberRecords = 0x80000;
#if DotNetCoreClrLinux
        private static readonly string PlatformPathPrefix = "";
        private static readonly string PlatformPathDelimiter = "//";
#else
        private static readonly string PlatformPathPrefix = "\\??\\";
        private static readonly string PlatformPathDelimiter = "\\";
#endif

        static int Main(string[] args)
        {
            int result;
            Program program = new Program();

            result = program.TheMain(args);

            return (result);
        }

        int TheMain(string[] args)
        {
            // Test config section:
            //
            //      Configure the shared (physical) log to be on a dedicated disk
            //      Configure the dedicated (logical) log to be on a disk other than the dedicated
            //      Pick a reasonable record size
            //
            try
            {
                Console.WriteLine("ReadTool <write | read> <Path> <Shared Log Guid> <Dedicated Log Guid> <Number 4K records for write or Number bytes per read for read>");

                if (args.Length != 5)
                {
                    return (-1);
                }

                if (args[0] == "read")
                {
                    _IsRead = true;
                }
                else if (args[0] == "write")
                {
                    _IsRead = false;
                }
                else
                {
                    return (-1);
                }

                _Drive = args[1] + PlatformPathDelimiter;

                _SharedGuid = Guid.Parse(args[2]);
                _DedicatedGuid = Guid.Parse(args[3]);
                _SharedLogFile = PlatformPathPrefix + _Drive + args[2] + ".log";
                _DedicatedLogFile = PlatformPathPrefix + _Drive + args[3] + ".sflog";
                _FileLogFile = _Drive + args[3] + ".filelog";

                _NumberRecords = int.Parse(args[4]);
                if (_NumberRecords == 0)
                {
                    _NumberRecords = 0x80000;
                }
            }
            catch (Exception e)
            {
                Console.WriteLine(e.ToString());
                return (-3);
            }

            ILogManager logManager;
            logManager = CreateLogManager();
            ILogicalLog lLog;

            if (_IsRead)
            {
                IPhysicalLog sharedLog;
                sharedLog = OpenPhysicalLog(logManager, _SharedLogFile, _SharedGuid);

                lLog = OpenLogicalLog(sharedLog, _DedicatedGuid);
                ReadLogFileBypassStream(lLog, 1052400).Wait();
                lLog.CloseAsync(CancellationToken.None).Wait();
                lLog = null;

                lLog = OpenLogicalLog(sharedLog, _DedicatedGuid);
                ReadLogFileByStream(lLog, "Ktl", _NumberRecords).Wait();
                lLog.CloseAsync(CancellationToken.None).Wait();
                lLog = null;

#if false
                lLog = OpenLogicalLog(sharedLog, _DedicatedGuid);
                ReadLogFileBypassStream(lLog, _NumberRecords).Wait();
                lLog.CloseAsync(CancellationToken.None).Wait();
                lLog = null;
#endif

                Task<ILogicalLog> t;
                t = Microsoft.ServiceFabric.Replicator.FileLogicalLog.CreateFileLogicalLog(_FileLogFile);
                t.Wait();
                lLog = t.Result;
                ReadLogFileByStream(lLog, "File", _NumberRecords).Wait();
                lLog.CloseAsync(CancellationToken.None).Wait();
                lLog = null;
            }
            else
            {
                IPhysicalLog sharedLog;
                sharedLog = CreatePhysicalLog(logManager,
                                              _SharedLogFile,
                                              _SharedGuid,
                                              _LogSize,
                                              0,
                                              0);
                lLog = CreateLogicalLog(sharedLog, _DedicatedGuid, null, _DedicatedLogFile, null, _LlogSize, 1024*1024);
                Write4KLogFile(lLog, "Ktl").Wait();
                lLog.CloseAsync(CancellationToken.None).Wait();
                lLog = null;

                Task<ILogicalLog> t;
                t = Microsoft.ServiceFabric.Replicator.FileLogicalLog.CreateFileLogicalLog(_FileLogFile);
                t.Wait();
                lLog = t.Result;

                Write4KLogFile(lLog, "File").Wait();
                lLog.CloseAsync(CancellationToken.None).Wait();
                lLog = null;
            }

            return (0);
        }

        int writeRecordSize = 4096 - 272;
        private async Task Write4KLogFile(
            ILogicalLog lLog,
            string s
            )
        {
            byte[] writeRecord = new byte[writeRecordSize];

            long ticksWriting = 0;
            long frequencyPerMs = Stopwatch.Frequency / 1000;

            Stopwatch swTotal = new Stopwatch();
            Stopwatch swWriting = new Stopwatch();

            try
            {
                if (_DedicatedLogOnly)
                {
                    await lLog.ConfigureWritesToOnlyDedicatedLogAsync(CancellationToken.None);
                }

                swTotal.Start();
                for (int i = 0; i < _NumberRecords; i++)
                {
                    for (int j = 0; j < writeRecordSize; j++)
                    {
                        writeRecord[j] = (byte)(i * j);
                    }

                    swWriting.Restart();
                    await lLog.AppendAsync(writeRecord, 0, writeRecord.Length, CancellationToken.None);
                    swWriting.Stop();
                    ticksWriting += swWriting.ElapsedTicks;
                }
                await lLog.FlushWithMarkerAsync(CancellationToken.None);
                swTotal.Stop();

                Console.WriteLine("Write {0}: Total ms {1} Writing ms {2}", s, swTotal.ElapsedMilliseconds, ticksWriting / frequencyPerMs);
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.ToString());
            }
        }

        internal static async Task<int> ReadAsync(Stream stream, byte[] buffer, int totalNumberOfBytesToRead)
        {
            int numberOfBytesRead = 0; ;

            do
            {
                var numberOfBytesToRead = totalNumberOfBytesToRead - numberOfBytesRead;
                var currentNumberOfBytesRead = await stream.ReadAsync(buffer, numberOfBytesRead, numberOfBytesToRead);
                if (currentNumberOfBytesRead == 0)
                {
                    return (numberOfBytesRead);
                }

                numberOfBytesRead += currentNumberOfBytesRead;
            } while (numberOfBytesRead < totalNumberOfBytesToRead);

            return (numberOfBytesRead);
        }


        private async Task ReadLogFileByStream(
            ILogicalLog lLog,
            string s,
            int readRecordSize
            )
        {
            long totalWriteBytes = lLog.Length;
            byte[] readRecord = new byte[readRecordSize];
            Random rnd = new Random();

            long ticksWaiting = 0;
            long ticksReading = 0;

            Stopwatch swTotal = new Stopwatch();
            Stopwatch swWaiting = new Stopwatch();
            Stopwatch swReading = new Stopwatch();
            long frequencyPerMs = Stopwatch.Frequency / 1000;

            Stream stream;

            //       This should be 0 so the log will figure it out from reading
            //       directly from the log. This uses a new IOCTL that returns
            //       the max record size and not the max user record size. If the driver
            //       is an old driver then default max record size to 1MB
            //
            stream = lLog.CreateReadStream(1024*1024);  // This should match replicator

            stream.Seek(0, System.IO.SeekOrigin.Begin);

            long totalReadBytes = 0;

            swTotal.Start();
            while (totalReadBytes < totalWriteBytes)
            {
                long bytesRead = readRecordSize;

#if RANDOM_READ_SIZES
                bytesRead = rnd.Next(readRecordSize-1) + 1;

                if (bytesRead + totalReadBytes > totalWriteBytes)
                {
                    bytesRead = totalWriteBytes - totalReadBytes;
                }
#endif
                swReading.Restart();
                bytesRead = await ReadAsync(stream, readRecord, (int)bytesRead);
                swReading.Stop();
                totalReadBytes += bytesRead;
                ticksReading += swReading.ElapsedTicks;

#if INCLUDE_WAITING_TIME
                swWaiting.Restart();
                int waitingms = rnd.Next(15);
                if (waitingms == 3)
                {
                    waitingms = 1;
                }
                else
                {
                    waitingms = 0;
                }

                await Task.Delay(waitingms);

                swWaiting.Stop();
                ticksWaiting += swWaiting.ElapsedTicks;
#endif
            }
            swTotal.Stop();

            Console.WriteLine("By {0} Stream: Size {1} Bytes Read {2} Elapsed ms {3} Waiting ms {4} Reading ms {5}", 
                               s, readRecordSize, totalReadBytes, swTotal.ElapsedMilliseconds, ticksWaiting / frequencyPerMs, ticksReading / frequencyPerMs);

            return;
        }


        private async Task ReadLogFileBypassStream(
            ILogicalLog lLog,
            int readRecordSize
            )
        {
            long totalWriteBytes = lLog.Length;
            byte[] readRecord = new byte[readRecordSize];

            long ticksWaiting = 0;
            long ticksReading = 0;
            long frequencyPerMs = Stopwatch.Frequency / 1000;

            Stopwatch swTotal = new Stopwatch();
            Stopwatch swWaiting = new Stopwatch();
            Stopwatch swReading = new Stopwatch();

            long totalReadBytes = 0;

            swTotal.Start();
            while (totalReadBytes < totalWriteBytes)
            {
                long bytesRead;

                swReading.Restart();
                bytesRead = await lLog.ReadAsync(readRecord, 0, readRecord.Length, readRecord.Length, CancellationToken.None);
                swReading.Stop();
                totalReadBytes += bytesRead;
                ticksReading += swReading.ElapsedTicks;

#if INCLUDE_WAITING_TIME
                swWaiting.Restart();
                int waitingms = rnd.Next(15);
                if (waitingms == 3)
                {
                    waitingms = 1;
                }
                else
                {
                    waitingms = 0;
                }

                await Task.Delay(waitingms);

                swWaiting.Stop();
                ticksWaiting += swWaiting.ElapsedTicks;
#endif

            }
            swTotal.Stop();

            Console.WriteLine("Bypass Stream: Size {0} Bytes Read {1} Elapsed ms {2} Waiting ms {3} Reading ms {4}",
                              readRecord.Length, totalReadBytes, swTotal.ElapsedMilliseconds, ticksWaiting / frequencyPerMs, ticksReading / frequencyPerMs);

            return;
        }


        // **
        //* Local functions
        ILogManager CreateLogManager()
        {
            Task<ILogManager> t = LogManager.OpenAsync(LogManager.LoggerType.Default, CancellationToken.None);
            t.Wait();
            return t.Result;
        }

        IPhysicalLog CreatePhysicalLog(
            ILogManager manager,
            string pathToCommonContainer,
            Guid physicalLogId,
            long containerSize,
            uint maximumNumberStreams,
            uint maximumLogicalLogBlockSize)
        {
            Task<IPhysicalLog> t = manager.CreatePhysicalLogAsync(
                pathToCommonContainer,
                physicalLogId,
                containerSize,
                maximumNumberStreams,
                maximumLogicalLogBlockSize,
                0,                             // Physical logs are not sparse
                CancellationToken.None);

            t.Wait();
            return t.Result;
        }

        IPhysicalLog OpenPhysicalLog(
            ILogManager manager,
            string pathToCommonContainer,
            Guid physicalLogId)
        {
            Task<IPhysicalLog> t = manager.OpenPhysicalLogAsync(
                pathToCommonContainer,
                physicalLogId,
                false,
                CancellationToken.None);

            t.Wait();
            return t.Result;
        }

        ILogicalLog CreateLogicalLog(
                IPhysicalLog phylog,
                Guid logicalLogId,
                string optionalLogStreamAlias,
                string optionalPath,
                FileSecurity optionalSecurityInfo,
                Int64 maximumSize,
                UInt32 maximumBlockSize)
        {
            Task<ILogicalLog> t = phylog.CreateLogicalLogAsync(
                logicalLogId,
                optionalLogStreamAlias,
                optionalPath,
                optionalSecurityInfo,
                maximumSize,
                maximumBlockSize,
                LogManager.LogCreationFlags.UseSparseFile,              // Logical logs are sparse
                "",
                CancellationToken.None);

            t.Wait();
            return t.Result;
        }

        ILogicalLog OpenLogicalLog(
            IPhysicalLog phylog,
            Guid logicalLogId)
        {
            Task<ILogicalLog> t = phylog.OpenLogicalLogAsync(
                logicalLogId,
                "",
                CancellationToken.None);

            t.Wait();
            return t.Result;
        }
    }
}