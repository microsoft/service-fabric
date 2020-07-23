// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.IO;

    using Utility = System.Fabric.Dca.Utility;

    internal class CheckpointManager
    {
        private readonly FabricEvents.ExtensionsEvents traceSource;
        private readonly string logSourceId;
        private readonly string lastEtlFileReadFileDirectoryPath;

        public CheckpointManager(
            bool isReadingFromApplicationManifest, 
            string etlPath, 
            string logDirectory, 
            string traceDirectory, 
            string producerInstanceId,
            FabricEvents.ExtensionsEvents traceSource,
            string logSourceId)
        {
            this.traceSource = traceSource;
            this.logSourceId = logSourceId;

            // Directory where the producer saves the file containing information 
            // about the point upto which we last read the ETL files
            this.lastEtlFileReadFileDirectoryPath = InitializeLastEtlFileReadFilePath(
                isReadingFromApplicationManifest,
                etlPath,
                logDirectory,
                traceDirectory,
                producerInstanceId);

            // If the directory containing the last ETL read information is
            // different from the log directory, then create it now.
            if (false == this.lastEtlFileReadFileDirectoryPath.Equals(
                logDirectory))
            {
                FabricDirectory.CreateDirectory(this.lastEtlFileReadFileDirectoryPath);

                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Directory containing last ETL read info: {0}",
                    this.lastEtlFileReadFileDirectoryPath);
            }
        }

        public bool GetLastEndTime(string lastReadFileName, out DateTime lastEndTime)
        {
            lastEndTime = DateTime.MinValue;
            var fileFullPath = Path.Combine(this.lastEtlFileReadFileDirectoryPath, lastReadFileName);

            var fileParam = new LastEndTimeFileParameters(this.traceSource, this.logSourceId)
            {
                Fs = null,
                FileName = fileFullPath,
                Mode = FileMode.Open,
                Access = FileAccess.Read
            };

            // Try to open the file that contains the last ETL read timestamp
            try
            {
                Utility.PerformIOWithRetries(
                    fileParam.OpenLastEndTimeFile);
            }
            catch (Exception e)
            {
                this.traceSource.WriteExceptionAsError(
                    this.logSourceId,
                    e,
                    "Failed to open file {0} for read.",
                    fileFullPath);
                return false;
            }

            if (null == fileParam.Fs)
            {
                // File wasn't found. So we'll assume that this is the first 
                // time that we are attempting to read ETL files.
                return true;
            }

            try
            {
                // Read the timestamp from the file
                fileParam.Reader = new StreamReader(fileParam.Fs);
                try
                {
                    try
                    {
                        Utility.PerformIOWithRetries(
                            fileParam.ReadLastEndTime);
                    }
                    catch (Exception e)
                    {
                        // Abort this ETL processing pass. We'll try again in the next pass.
                        this.traceSource.WriteExceptionAsError(
                            this.logSourceId,
                            e,
                            "Failed to read last ETL read end time from file {0}.",
                            fileFullPath);
                        return false;
                    }
                }
                finally
                {
                    fileParam.Reader.Dispose();
                }
            }
            finally
            {
                fileParam.Fs.Dispose();
            }

            // Convert the timestamp from binary to DateTime
            lastEndTime = DateTime.FromBinary(fileParam.LastEndTimeLong);
            return true;
        }

        public bool SetLastEndTime(string lastReadFileName, DateTime stopTime)
        {
            var fileFullPath = Path.Combine(this.lastEtlFileReadFileDirectoryPath, lastReadFileName);
            var fileParam = new LastEndTimeFileParameters(this.traceSource, this.logSourceId)
            {
                Fs = null,
                FileName = fileFullPath,
                Mode = FileMode.OpenOrCreate,
                Access = FileAccess.Write,

                // Convert the timestamp from DateTime to binary
                LastEndTimeLong = stopTime.ToBinary(),
                LastEndTime = stopTime
            };

            // Open the file that contains the last ETL read timestamp or create
            // the file if it doesn't already exist.
            try
            {
                Utility.PerformIOWithRetries(
                    fileParam.OpenLastEndTimeFile);
            }
            catch (Exception e)
            {
                this.traceSource.WriteExceptionAsError(
                    this.logSourceId,
                    e,
                    "Failed to open file {0} for write.",
                    fileFullPath);
                return false;
            }

            try
            {
                // Write the time stamp to the file
                fileParam.Writer = new StreamWriter(fileParam.Fs);
                try
                {
                    try
                    {
                        Utility.PerformIOWithRetries(
                            fileParam.WriteLastEndTime);
                    }
                    catch (Exception e)
                    {
                        this.traceSource.WriteExceptionAsError(
                            this.logSourceId,
                            e,
                            "Failed to write last ETL read end time to file {0}.",
                            fileFullPath);
                        return false;
                    }
                }
                finally
                {
                    fileParam.Writer.Dispose();
                }
            }
            finally
            {
                fileParam.Fs.Dispose();
            }

            return true;
        }

        private static string InitializeLastEtlFileReadFilePath(
            bool isReadingFromApplicationManifest,
            string etlPath,
            string logDirectory,
            string traceDirectory,
            string producerInstanceId)
        {
            if (isReadingFromApplicationManifest)
            {
                return Path.Combine(
                    traceDirectory,
                    producerInstanceId);
            }

            // Build the directory name where the file containing the last ETL
            // read information will be stored
            if (!string.IsNullOrEmpty(etlPath))
            {
                return Path.Combine(
                    traceDirectory,
                    EtlProducerConstants.LastEtlReadDirectory);
            }

            return logDirectory;
        }

        private class LastEndTimeFileParameters
        {
            private readonly FabricEvents.ExtensionsEvents traceSource;
            private readonly string logSourceId;

            public LastEndTimeFileParameters(
                FabricEvents.ExtensionsEvents traceSource,
                string logSourceId)
            {
                this.traceSource = traceSource;
                this.logSourceId = logSourceId;
            }

            internal FileAccess Access { get; set; }

            internal string FileName { get; set; }

            internal FileStream Fs { get; set; }

            internal DateTime LastEndTime { get; set; }

            internal long LastEndTimeLong { get; set; }

            internal FileMode Mode { get; set; }

            internal StreamReader Reader { get; set; }

            internal StreamWriter Writer { get; set; }

            public void OpenLastEndTimeFile()
            {
                // Check if file exists as an optimization before trying to open.
                if (this.Mode == FileMode.Open && !FabricFile.Exists(this.FileName))
                {
                    return;
                }

                try
                {
                    this.Fs = FabricFile.Open(this.FileName, this.Mode, this.Access);
#if !DotNetCoreClrLinux
                    Helpers.SetIoPriorityHint(this.Fs.SafeFileHandle, Kernel32Types.PRIORITY_HINT.IoPriorityHintVeryLow);
#endif
                }
                catch (FileNotFoundException)
                {
                    // File was not found. This is not an error.
                }
            }

            public void WriteLastEndTime()
            {
                // Write the version number. This will help us read data from the file.
                this.Writer.WriteLine(EtlProducerConstants.LastEndTimeFormatVersionString);
                this.Writer.WriteLine(
                    "{0},{1}",
                    this.LastEndTimeLong,
                    this.LastEndTime); // We write the human-readable form for easier debugging
            }

            public void ReadLastEndTime()
            {
                var success = true;

                // Check the DCA version to make sure we can parse this file
                var versionLine = this.Reader.ReadLine();
                if (null == versionLine)
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Unable to read DCA version information from file {0}.",
                        this.FileName);
                    success = false;
                }

                if (success)
                {
                    if (false == versionLine.Equals(EtlProducerConstants.LastEndTimeFormatVersionString))
                    {
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "Version information in file {0} is '{1}', which is different from expected version information '{2}'.",
                            this.FileName,
                            versionLine,
                            EtlProducerConstants.LastEndTimeFormatVersionString);
                        success = false;
                    }
                }

                string endTimeLine = null;
                if (success)
                {
                    // Read the last end time
                    endTimeLine = this.Reader.ReadLine();
                    if (null == endTimeLine)
                    {
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "File {0} does not contain the last ETL read end time.",
                            this.FileName);
                        success = false;
                    }
                }

                string[] endTimeLineParts = null;
                if (success)
                {
                    endTimeLineParts = endTimeLine.Split(',');
                    if (endTimeLineParts.Length != 2)
                    {
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "The last ETL read end time '{0}' in file {1} is not in the expected format.",
                            endTimeLine,
                            this.FileName);
                        success = false;
                    }
                }

                if (success)
                {
                    long parsedLastEndTime;
                    success = long.TryParse(
                        endTimeLineParts[0],
                        NumberStyles.Integer,
                        CultureInfo.InvariantCulture,
                        out parsedLastEndTime);
                    this.LastEndTimeLong = parsedLastEndTime;
                    if (false == success)
                    {
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "Unable to parse ETL read end time '{0}' in file {1} as a long data type.",
                            endTimeLine,
                            this.FileName);
                    }
                }

                if (false == success)
                {
                    // We were unable to read the last ETL read end time from the file.
                    // So we'll behave as though the file did not exist, i.e. assume the 
                    // minimum possible value for the last ETL read end time.
                    this.LastEndTimeLong = DateTime.MinValue.ToBinary();
                }
            }
        }
    }
}