// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ReadPerformance
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
        public string _SharedDrive;
        public string _DedicatedDrive;
        public const uint _RecordOverhead = 4 * 422; // 1088;
        public uint _MagicRecordSize = 1024 * 1024;  // 1032192;  // This is 4 complete records Why 16K short of 1MB ?
        public uint _RecordSize;
        public int _NumberTasks = 4;
        public int _TimeToRunInSeconds = 0;
//        public long _LogSize = 4 * 1024L * 1024L * 1024L;   // bad
        public long _LogSize = 8 * 1024L * 1024L * 1024L;
        public long _LlogSize = 2 * 1024L * 1024L * 1024L;
        public bool _DedicatedLogOnly = true;
        public bool _AutomaticMode = false;
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

        /*
<?xml version="1.0" encoding="utf-8"?>
<PerformanceResults>
  <PerformanceResult>
    <Context>
      <Environment Name="MachineName" Value="DANVER2" />
      <Parameter Name="BlockSize" Value="1" />
      <Parameter Name="ForegroundQueueSIze" Value="32" />
      <Parameter Name="BackgroundQueueSize" Value="0" />
    </Context>
    <Measurements>
      <Measurement Name="RawDiskPerformance">
              <Value Metric="MBPerSecond" Value="3421.16921758783" When="2/13/2015 2:44:04 AM" />
          </Measurement>
    </Measurements>
  </PerformanceResult>
</PerformanceResults>
 */

        void PrintResults(string ResultsXML, string TestName, uint[] BlockSize, float[] MBInSec)
        {
            if (BlockSize.Length != MBInSec.Length)
            {
                throw new InvalidOperationException("BlockSize and MBInSec length not equal");
            }

            XmlTextWriter writer = new XmlTextWriter(ResultsXML, null);

            //Use indenting for readability.
            writer.Formatting = Formatting.Indented;

            //Write the XML delcaration. 
            writer.WriteStartDocument();

            //Write a root element.
            writer.WriteStartElement("PerformanceResults");

            for (int i = 0; i < BlockSize.Length; i++)
            {
                writer.WriteStartElement("PerformanceResult");
                
                writer.WriteStartElement("Context");

                writer.WriteStartElement("Environment");
                writer.WriteAttributeString("Name", "MachineName");
                writer.WriteAttributeString("Value", "ComputerName");  // TODO: COmputername
                writer.WriteEndElement();    // Environment

                writer.WriteStartElement("Parameter");
                writer.WriteAttributeString("Name", "BlockSizeInKB");
                uint blockSizeInKB = BlockSize[i] * 4;
                writer.WriteAttributeString("Value", blockSizeInKB.ToString());
                writer.WriteEndElement();    // Environment

                writer.WriteEndElement();    // Context

                writer.WriteStartElement("Measurements");

                writer.WriteStartElement("Measurement");
                writer.WriteAttributeString("Name", TestName);

                writer.WriteStartElement("Value");
                writer.WriteAttributeString("Metric", "MBPerSecond");
                writer.WriteAttributeString("Value", MBInSec[i].ToString());
                writer.WriteAttributeString("When", DateTime.UtcNow.ToString());

                writer.WriteEndElement();    // Value

                writer.WriteEndElement();    // Measurement

                writer.WriteEndElement();    // Measurements

                writer.WriteEndElement();    // PerformanceResult
            }

            //Write the close tag for the root element.
            writer.WriteEndElement();

            writer.WriteEndDocument();

            //Write the XML to file and close the writer.
            writer.Flush();
            writer.Close();  


        }

        int TheMain(string[] args)
        {
            string cDrive = "c:";

            // Test config section:
            //
            //      Configure the shared (physical) log to be on a dedicated disk
            //      Configute the dedicated (logical) log to be on a disk other than the dedicated
            //      Pick a reasonable record size
            //
            string sharedDrive = cDrive;
            string dedicatedDrive = cDrive;

            try
            {
                Console.WriteLine("Logical log performance test <shared drive letter> <dedicated drive letter> <number tasks> <time to run in sec> <dedicated log only> <record size in 4KB> <automatic mode>");

                if (args.Length != 7)
                {
                    Console.WriteLine();
                    Console.WriteLine("The <shared drive letter> parameter indicates which drive to create the shared log files It should be in the form c:");
                    Console.WriteLine();
                    Console.WriteLine("The <dedicated drive letter> parameter indicates which drive to create the dedicated log files It should be in the form c:");
                    Console.WriteLine();                   
                    Console.WriteLine("The <number tasks> parameter indicates the number of concurrent tasks that run performing writing to a unique stream using a single outstanding IO.");
                    Console.WriteLine();                   
                    Console.WriteLine("The <dedicated log only> flag when true indicates that the driver should only write to the dedicated log and not the shared log. When false data is written to both logs.");
                    Console.WriteLine();
                    Console.WriteLine("The <record size in 4KB> is the number of bytes to write in each record");
                    Console.WriteLine();
                    Console.WriteLine("The flag <automatic mode> is true to run in automatic mode");
                    Console.WriteLine();
                    return(-1);
                }

                try
                {
                    _SharedDrive = args[0].Substring(0,2);
                    if (_SharedDrive[1] != ':')
                    {
                        Console.WriteLine("Invalid argument for <drive letter> {0}, expect c:", args[0]);
                        return (-1);
                    }
                }
                catch (Exception e)
                {
                    Console.WriteLine("Invalid argument for <shared drive letter> {0}, expect c:", e.ToString());
                    return (-1);
                }

                try
                {
                    _DedicatedDrive = args[1].Substring(0, 2);
                    if (_DedicatedDrive[1] != ':')
                    {
                        Console.WriteLine("Invalid argument for <dedicated drive letter> {0}, expect c:", args[0]);
                        return (-1);
                    }
                }
                catch (Exception e)
                {
                    Console.WriteLine("Invalid argument for <dedicated drive letter> {0}, expect c:", e.ToString());
                    return (-1);
                }

                try
                {
                    _NumberTasks = int.Parse(args[2]);
                } catch(Exception e) {
                    Console.WriteLine("Invalid argument for <number tasks> {0}", e.ToString());
                    return(-1);
                }

                try
                {
                    _TimeToRunInSeconds = int.Parse(args[3]);
                } catch(Exception e) {
                    Console.WriteLine("Invalid argument for <time to run in sec> {0}", e.ToString());
                    return(-1);
                }

                try
                {
                    _DedicatedLogOnly = bool.Parse(args[4]);
                }
                catch (Exception e)
                {
                    Console.WriteLine("Invalid argument for <dedicated log only> {0}", e.ToString());
                    return (-1);
                }

                try
                {
                    _RecordSize = uint.Parse(args[5]) * 0x1000;
                }
                catch (Exception e)
                {
                    Console.WriteLine("Invalid argument for <record size in 4KB> {0}", e.ToString());
                    return (-1);
                }
                if (_RecordSize == 0)
                {
                    _RecordSize = _MagicRecordSize;
                }

                try
                {
                    _AutomaticMode = bool.Parse(args[6]);
                }
                catch (Exception e)
                {
                    Console.WriteLine("Invalid argument for <automatic mode> {0}", e.ToString());
                    return (-1);
                }

                if (_AutomaticMode)
                {
                    const int automationBlockCount = 4;
                    uint[] automationBlockSizes = new uint[automationBlockCount] { 4, 16, 32, 64 };
                    float[] MBPerSecond = new float[automationBlockCount];

                    for (int i = 0; i < automationBlockCount; i++)
                    {
                        _RecordSize = automationBlockSizes[i] * 0x1000;
                        LogPerformanceWorkloadTest(out MBPerSecond[i]);
                        Console.WriteLine();
                    }

                    PrintResults("Results.Perf", "LogicalLogPerformance", automationBlockSizes, MBPerSecond);

                } else {
                    float MBPerSecond;
                    LogPerformanceWorkloadTest(out MBPerSecond);
                }

                return (0);
            }
            catch (Exception e)
            {
                Console.WriteLine(e.ToString());
                return (-3);
            }
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

        int ReadLogicalLog(
            ILogicalLog logicalLog,
            byte[] buffer,
            int readOffset,
            int bufferOffset,
            int bytesToRead,
            int count)
        {
            if (readOffset != -1)
            {
                logicalLog.SeekForRead(readOffset, SeekOrigin.Begin);
            }

            Task<int> t = logicalLog.ReadAsync(buffer, bufferOffset, count, bytesToRead, CancellationToken.None);

            t.Wait();
            return t.Result;
        }

        void AppendLogicalLog(
            ILogicalLog logicalLog,
            byte[] buffer)
        {
            Task t = logicalLog.AppendAsync(buffer, 0, buffer.Length, CancellationToken.None);

            t.Wait();
        }

        // Truncate the oldest information
        void TruncateHead(
            ILogicalLog logicalLog,
            long streamOffset)
        {
            logicalLog.TruncateHead(streamOffset);
        }

        // Truncate the most recent information
        void TruncateTail(
            ILogicalLog logicalLog,
            long streamOffset)
        {
            Task t = logicalLog.TruncateTail(streamOffset, CancellationToken.None);

            t.Wait();
        }

        byte[] BuildBuffer(int count)
        {
            byte[] result = new byte[count];

            for (int ix = 0; ix < result.Length; ix++)
                result[ix] = (byte)(ix % count);

            return result;
        }

        private ManualResetEvent StartPerfWorkloads = new ManualResetEvent(false);
        private long TotalByteCount = 0;
        //private long TotalElapsedMs = 0;
        private long TotalElapsedMsLargeReads = 0;

        int writeRecordSize = 4096 - 272;
        int numberWriteRecords = 0x4000;    // 1GB log space
        private async Task Write4KLogFile(
            ILogicalLog lLog
            )
        {
            byte[] writeRecord = new byte[writeRecordSize];

            try
            {
                if (_DedicatedLogOnly)
                {
                    await lLog.ConfigureWritesToOnlyDedicatedLogAsync(CancellationToken.None);
                }


                for (int i = 0; i < numberWriteRecords; i++)
                {
                    for (int j = 0; j < writeRecordSize; j++)
                    {
                        writeRecord[j] = (byte)(i * j);
                    }

                    await lLog.AppendAsync(writeRecord, 0, writeRecord.Length, CancellationToken.None);
                    await lLog.FlushWithMarkerAsync(CancellationToken.None);
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.ToString());
            }
        }


        private async Task LogPerformanceWorkloadWorker(
            ILogicalLog lLog,
            int readRecordSize,
            CancellationToken CancelToken
            )
        {
            int totalWriteBytes = numberWriteRecords * writeRecordSize;
            byte[] readRecord = new byte[readRecordSize];

            //
            // Compute old way
            //
            Stopwatch sw = new Stopwatch();

            int numberReadRecords = (totalWriteBytes / readRecordSize) + 1;

            lLog.SeekForRead(0, System.IO.SeekOrigin.Begin);

            int writtenRecordIndex = 0;
            int writtenRecordOffset = 0;
            long time;

            // Reenable this only to test old style performance
#if false
            sw.Start();
            for (int i = 0; i < numberReadRecords; i++)
            {
                int bytesRead;

                bytesRead = await lLog.ReadAsync(readRecord, 0, readRecordSize, 0, CancellationToken.None);

                for (int j = 0; j < bytesRead; j++)
                {
                    if (readRecord[j] != (byte)(writtenRecordOffset * writtenRecordIndex))
                    {
                        Console.WriteLine("Old Failed validation {0} {1}", writtenRecordIndex, writtenRecordOffset);
                        throw new InvalidDataException();
                    }
                    writtenRecordOffset++;
                    if (writtenRecordOffset == writeRecordSize)
                    {
                        writtenRecordIndex++;
                        writtenRecordOffset = 0;
                    }
                }
            }
            sw.Stop();
            time = sw.ElapsedMilliseconds;
            Interlocked.Add(ref this.TotalByteCount, totalWriteBytes);
            Interlocked.Add(ref this.TotalElapsedMs, time);

            Console.WriteLine("Read with small records {0} ms", time);
#endif

            // Compute New Way
            writtenRecordIndex = 0;
            writtenRecordOffset = 0;

            sw = new Stopwatch();

            lLog.SeekForRead(0, System.IO.SeekOrigin.Begin);

            sw.Start();
            for (int i = 0; i < numberReadRecords; i++)
            {
                int bytesRead;

                bytesRead = await lLog.ReadAsync(readRecord, 0, readRecordSize, readRecordSize, CancellationToken.None);

                for (int j = 0; j < bytesRead; j++)
                {
                    if (readRecord[j] != (byte)(writtenRecordOffset * writtenRecordIndex))
                    {
                        Console.WriteLine("New Failed validation {0} {1}", writtenRecordIndex, writtenRecordOffset);
                        throw new InvalidDataException();
                    }
                    writtenRecordOffset++;
                    if (writtenRecordOffset == writeRecordSize)
                    {
                        writtenRecordIndex++;
                        writtenRecordOffset = 0;
                    }
                }
            }
            sw.Stop();
            time = sw.ElapsedMilliseconds;

            Console.WriteLine("Read with large records {0} ms", time);
            Interlocked.Add(ref this.TotalElapsedMsLargeReads, time);

            return;
        }

        private void StartWorkloadThreads(
            ILogicalLog[] lLog,
            int numberTasks,
            int RecordSize,
            CancellationTokenSource[] CancelToken,
            out float MBPerSecond
            )
        {
            Task[] tasks = new Task[numberTasks];
            TotalByteCount = 0;

            for (int i = 0; i < numberTasks; i++)
            {
                int ii = i;
                tasks[i] = Task.Factory.StartNew(() =>
                    {
                        ILogicalLog logicalLog = lLog[ii];
                        Write4KLogFile(logicalLog).Wait();
                        LogPerformanceWorkloadWorker(logicalLog, RecordSize, CancellationToken.None).Wait();
                    });
            }

            this.StartPerfWorkloads.Set();

            Task.WaitAll(tasks);

            Console.WriteLine("Number Tasks: {0}", numberTasks);
            Console.WriteLine("RecordSize {0}", RecordSize);
            Console.WriteLine("Total bytes {0}", TotalByteCount);

            // renable when old style perf is needed
#if false
            long bytesPerSecond = TotalByteCount / (TotalElapsedMs / 1000);
            float mbPerSecond = (bytesPerSecond / (1024 * 1024));
            Console.WriteLine("Bytes per second:             {0}\nMB per second:             {1}", bytesPerSecond, mbPerSecond);
#endif

            long byteCount = TotalByteCount;
            long bytesPerSecondLargeReads = TotalByteCount / (TotalElapsedMsLargeReads / 1000);

            float mbPerSecondLargeReads = (bytesPerSecondLargeReads / (1024 * 1024));

            Console.WriteLine("Bytes per second Large Reads: {0}\nMB per second Large Reads: {1}", bytesPerSecondLargeReads, mbPerSecondLargeReads);

            MBPerSecond = mbPerSecondLargeReads;
        }

        public void LogPerformanceWorkloadTest(
            out float MBPerSecond
            )
        {
            try
            {
                //* Local functions
                Func<ILogManager>
                MakeManager = (() =>
                {
                    Task<ILogManager> t = LogManager.OpenAsync(LogManager.LoggerType.Default, CancellationToken.None);
                    t.Wait();
                    return t.Result;
                });

                Func<ILogManager, string, Guid, long, uint, uint, IPhysicalLog>
                MakePhyLog = ((
                    ILogManager manager,
                    string pathToCommonContainer,
                    Guid physicalLogId,
                    long containerSize,
                    uint maximumNumberStreams,
                    uint maximumLogicalLogBlockSize) =>
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
                });

                Func<ILogManager, string, Guid, IPhysicalLog>
                OpenPhyLog = ((
                    ILogManager manager,
                    string pathToCommonContainer,
                    Guid physicalLogId) =>
                {
                    Task<IPhysicalLog> t = manager.OpenPhysicalLogAsync(
                        pathToCommonContainer,
                        physicalLogId,
                        false,
                        CancellationToken.None);

                    t.Wait();
                    return t.Result;
                });

                string physLogName = PlatformPathPrefix + _SharedDrive + PlatformPathDelimiter + "PerfWorkloadTestP.log";
                    
                ILogManager logManager = MakeManager();

                try
                {
                    File.Delete(physLogName.Substring(4));
                }
                catch (Exception)
                {
                    // ok if this fails
                }

                Console.WriteLine("Raw Record size is {0}", _RecordSize);
                Guid logId = Guid.NewGuid();
                string logName = physLogName;
                IPhysicalLog phyLog = MakePhyLog(
                    logManager,
                    logName,
                    logId,
                    _LogSize,
                    0,
                    4*_RecordSize);

                //
                // Determine the largest record size. We write full records to maximmize our I/O rate.
                // Max record size is determined by the Maximum block size plus the metadata space (4K) minus the 
                // overhead of the headers.
                //
                //const int MaxLogBlkSize = 1024 * 1024;
                //const int recordSize = 131;


                CancellationTokenSource[] cancelToken = new CancellationTokenSource[_NumberTasks];
                Task[] tasks = new Task[_NumberTasks];
                ILogicalLog[] lLogs = new ILogicalLog[_NumberTasks];
                int bytesToWrite = 1;

                this.StartPerfWorkloads.Reset();

                for (int i = 0; i < _NumberTasks; i++)
                {
                    //
                    // MUltiply record size by 4 so log is created with correct geometry
                    //

                    Guid lLogId = Guid.NewGuid();
                    string logicalLogName = PlatformPathPrefix + _DedicatedDrive + PlatformPathDelimiter + "Perf" + lLogId.ToString() + ".log";
                    lLogs[i] = CreateLogicalLog(phyLog, 
                                                        lLogId, 
                                                        null, 
                                                        logicalLogName, 
                                                        null, 
                                                        _LlogSize, 
                                                        4*_RecordSize);

                    lLogs[i].CloseAsync(CancellationToken.None).Wait();
                    lLogs[i] = null;
                    lLogs[i] = OpenLogicalLog(phyLog, lLogId);

                    //
                    // Harvest the size of a complete buffer to write
                    //
                    bytesToWrite = (int)lLogs[i].MaximumBlockSize;
                    
                }

                //
                // Get a buffer
                //
                Console.WriteLine("Each formatted record contains {0} bytes", bytesToWrite);

                StartWorkloadThreads(lLogs, _NumberTasks, (int)_RecordSize, cancelToken, out MBPerSecond);

                for (int i = 0; i < _NumberTasks; i++)
                {
                    lLogs[i].CloseAsync(CancellationToken.None).Wait();
                    lLogs[i] = null;
                }

                phyLog.CloseAsync(CancellationToken.None).Wait();
                phyLog = null;

                //* Cleanup
                logManager.DeletePhysicalLogAsync(logName, logId, CancellationToken.None).Wait();
                logManager.CloseAsync(CancellationToken.None).Wait();

                try
                {
                    File.Delete(physLogName.Substring(4));
                }
                catch (Exception)
                {
                    // ok if this fails
                }

            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.ToString());
                throw;
            }
        }
    }

    class TestLogNotification
    {
        public bool Fired = false;

        public async Task SetupLogUsageNotificationAsync(ILogicalLog lLog, uint PercentUsage, CancellationToken CancelToken)
        {
            Fired = false;
            await lLog.WaitCapacityNotificationAsync(PercentUsage, CancelToken);
            Fired = true;
        }
    }
}