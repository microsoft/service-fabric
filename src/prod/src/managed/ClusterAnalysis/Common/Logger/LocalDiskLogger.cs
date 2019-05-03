// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.Common.Log
{
    using System;
    using System.Globalization;
    using System.IO;
    using System.Text;
    using ClusterAnalysis.Common.Util;

    /// <summary>
    /// Simple Disk based implementation of logger
    /// </summary>
    /// <remarks>
    /// NOT THREAD SAFE.
    /// </remarks>
    public class LocalDiskLogger : ILogger, ILogProvider
    {
        // Can be increased once service stabilizes. Right now, I need logs to be persisted frequently
        private const uint MaxLengthBeforePersist = 128;

        private static Random rand = new Random();

        private static ILogProvider logProviderSingleInstance;

        private static string logFolder;

        private string fullPathToLogFileOnDisk;

        private StringBuilder logBuilder;

        private object flushLock;

        private LocalDiskLogger()
        {
            logFolder = string.Format("LogFolder-{0}", DateTime.UtcNow.Ticks);
            if (!Directory.Exists(logFolder))
            {
                Directory.CreateDirectory(logFolder);
            }
        }

        /// <summary>
        /// Create a reference of <see cref="ILogger"/>
        /// </summary>
        /// <remarks>
        /// Private constructor to ensure that creation can be controlled.
        /// </remarks>
        private LocalDiskLogger(string fullPathToLogFile)
        {
            Assert.IsNotEmptyOrNull(fullPathToLogFile, "Log file Name can't be null.");
            this.flushLock = new object();
            this.logBuilder = new StringBuilder();
            var logFileNameWithoutExtension = Path.GetFileNameWithoutExtension(fullPathToLogFile);
            var logFileExtension = Path.GetExtension(fullPathToLogFile);
            int counter = 1;
            while (true)
            {
                var fullPath = Path.Combine(Environment.CurrentDirectory, logFolder, fullPathToLogFile);
                if (File.Exists(fullPath))
                {
                    this.logBuilder.AppendLine(string.Format(CultureInfo.InvariantCulture, "File '{0}' Already Exists", fullPathToLogFile));
                    fullPathToLogFile = string.Format(CultureInfo.InvariantCulture, "{0}-{1}{2}", logFileNameWithoutExtension, counter++, logFileExtension);
                    continue;
                }

                break;
            }

            this.fullPathToLogFileOnDisk = Path.Combine(Environment.CurrentDirectory, logFolder, fullPathToLogFile);
        }

        /// <summary>
        /// Get the provider Instance
        /// </summary>
        public static ILogProvider LogProvider
        {
            get { return logProviderSingleInstance ?? (logProviderSingleInstance = new LocalDiskLogger()); }
        }

        public ILogger CreateLoggerInstance(string logIdentifier)
        {
            return new LocalDiskLogger(logIdentifier);
        }

        /// <summary>
        /// Log a message
        /// </summary>
        /// <param name="format"></param>
        /// <param name="message"></param>
        public void LogMessage(string format, params object[] message)
        {
            Assert.IsNotNull(format, "format");

            lock (this.flushLock)
            {
                this.logBuilder.AppendLine();
                try
                {
                    this.logBuilder.Append(
                        string.Format(CultureInfo.InvariantCulture, "TimeLocal: '{0}', Msg: '{1}'", DateTime.Now, string.Format(format, message)));
                    {
                        // No need to hog memory, persist to disk.
                        // if (this.logBuilder.Length > MaxLengthBeforePersist)
                        this.Flush();
                    }
                }
                catch (FormatException exp)
                {
                    this.logBuilder.Append("Encountered Exception " + exp.ToString());
                }
            }
        }

        /// <inheritdoc />
        public void LogWarning(string format, params object[] message)
        {
            this.LogMessage(format, message);
        }

        /// <inheritdoc />
        public void LogError(string format, params object[] message)
        {
            this.LogMessage(format, message);
        }

        /// <summary>
        /// Flush to Disk
        /// </summary>
        public void Flush()
        {
            lock (this.flushLock)
            {
                this.PersistToDisk();
                this.logBuilder.Clear();
            }
        }

        private void PersistToDisk()
        {
            if (this.logBuilder.Length == 0)
            {
                // nothing to persist here
                return;
            }

            this.logBuilder.AppendLine();

            try
            {
                File.AppendAllText(this.fullPathToLogFileOnDisk, this.logBuilder.ToString());
                this.logBuilder.Clear();
            }
            catch (IOException)
            {
            }
            catch (UnauthorizedAccessException)
            {
            }
        }
    }
}