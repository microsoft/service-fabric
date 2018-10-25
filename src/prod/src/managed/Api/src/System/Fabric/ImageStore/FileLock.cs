// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.ImageStore
{
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.IO;
    using System.Diagnostics;
    using System.Threading;
    using System.Fabric.Strings;

    internal class FileLock : IDisposable
    {
        public const string ReaderLockExtension = ".ReaderLock";
        public const string WriterLockExtension = ".WriterLock";
        private const string TraceTag = "ManagedFileLock";
        // scope has to be defined because Trace is defined for both System.Fabric.Common.Tracing and System.Diagnostics
        private FabricEvents.ExtensionsEvents traceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.ImageStoreClient);

        private FileStream writerStream;
        private FileStream readerStream;
        private string path;
        private bool isReaderLock;
        private string writerLockPath;
        private string readerLockPath;        
        private string operation;

        public FileLock(string path, bool isReaderLock)
        {
            this.path = path;            
            this.isReaderLock = isReaderLock;
            this.writerLockPath = this.path + FileLock.WriterLockExtension;
            this.readerLockPath = this.path + FileLock.ReaderLockExtension;            
            this.operation = this.isReaderLock ? "reader" : "writer";
        }
        
        public static bool DoesWriterLockExist(string path)
        {
            string writerLockPath = path + FileLock.WriterLockExtension;
            return FabricFile.Exists(writerLockPath);
        }

        public bool Acquire()
        {
            Exception exception = null;
            FileStream localReaderStream = null;
            FileStream localWriterStream = null;
            try
            {
                if (this.isReaderLock)
                {
#if DotNetCoreClrLinux
                    this.CreateDirectoryPath(this.readerLockPath);
                    localReaderStream = File.Open(this.readerLockPath, FileMode.OpenOrCreate, FileAccess.Read, FileShare.Read);
#else
                    localReaderStream = FabricFile.Open(this.readerLockPath, FileMode.OpenOrCreate, FileAccess.Read, FileShare.Read);
#endif
                    if (FabricFile.Exists(this.writerLockPath))
                    {
                        this.traceSource.WriteWarning(
                            TraceTag,
                            "Failed to acquire reader lock for {0} since writer lock is found. This could indicate that the file is being written or corrupted.",
                            this.path);

                        throw new FabricImageStoreException(
                            StringResources.Error_CorruptedImageStoreObjectFound,
                            FabricErrorCode.CorruptedImageStoreObjectFound);
                    }
                }
                else
                {
#if DotNetCoreClrLinux
                    this.CreateDirectoryPath(this.readerLockPath);
                    localReaderStream = File.Open(this.readerLockPath, FileMode.OpenOrCreate, FileAccess.Write, FileShare.None);
                    this.CreateDirectoryPath(this.writerLockPath);
                    localWriterStream = File.Open(this.writerLockPath, FileMode.OpenOrCreate, FileAccess.Write, FileShare.None);
#else
                    localReaderStream = FabricFile.Open(this.readerLockPath, FileMode.OpenOrCreate, FileAccess.Write, FileShare.None);
                    localWriterStream = FabricFile.Open(this.writerLockPath, FileMode.OpenOrCreate, FileAccess.Write, FileShare.None);
#endif
                }
            }
            catch (Exception e)
            {
                exception = e;
            }

            if (exception == null)
            {
                this.readerStream = localReaderStream;
                this.writerStream = localWriterStream;

                this.traceSource.WriteInfo(TraceTag, "Obtained {0} lock for {1}", this.operation, this.path);
                return true;
            }
            else
            {
                if (localReaderStream != null)
                {
                    localReaderStream.Dispose();
                }

                if (localWriterStream != null)
                {
                    localWriterStream.Dispose();
                }

                this.traceSource.WriteWarning(
                    TraceTag,
                    "Failed to acquire {0} lock for {1}. Error = {2}",
                    this.operation,
                    this.path,
                    exception);

                return false;
            }
        }

        public bool Acquire(TimeSpan timeout)
        {
            Stopwatch stopWatch = new Stopwatch();
            bool success;
            
            do
            {
                success = Acquire();
                if (!success && stopWatch.Elapsed < timeout)
                {
                    Thread.Sleep(100);
                }
            } 
            while (!success && stopWatch.Elapsed < timeout);

            return success;
        }

        public bool Release()
        {            
            if (this.readerStream != null)
            {             
                try
                {
                    this.readerStream.Dispose();
                    this.readerStream = null;
                    FabricFile.Delete(this.readerLockPath, deleteReadonly: true);
                }
                catch (Exception e)
                {
                    this.traceSource.WriteInfo(TraceTag, "Unable to delete {0} because of {1}", this.readerLockPath, e);
                }
            }

            if (this.writerStream != null)
            {
                this.writerStream.Dispose();
                this.writerStream = null;
                FabricFile.Delete(this.writerLockPath, deleteReadonly: true);
            }

            this.traceSource.WriteInfo(TraceTag, "Released {0} lock on {1}", this.operation, this.path);

            return true;
        }

        public void Dispose()
        {
            this.Release();
            GC.SuppressFinalize(this);
        }

#if DotNetCoreClrLinux
        private void CreateDirectoryPath(string path)
        {
            string directoryName = FabricPath.GetDirectoryName(path);
            if ((directoryName != null) && (directoryName.Length > 0) && (!FabricDirectory.Exists(directoryName)))
            {
                FabricDirectory.CreateDirectory(directoryName);
            }
        }
#endif
    }
}