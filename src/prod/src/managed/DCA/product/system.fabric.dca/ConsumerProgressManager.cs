// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca.EtlConsumerHelper;
    using System.Fabric.Interop;
    using System.IO;

    internal class ConsumerProgressManager
    {
        private const string BookmarkDirName = "BMk";
        private const string BookmarkFileName = "LastReadFile.dat";
        private const string BookmarkFileFormatVersionString = "Version: 1";

        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        // Tag used to represent the source of the log message
        private readonly string logSourceId;

        // Bookmark folders to hold ETL read progress for fabric and lease logs
        private readonly Dictionary<string, string> bookmarkFolders;

        // Bookmark files last event read position
        private readonly Dictionary<string, long> bookmarkLastEventReadPosition;

        // Method execution initial retry interval in ms
        private readonly int methodExecutionInitialRetryIntervalMs;

        // Method execution max retry count
        private readonly int methodExecutionMaxRetryCount;

        // Method execution max retry interval in ms
        private readonly int methodExecutionMaxRetryIntervalMs;

        public ConsumerProgressManager(
            FabricEvents.ExtensionsEvents traceSource, 
            string logSourceId,
            int methodExecutionInitialRetryIntervalMs,
            int methodExecutionMaxRetryCount,
            int methodExecutionMaxRetryIntervalMs)
        {
            this.traceSource = traceSource;
            this.logSourceId = logSourceId;
            this.methodExecutionInitialRetryIntervalMs = methodExecutionInitialRetryIntervalMs;
            this.methodExecutionMaxRetryCount = methodExecutionMaxRetryCount;
            this.methodExecutionMaxRetryIntervalMs = methodExecutionMaxRetryIntervalMs;
            this.bookmarkFolders = new Dictionary<string, string>();
            this.bookmarkLastEventReadPosition = new Dictionary<string, long>();
        }

        private enum LastEventReadInfoParts
        {
            LastEventTimestampLong,
            LastEventTimestamp,
            LastEventIndex,

            // This is not an actual piece of information in the file, but a value
            // representing the count of the pieces of information we store in the file.
            Count
        }

        internal Dictionary<string, string> BookmarkFolders
        {
            get
            {
                return this.bookmarkFolders;
            }
        }

        internal Dictionary<string, long> BookmarkLastEventReadPosition
        {
            get 
            {
                return this.bookmarkLastEventReadPosition;
            }
        }

        /// <summary>
        /// Creates bookmark folders and files for lease
        /// and fabric traces
        /// </summary>
        /// <param name="workDirectory"></param>
        /// <param name="destinationKey"></param>
        /// <returns></returns>
        internal bool InitializeBookmarkFoldersAndFiles(string workDirectory, string destinationKey)
        {
            return this.InitializeBookmarkFolders(workDirectory, destinationKey) && this.InitializeBookmarkFiles();
        }

        /// <summary>
        /// Creates the bookmark file if it does not exist.
        /// </summary>
        /// <param name="bookmarkFolder"></param>
        /// <param name="bytesWritten"></param>
        /// <returns></returns>
        internal bool CreateBookmarkFile(
            string bookmarkFolder, 
            out long bytesWritten)
        {
            FileStream fs = null;
            StreamWriter writer = null;
            bytesWritten = 0;
            string bookmarkFile = Path.Combine(bookmarkFolder, BookmarkFileName);

            if (true == FabricFile.Exists(bookmarkFile))
            {
                return true;
            }

            try
            {
                // Create the bookmark file
                try
                {
                    Utility.PerformIOWithRetries(
                        () =>
                        {
                            fs = new FileStream(bookmarkFile, FileMode.Create);
                            writer = new StreamWriter(fs);
                        },
                        this.methodExecutionInitialRetryIntervalMs,
                        this.methodExecutionMaxRetryCount,
                        this.methodExecutionMaxRetryIntervalMs);
                }
                catch (Exception e)
                {
                    this.traceSource.WriteExceptionAsError(
                        this.logSourceId,
                        e,
                        "Unable to create bookmark file {0} while processing events.",
                        bookmarkFile);
                    return false;
                }

                long localBytesWritten = 0;

                // Write the version information
                try
                {
                    Utility.PerformIOWithRetries(
                        () =>
                        {
                            writer.WriteLine(BookmarkFileFormatVersionString);
                            localBytesWritten += BookmarkFileFormatVersionString.Length + 2;
                            writer.Flush();
                        },
                        this.methodExecutionInitialRetryIntervalMs,
                        this.methodExecutionMaxRetryCount,
                        this.methodExecutionMaxRetryIntervalMs);
                }
                catch (Exception e)
                {
                    this.traceSource.WriteExceptionAsError(
                        this.logSourceId,
                        e,
                        "Unable to write version information in bookmark file {0} while processing events.",
                        bookmarkFile);
                    return false;
                }

                // Write the timestamp and index of the last event read and close the file
                int lastEventIndex = -1;
                string lastEventReadString = string.Format("{0},{1},{2}", DateTime.MinValue.ToBinary(), DateTime.MinValue.ToString(), lastEventIndex.ToString());
                try
                {
                    Utility.PerformIOWithRetries(
                        () =>
                        {
                            writer.WriteLine(lastEventReadString);
                            localBytesWritten += lastEventReadString.Length + 2;
                            writer.Dispose();
                            fs.Dispose();
                        },
                        this.methodExecutionInitialRetryIntervalMs,
                        this.methodExecutionMaxRetryCount,
                        this.methodExecutionMaxRetryIntervalMs);
                }
                catch (Exception e)
                {
                    this.traceSource.WriteExceptionAsError(
                        this.logSourceId,
                        e,
                        "Unable to write last event read {0} in bookmark file {1} while processing events.",
                        lastEventReadString,
                        bookmarkFile);
                    return false;
                }

                // record bytes written
                bytesWritten = localBytesWritten;
            }
            finally
            {
                if (null != writer)
                {
                    writer.Dispose();
                }

                if (null != fs)
                {
                    fs.Dispose();
                }
            }

            return true;
        }

        /// <summary>
        /// Gets the file position where last event index is recorded.
        /// </summary>
        /// <param name="bookmarkFolder"></param>
        /// <param name="lastEventReadPosition"></param>
        /// <param name="bytesRead"></param>
        /// <returns></returns>
        internal bool GetLastEventReadPosition(
            string bookmarkFolder,
            out long lastEventReadPosition,
            out long bytesRead)
        {
            lastEventReadPosition = 0;
            bytesRead = 0;

            string bookmarkFile = Path.Combine(bookmarkFolder, BookmarkFileName);
            if (false == FabricFile.Exists(bookmarkFile))
            {
                // Bookmark file doesn't exist
                return false;
            }

            StreamReader reader = null;
            try
            {
                // Open the file
                try
                {
                    Utility.PerformIOWithRetries(
                        () =>
                        {
                            FileStream fileStream = FabricFile.Open(bookmarkFile, FileMode.Open, FileAccess.Read);
                            Helpers.SetIoPriorityHint(fileStream.SafeFileHandle, Kernel32Types.PRIORITY_HINT.IoPriorityHintVeryLow);
                            reader = new StreamReader(fileStream);
                        },
                        this.methodExecutionInitialRetryIntervalMs,
                        this.methodExecutionMaxRetryCount,
                        this.methodExecutionMaxRetryIntervalMs);
                }
                catch (Exception e)
                {
                    this.traceSource.WriteExceptionAsError(
                        this.logSourceId,
                        e,
                        "Unable to open bookmark file {0}.",
                        bookmarkFile);
                    return false;
                }

                long localBytesRead = 0;
                long streamPosition = 0;

                // Get the version
                string versionString = string.Empty;
                try
                {
                    Utility.PerformIOWithRetries(
                        () =>
                        {
                            versionString = reader.ReadLine();
                            localBytesRead += versionString.Length + 2;
                            streamPosition = localBytesRead;
                        },
                        this.methodExecutionInitialRetryIntervalMs,
                        this.methodExecutionMaxRetryCount,
                        this.methodExecutionMaxRetryIntervalMs);
                }
                catch (Exception e)
                {
                    this.traceSource.WriteExceptionAsError(
                        this.logSourceId,
                        e,
                        "Unable to read version information from bookmark file {0}.",
                        bookmarkFile);
                    return false;
                }

                // Check the version
                if (false == versionString.Equals(BookmarkFileFormatVersionString, StringComparison.Ordinal))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Unexpected version string {0} encountered in bookmark file {1}.",
                        versionString,
                        bookmarkFile);
                    return false;
                }

                // Get information about the last event that we read
                string infoLine = string.Empty;
                try
                {
                    Utility.PerformIOWithRetries(
                        () =>
                        {
                            infoLine = reader.ReadLine();
                            localBytesRead += infoLine.Length + 2;
                        },
                        this.methodExecutionInitialRetryIntervalMs,
                        this.methodExecutionMaxRetryCount,
                        this.methodExecutionMaxRetryIntervalMs);
                }
                catch (Exception e)
                {
                    this.traceSource.WriteExceptionAsError(
                        this.logSourceId,
                        e,
                        "Unable to read information about last event read from bookmark file {0}.",
                        bookmarkFile);
                    return false;
                }

                // record bytes read
                bytesRead = localBytesRead;

                lastEventReadPosition = streamPosition;
            }
            finally
            {
                if (null != reader)
                {
                    reader.Dispose();
                }
            }

            return true;
        }

        /// <summary>
        /// Retrieves the last event index processed.
        /// </summary>
        /// <param name="bookmarkFolder"></param>
        /// <param name="bytesRead"></param>
        /// <returns></returns>
        internal EventIndex ReadBookmarkFile(
            string bookmarkFolder,
            out long bytesRead)
        {
            bytesRead = 0;
            EventIndex lastEventIndex = new EventIndex();
            lastEventIndex.Set(DateTime.MinValue, -1);

            string bookmarkFile = Path.Combine(bookmarkFolder, BookmarkFileName);
            if (false == FabricFile.Exists(bookmarkFile))
            {
                // Bookmark file doesn't exist
                return lastEventIndex;
            }

            StreamReader reader = null;
            try
            {
                // Open the file
                try
                {
                    Utility.PerformIOWithRetries(
                        () =>
                        {
                            FileStream fileStream = FabricFile.Open(bookmarkFile, FileMode.Open, FileAccess.Read);
                            Helpers.SetIoPriorityHint(fileStream.SafeFileHandle, Kernel32Types.PRIORITY_HINT.IoPriorityHintVeryLow);
                            reader = new StreamReader(fileStream);
                        },
                        this.methodExecutionInitialRetryIntervalMs,
                        this.methodExecutionMaxRetryCount,
                        this.methodExecutionMaxRetryIntervalMs);
                }
                catch (Exception e)
                {
                    this.traceSource.WriteExceptionAsError(
                        this.logSourceId,
                        e,
                        "Unable to open bookmark file {0}.",
                        bookmarkFile);
                    return lastEventIndex;
                }

                long localBytesRead = 0;

                // Get the version
                string versionString = string.Empty;
                try
                {
                    Utility.PerformIOWithRetries(
                        () =>
                        {
                            versionString = reader.ReadLine();
                            localBytesRead += versionString.Length + 2;
                        },
                        this.methodExecutionInitialRetryIntervalMs,
                        this.methodExecutionMaxRetryCount,
                        this.methodExecutionMaxRetryIntervalMs);
                }
                catch (Exception e)
                {
                    this.traceSource.WriteExceptionAsError(
                        this.logSourceId,
                        e,
                        "Unable to read version information from bookmark file {0}.",
                        bookmarkFile);
                    return lastEventIndex;
                }

                // Check the version
                if (false == versionString.Equals(BookmarkFileFormatVersionString, StringComparison.Ordinal))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Unexpected version string {0} encountered in bookmark file {1}.",
                        versionString,
                        bookmarkFile);
                    return lastEventIndex;
                }

                // Get information about the last event that we read
                string infoLine = string.Empty;
                try
                {
                    Utility.PerformIOWithRetries(
                        () =>
                        {
                            infoLine = reader.ReadLine();
                            localBytesRead += infoLine.Length + 2;
                        },
                        this.methodExecutionInitialRetryIntervalMs,
                        this.methodExecutionMaxRetryCount,
                        this.methodExecutionMaxRetryIntervalMs);
                }
                catch (Exception e)
                {
                    this.traceSource.WriteExceptionAsError(
                        this.logSourceId,
                        e,
                        "Unable to read information about last event read from bookmark file {0}.",
                        bookmarkFile);
                    return lastEventIndex;
                }

                string[] infoLineParts = infoLine.Split(',');
                if (infoLineParts.Length != (int)LastEventReadInfoParts.Count)
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "The information in bookmark file {0} about the last event read is not in the expected format. {1}",
                        bookmarkFile,
                        infoLine);
                    return lastEventIndex;
                }

                string lastEventTimestampString = infoLineParts[(int)LastEventReadInfoParts.LastEventTimestampLong].Trim();
                long lastEventTimestampBinary;
                if (false == long.TryParse(lastEventTimestampString, out lastEventTimestampBinary))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Unable to retrieve timestamp of last event from bookmark file {0}.",
                        bookmarkFile);
                    return lastEventIndex;
                }

                DateTime lastEventTimestamp = DateTime.FromBinary(lastEventTimestampBinary);

                string lastEventTimestampDifferentiatorString = infoLineParts[(int)LastEventReadInfoParts.LastEventIndex].Trim();
                int lastEventTimestampDifferentiator;
                if (false == int.TryParse(lastEventTimestampDifferentiatorString, out lastEventTimestampDifferentiator))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Unable to retrieve timestamp differentiator of last event from bookmark file {0}.",
                        bookmarkFile);
                    return lastEventIndex;
                }

                // record bytes read
                bytesRead = localBytesRead;

                lastEventIndex.Set(lastEventTimestamp, lastEventTimestampDifferentiator);
            }
            finally
            {
                if (null != reader)
                {
                    reader.Dispose();
                }
            }

            return lastEventIndex;
        }

        /// <summary>
        /// Updates the bookmark file with the last event read index.
        /// </summary>
        /// <param name="bookmarkFolder"></param>
        /// <param name="lastReadEventIndexPosition"></param>
        /// <param name="eventIndex"></param>
        /// <param name="bytesWritten"></param>
        /// <returns></returns>
        internal bool UpdateBookmarkFile(
            string bookmarkFolder,
            long lastReadEventIndexPosition,
            EventIndex eventIndex,
            out long bytesWritten)
        {
            bytesWritten = 0;
            FileStream fs = null;
            StreamWriter writer = null;
            string bookmarkFile = Path.Combine(bookmarkFolder, BookmarkFileName);

            if (false == FabricFile.Exists(bookmarkFile))
            {
                return false;
            }

            try
            {
                // Open the bookmark file
                try
                {
                    Utility.PerformIOWithRetries(
                        () =>
                        {
                            fs = new FileStream(bookmarkFile, FileMode.Open);
                            writer = new StreamWriter(fs);
                        },
                        this.methodExecutionInitialRetryIntervalMs,
                        this.methodExecutionMaxRetryCount,
                        this.methodExecutionMaxRetryIntervalMs);
                }
                catch (Exception e)
                {
                    this.traceSource.WriteExceptionAsError(
                        this.logSourceId,
                        e,
                        "Unable to open bookmark file {0} while processing events.",
                        bookmarkFile);
                    return false;
                }

                // Move the stream position to the point where the event index needs to be written
                fs.Position = lastReadEventIndexPosition;

                long localBytesWritten = 0;

                // Write the timestamp and index of the last event read and close the file
                string lastEventReadString = string.Format("{0},{1},{2}", eventIndex.Timestamp.ToBinary(), eventIndex.Timestamp.ToString(), eventIndex.TimestampDifferentiator);
                try
                {
                    Utility.PerformIOWithRetries(
                        () =>
                        {
                            writer.WriteLine(lastEventReadString);
                            localBytesWritten = lastEventReadString.Length + 2;
                            writer.Dispose();
                            fs.Dispose();
                        },
                        this.methodExecutionInitialRetryIntervalMs,
                        this.methodExecutionMaxRetryCount,
                        this.methodExecutionMaxRetryIntervalMs);
                }
                catch (Exception e)
                {
                    this.traceSource.WriteExceptionAsError(
                        this.logSourceId,
                        e,
                        "Unable to write last event read {0} in bookmark file {1} while processing events.",
                        lastEventReadString,
                        bookmarkFile);
                    return false;
                }

                // record bytes written
                bytesWritten = localBytesWritten;
            }
            finally
            {
                if (null != writer)
                {
                    writer.Dispose();
                }

                if (null != fs)
                {
                    fs.Dispose();
                }
            }

            return true;
        }

        /// <summary>
        /// Create bookmark folders for lease and fabric traces.
        /// </summary>
        /// <param name="workDirectory"></param>
        /// <param name="destinationKey"></param>
        /// <returns></returns>
        private bool InitializeBookmarkFolders(string workDirectory, string destinationKey)
        {
            // initialize fabric bookmark folder
            string fabricBookmarkFolder = string.Empty;
            if (this.CreateBookmarkSubDirectory(
                EtlToInMemoryBufferWriter.FabricTracesFolderName,
                workDirectory,
                destinationKey,
                out fabricBookmarkFolder))
            {
                this.bookmarkFolders.Add(EtlToInMemoryBufferWriter.FabricTracesFolderName, fabricBookmarkFolder);
            }

            // initialize lease bookmark folder
            string leaseBookmarkFolder = string.Empty;
            if (this.CreateBookmarkSubDirectory(
                EtlToInMemoryBufferWriter.LeaseTracesFolderName,
                workDirectory,
                destinationKey,
                out leaseBookmarkFolder))
            {
                this.bookmarkFolders.Add(EtlToInMemoryBufferWriter.LeaseTracesFolderName, leaseBookmarkFolder);
            }

            return this.bookmarkFolders.Count == 2;
        }

        /// <summary>
        /// Creates bookmark files for lease and fabric traces
        /// and records the last event index file position.
        /// </summary>
        /// <returns></returns>
        private bool InitializeBookmarkFiles()
        {
            foreach (var kv in this.bookmarkFolders)
            {
                long bytesWritten = 0;
                bool success = this.CreateBookmarkFile(
                    this.bookmarkFolders[kv.Key],
                    out bytesWritten);

                if (success)
                {
                    long lastEventReadPosition = 0;
                    long bytesRead = 0;
                    success = this.GetLastEventReadPosition(
                        this.bookmarkFolders[kv.Key],
                        out lastEventReadPosition,
                        out bytesRead);

                    if (success)
                    {
                        this.bookmarkLastEventReadPosition.Add(kv.Key, lastEventReadPosition);
                    }
                    else
                    {
                        this.traceSource.WriteWarning(
                        this.logSourceId,
                        "Bookmark last event index file position could not be retrieved for trace file sub folder {0}.",
                        kv.Key);
                    }
                }
                else
                {
                    this.traceSource.WriteWarning(
                        this.logSourceId,
                        "Bookmark file not created for trace file sub folder {0}.",
                        kv.Key);
                }
            }

            return this.bookmarkFolders.Count == this.bookmarkLastEventReadPosition.Count;
        }

        /// <summary>
        /// Creates bookmark directory.
        /// </summary>
        /// <param name="sourceFolder"></param>
        /// <param name="workDirectory"></param>
        /// <param name="destinationKey"></param>
        /// <param name="bookmarkFolder"></param>
        /// <returns></returns>
        private bool CreateBookmarkSubDirectory(
            string sourceFolder,
            string workDirectory,
            string destinationKey,
            out string bookmarkFolder)
        {
            string bookmarkParentFolder = Path.Combine(
                workDirectory,
                Utility.ShortWindowsFabricIdForPaths);

            bool success = Utility.CreateWorkSubDirectory(
                this.traceSource,
                this.logSourceId,
                destinationKey,
                sourceFolder,
                bookmarkParentFolder,
                out bookmarkFolder);

            if (success)
            {
                bookmarkFolder = Path.Combine(bookmarkFolder, BookmarkDirName);
            }

            FabricDirectory.CreateDirectory(bookmarkFolder);
            return success;
        }
    }
}