// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Data.Log;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Data.Testability;
    using Microsoft.Win32.SafeHandles;

    /// <summary>
    /// File implementation of the ILogicalLog interface
    /// </summary>
    internal class FileLogicalLog : ILogicalLog
    {
        private const string LogFileNameSuffix = ".log";

        private readonly string logFileName;
        private readonly LogWriteFaultInjectionParameters logWriteFaultInjectionParameters;

        private bool isDisposed;

        private SafeFileHandle logFileHandle;

        private FileStream logFileStream;

        private Random randomGenerator;

        private FileLogicalLog(
            string logFileName,
            LogWriteFaultInjectionParameters logWriteFaultInjectionParameters)
        {
            this.logFileName = logFileName;
            this.logWriteFaultInjectionParameters = logWriteFaultInjectionParameters;
            this.logFileStream = null;
            this.logFileHandle = null;
            this.isDisposed = false;
            this.randomGenerator = new Random(Environment.TickCount);
        }

        public long HeadTruncationPosition
        {
            get { throw new NotImplementedException(); }
        }

        public bool IsFunctional
        {
            get { return !this.logFileHandle.IsInvalid; }
        }

        public long Length
        {
            get { return this.logFileStream.Length; }
        }

        public long MaximumBlockSize
        {
            get { throw new NotImplementedException(); }
        }

        public uint MetadataBlockHeaderSize
        {
            get { throw new NotImplementedException(); }
        }

        public long ReadPosition
        {
            get { throw new NotImplementedException(); }
        }

        public long WritePosition
        {
            get { return this.logFileStream.Position; }
        }

        public static Task<ILogicalLog> CreateFileLogicalLog(string logFileName)
        {
            return CreateFileLogicalLog(
                logFileName,
                new LogWriteFaultInjectionParameters());
        }

        public static Task<ILogicalLog> CreateFileLogicalLog(
            string logFileName, 
            LogWriteFaultInjectionParameters logWriteFaultInjectionParameters)
        {
            var created = new FileLogicalLog(logFileName, logWriteFaultInjectionParameters);
            created.Open();

            return Task.FromResult(created as ILogicalLog);
        }

        public static string GetFullPathToLog(string folder, string fileName)
        {
            return Path.Combine(folder, fileName + LogFileNameSuffix);
        }

        public void Abort()
        {
            this.Dispose();
        }

        public Task AppendAsync(byte[] buffer, int offset, int count, CancellationToken cancellationToken)
        {
            return this.logFileStream.WriteAsync(buffer, offset, count);
        }

        public Task CloseAsync(CancellationToken cancellationToken)
        {
            this.Dispose();
            return Task.FromResult(0);
        }

        public Task ConfigureWritesToOnlyDedicatedLogAsync(CancellationToken cancellationToken)
        {
            return Task.FromResult(0);
        }

        public Task ConfigureWritesToSharedAndDedicatedLogAsync(CancellationToken cancellationToken)
        {
            return Task.FromResult(0);
        }

        public Stream CreateReadStream(int sequentialAccessReadSize)
        {
            var fileStream = FabricFile.Open(
                this.logFileName,
                FileMode.Open,
                FileAccess.Read,
                FileShare.ReadWrite,
                8*1024,
                FileOptions.Asynchronous | FileOptions.SequentialScan);

            return fileStream;
        }

        public void Dispose()
        {
            if (this.isDisposed)
            {
                return;
            }

            this.isDisposed = true;
            
            if (this.logFileStream != null)
            {
                this.logFileStream.Dispose();
            }

            if (this.logFileHandle != null)
            {
                this.logFileHandle.Dispose();
            }
        }

        public async Task FlushAsync(CancellationToken cancellationToken)
        {
            await this.DelayBeforeFlush(cancellationToken);
            await this.logFileStream.FlushAsync(cancellationToken);
        }

        public async Task FlushWithMarkerAsync(CancellationToken cancellationToken)
        {
            await this.DelayBeforeFlush(cancellationToken);
            await this.logFileStream.FlushAsync(cancellationToken);
        }

        public Task<uint> QueryLogUsageAsync(CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<int> ReadAsync(
            byte[] buffer,
            int offset,
            int count,
            int bytesToRead,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public long SeekForRead(long offset, SeekOrigin origin)
        {
            throw new NotImplementedException();
        }

        public void SetSequentialAccessReadSize(Stream logStream, int sequentialAccessReadSize)
        {
        }

        public void TruncateHead(long streamOffset)
        {
            if (this.logFileStream != null)
            {
                NativeMethods.ZeroFile(this.logFileHandle, (ulong) streamOffset);
            }
        }

        public Task TruncateTail(long streamOffset, CancellationToken cancellationToken)
        {
            this.logFileStream.SetLength(streamOffset);
            this.logFileStream.Position = streamOffset;

            return Task.FromResult(0);
        }

        public Task WaitBufferFullNotificationAsync(CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task WaitCapacityNotificationAsync(uint percentOfSpaceUsed, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        private void Open()
        {
            Utility.Assert(
                (this.logFileHandle == null) || (this.logFileHandle.IsClosed == true),
                "log file handle should either be null or closed");

            this.logFileStream = FabricFile.Open(
                this.logFileName,
                FileMode.OpenOrCreate,
                FileAccess.Write,
                FileShare.Read,
                8*1024,
                FileOptions.Asynchronous | FileOptions.SequentialScan);

            this.logFileHandle = this.logFileStream.SafeFileHandle;
            NativeMethods.SetSparseFileAttribute(this.logFileHandle);
            this.logFileStream.Position = this.logFileStream.Length;
        }

        public async Task DelayBeforeFlush(CancellationToken cancellationToken)
        {
            if (this.logWriteFaultInjectionParameters.Test_LogDelayRatio <= 0) return;
            if (this.randomGenerator.NextDouble() < this.logWriteFaultInjectionParameters.Test_LogDelayRatio)
            {
                var flushDelay = TimeSpan.FromMilliseconds(this.randomGenerator.Next(this.logWriteFaultInjectionParameters.Test_LogMinDelayIntervalMilliseconds, this.logWriteFaultInjectionParameters.Test_LogMaxDelayIntervalMilliseconds));
                FabricEvents.Events.LogManager(this.logFileName, string.Format("Delaying {0} before Flush in FileLogicalLog", flushDelay));
                await Task.Delay(flushDelay);

                if (this.randomGenerator.NextDouble() < this.logWriteFaultInjectionParameters.Test_LogDelayProcessExitRatio)
                {
                    FabricEvents.Events.LogManager(this.logFileName, "Exiting Process due to fault injection");
                    NativeMethods.ExitProcess();
                }
            }
        }
    }
}