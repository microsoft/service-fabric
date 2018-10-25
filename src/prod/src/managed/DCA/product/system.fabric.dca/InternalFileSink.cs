// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca.EtlConsumerHelper
{
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Interop;
    using System.IO;

    using Utility = System.Fabric.Dca.Utility;

    internal class InternalFileSink
    {
        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        // Tag used to represent the source of the log message
        private readonly string logSourceId;

        private StreamWriter fileStream;
        private string tempFileName;
        private Statistics writeStatistics = new Statistics();

        internal InternalFileSink(FabricEvents.ExtensionsEvents traceSource, string logSourceId)
        {
            this.traceSource = traceSource;
            this.logSourceId = logSourceId;
        }

        internal string TempFileName
        {
            get
            {
                return this.tempFileName;
            }
        }

        internal Statistics WriteStatistics
        {
            get
            {
                return this.writeStatistics;
            }
        }

        internal static void CopyFile(string source, string destination, bool overwrite, bool compressDestination)
        {
            string tempArchivePath = string.Empty;
            try
            {
                if (compressDestination)
                {
                    string archiveEntryName = Path.GetFileNameWithoutExtension(destination);
                    FileCompressor.Compress(source, archiveEntryName, out tempArchivePath);

                    source = tempArchivePath;
                }

                CopyFileParameters copyParam = new CopyFileParameters()
                {
                    Source = source,
                    Destination = destination,
                    Overwrite = true
                };

                Utility.PerformIOWithRetries(
                    CopyFileWorker,
                    (object)copyParam);
            }
            finally
            {
                if (false == string.IsNullOrEmpty(tempArchivePath))
                {
                    Utility.PerformIOWithRetries(
                        () => { FabricFile.Delete(tempArchivePath); });
                }
            }
        }

        internal static void GetFilesMatchingPattern(string directory, string pattern, out string[] files)
        {
            PatternMatchingParameters patternMatchingParam = new PatternMatchingParameters
            {
                Directory = directory,
                Pattern = pattern
            };
            Utility.PerformIOWithRetries(
                ctx =>
                {
                    PatternMatchingParameters param = ctx;
                    param.Files = FabricDirectory.GetFiles(
                        param.Directory,
                        param.Pattern);
                },
                patternMatchingParam);

            files = patternMatchingParam.Files;
        }

        internal static bool FileExists(string fileName)
        {
            return FabricFile.Exists(fileName);
        }

        internal void Initialize()
        {
            this.tempFileName = Utility.GetTempFileName();
            FileStream tmpFileStream = FabricFile.Open(this.tempFileName, FileMode.Create, FileAccess.Write);
#if !DotNetCoreClrLinux
            Helpers.SetIoPriorityHint(tmpFileStream.SafeFileHandle, Kernel32Types.PRIORITY_HINT.IoPriorityHintVeryLow);
#endif
            this.fileStream = new StreamWriter(tmpFileStream);
            this.writeStatistics = new Statistics();
        }

        internal void Close()
        {
            try
            {
                Utility.PerformIOWithRetries(
                    () =>
                    {
                        try
                        {
                            this.fileStream.Dispose();
                        }
                        catch (System.Text.EncoderFallbackException)
                        {
                            // This can happen if the manifest file does not match the binary.
                            // Write an error message and move on.
                            this.traceSource.WriteError(
                                this.logSourceId,
                                "EncoderFallbackException occurred while closing file {0}",
                                this.TempFileName);
                        }
                    });
            }
            catch (Exception e)
            {
                this.traceSource.WriteExceptionAsError(
                    this.logSourceId,
                    e,
                    "Failed to close handle to temporary file {0}.",
                    this.TempFileName);
            }
        }

        internal void Delete()
        {
            try
            {
                Utility.PerformIOWithRetries(
                    () =>
                    {
                        FabricFile.Delete(this.TempFileName);
                    });
            }
            catch (Exception e)
            {
                this.traceSource.WriteExceptionAsError(
                    this.logSourceId,
                    e,
                    "Failed to delete temporary file {0}.",
                    this.TempFileName);
            }
        }

        internal void WriteEvent(string eventString)
        {
            Utility.PerformIOWithRetries(
                this.WriteEventWorker,
                eventString);
        }

        private static void CopyFileWorker(object context)
        {
            CopyFileParameters copyParam = (CopyFileParameters)context;
            FabricFile.Copy(copyParam.Source, copyParam.Destination, copyParam.Overwrite);
        }

        private void WriteEventWorker(string eventString)
        {
            try
            {
                this.fileStream.WriteLine(eventString);
                this.WriteStatistics.EventsWritten++;
            }
            catch (System.Text.EncoderFallbackException)
            {
                this.WriteStatistics.EncoderErrors++;
            }
        }

        public class Statistics
        {
            public int EventsWritten { get; set; }

            public int EncoderErrors { get; set; }
        }

        private class PatternMatchingParameters
        {
            internal string Directory { get; set; }

            internal string Pattern { get; set; }

            internal string[] Files { get; set; }
        }

        private class CopyFileParameters
        {
            internal string Source { get; set; }

            internal string Destination { get; set; }

            internal bool Overwrite { get; set; }
        }
    }
}