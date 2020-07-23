// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace LogLogPerformance
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
        public int _TimeToRunInSeconds = 5 * 60;
        public const uint _RecordOverhead = 4 * 422; // 1088;
        public uint _MagicRecordSize = 1024 * 1024;  // 1032192;  // This is 4 complete records Why 16K short of 1MB ?
        public uint _RecordSize;
        public int _NumberTasks = 4;
        public long _LogSize = 8 * 1024L * 1024L * 1024L;
        public long _LlogSize = 2 * 1024L * 1024L * 1024L;
        public bool _DedicatedLogOnly = true;
        public bool _AutomaticMode = false;
#if DotNetCoreClrLinux
        private static readonly string PlatformPathPrefix = "";
        private static readonly string PathSeparator = "/";
#else
        private static readonly string PlatformPathPrefix = "\\??\\";
        private static readonly string PathSeparator = "\\";
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
            //string systemDrive = Environment.GetEnvironmentVariable("SYSTEMDRIVE");
            //                    string eDrive = "e:";
            //                    string dDrive = "d:";
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
                        const int automationBlockCount = 5;
                    uint[] automationBlockSizes = new uint[automationBlockCount] { 64, 128, 256, 512, 1024 };
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
            int count)
        {
            if (readOffset != -1)
            {
                logicalLog.SeekForRead(readOffset, SeekOrigin.Begin);
            }

            Task<int> t = logicalLog.ReadAsync(buffer, bufferOffset, count, 0, CancellationToken.None);

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

        private async Task LogPerformanceWorkloadWorker(
            ILogicalLog lLog,
            byte[] recordBuffer,
            TestLogNotification tn75,
            CancellationToken tn75CancelToken,
            CancellationToken CancelToken
            )
        {
            try
            {
                Task taskUsageNotification75;
                long bytes = 0;
                taskUsageNotification75 = tn75.SetupLogUsageNotificationAsync(lLog, 75, tn75CancelToken);

                if (_DedicatedLogOnly)
                {
                    await lLog.ConfigureWritesToOnlyDedicatedLogAsync(CancellationToken.None);
                }

                bool canceled = false;
                this.StartPerfWorkloads.WaitOne();

                while (!canceled)
                {
                    await lLog.AppendAsync(recordBuffer, 0, 40, /*recordBuffer.Length,*/ CancellationToken.None);
                    bytes += 40; /*recordBuffer.Length;*/

                    if (tn75.Fired)
                    {
                        //
                        // Truncate back to 10%
                        //
                        long allowedData = lLog.Length / 10;
                        long truncationPosition = bytes - allowedData;
                        lLog.TruncateHead(truncationPosition);
                        taskUsageNotification75 = tn75.SetupLogUsageNotificationAsync(lLog, 75, tn75CancelToken);
                    }

                    if (CancelToken.IsCancellationRequested)
                    {
                        canceled = true;
                    }
                }
                await lLog.FlushWithMarkerAsync(CancellationToken.None);
                Interlocked.Add(ref this.TotalByteCount, bytes);
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.ToString());
            }

            Console.WriteLine("Tasks completed");

            return;
        }

        private void StartWorkloadThreads(
            ILogicalLog[] lLog,
            byte[] recordBuffer,
            TestLogNotification[] tn75,
            CancellationTokenSource[] tn75CancelToken,
            CancellationTokenSource[] CancelToken,
            out float MBPerSecond
            )
        {
            int numberTasks = tn75.Length;
            Task[] tasks = new Task[numberTasks];

            TotalByteCount = 0;

            for (int i = 0; i < numberTasks; i++)
            {
                int ii = i;
                tasks[i] = Task.Factory.StartNew(() =>
                    {
                        ILogicalLog logicalLog = lLog[ii];
                        TestLogNotification tn75a = tn75[ii];
                        CancellationToken tn75ct = tn75CancelToken[ii].Token;
                        CancellationToken ct = CancelToken[ii].Token;
                        LogPerformanceWorkloadWorker(logicalLog, recordBuffer, tn75a, tn75ct, ct).Wait();
                    });
            }

            long StartTicks = DateTime.Now.Ticks;
            this.StartPerfWorkloads.Set();
            Thread.CurrentThread.Join(TimeSpan.FromSeconds(_TimeToRunInSeconds));

            for (int i = 0; i < numberTasks; i++)
            {
                try
                {
                    tn75CancelToken[i].Cancel();
                }
                catch
                {
                }
                CancelToken[i].Cancel();
            }

            Task.WaitAll(tasks);
            
            long byteCount = TotalByteCount;
            long EndTicks = DateTime.Now.Ticks;

            long totalTimeInSeconds = (EndTicks - StartTicks) / 10000000;
            long bytesPerSecond = TotalByteCount / totalTimeInSeconds;

            float mbPerSecond = (bytesPerSecond / (1024 * 1024));
            Console.WriteLine("Number Tasks: {0}", numberTasks);
            Console.WriteLine("Seconds running {0}", totalTimeInSeconds );
            Console.WriteLine("RecordSize {0}", recordBuffer.Length);
            Console.WriteLine("Total bytes {0}", TotalByteCount);
            Console.WriteLine("Bytes per second: {0}\nMB per second: {1}", bytesPerSecond, mbPerSecond);

            MBPerSecond = mbPerSecond;
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

                string physLogName = PlatformPathPrefix + _SharedDrive + PathSeparator + "PerfWorkloadTestP.log";

                Console.WriteLine("Physical Log: {0}", physLogName);
                    
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
                TestLogNotification[] tn75 = new TestLogNotification[_NumberTasks];
                CancellationTokenSource[] cancelTokenSource75 = new CancellationTokenSource[_NumberTasks];
                int bytesToWrite = 1;

                this.StartPerfWorkloads.Reset();

                for (int i = 0; i < _NumberTasks; i++)
                {
                    //
                    // MUltiply record size by 4 so log is created with correct geometry
                    //

                    Guid lLogId = Guid.NewGuid();
                    string logicalLogName = PlatformPathPrefix + _DedicatedDrive + PathSeparator + "Perf" + lLogId.ToString() + ".log";
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
                    
                    //
                    // Truncate at 75%
                    //

                    cancelToken[i] = new CancellationTokenSource();
                    CancellationToken token = cancelToken[i].Token;

                    tn75[i] = new TestLogNotification();
                    cancelTokenSource75[i] = new CancellationTokenSource();
                }

                //
                // Get a buffer
                //
                Console.WriteLine("Each formatted record contains {0} bytes", bytesToWrite);
                byte[] result = new byte[(uint)bytesToWrite];

                for (int ix = 0; ix < result.Length; ix++)
                {
                    result[ix] = (byte)(ix % 131);
                }

                StartWorkloadThreads(lLogs, result, tn75, cancelTokenSource75, cancelToken, out MBPerSecond);

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