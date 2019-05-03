// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.TraceAccessLayer.StoreReaders
{
    using System;
    using System.IO;
    using System.Linq;
    using ClusterAnalysis.Common;
    using ClusterAnalysis.Common.Log;
    using ClusterAnalysis.Common.Util;
    using ClusterAnalysis.TraceAccessLayer.StoreConnection;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Reader;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Reader;

    public abstract class LocalStoreReader : TraceStoreReader
    {
        /// <summary>
        /// LogRoot folder
        /// </summary>
        private const string LogFolder = "log";

        /// <summary>
        /// Windows traces for now.. TODO Handle Linux
        /// </summary>
        private const string TraceFilesPattern = "*.etl";

        /// <summary>
        /// Local connection information.
        /// </summary>
        private LocalTraceStoreConnectionInformation localConnectionInfo;

        protected LocalStoreReader(ILogProvider logProvider, LocalTraceStoreConnectionInformation localConnectionInfo) : base(logProvider)
        {
            Assert.IsNotNull(localConnectionInfo, "localConnectionInfo can't be null");
            this.localConnectionInfo = localConnectionInfo;
        }

        /// <inheritdoc />
        public override bool IsPropertyLevelFilteringSupported()
        {
            return false;
        }

        /// <summary>
        /// Get the Relative path where Traces are Present.
        /// </summary>
        /// <returns></returns>
        protected abstract string GetRelativePathUnderLogRoot();

        /// <summary>
        /// From trace folder, gets the paths for trace files whose creation dates fall in the interval
        /// </summary>
        /// <param name="duration">Time duration</param>
        /// <returns>Trace files full paths</returns>
        /// <remarks>
        /// This is kept virtual so that Kids can override this if required.
        /// </remarks>
        protected virtual string[] GetInRangeTraceFiles(Duration duration)
        {
            var dir = new DirectoryInfo(Path.Combine(this.GetLogRoot(), this.GetRelativePathUnderLogRoot()));
            if (!dir.Exists)
            {
                throw new DirectoryNotFoundException(dir.FullName);
            }

            var sortedFiles = dir.GetFiles(TraceFilesPattern, SearchOption.TopDirectoryOnly).OrderBy(f => f.CreationTime).ToArray();

            if (sortedFiles.Length == 0)
            {
                return new string[0];
            }

            int startIndex = ReverseLinearSearch(sortedFiles, duration.StartTime);
            int endIndex = ReverseLinearSearch(sortedFiles, duration.EndTime);

            if (endIndex < 0)
            {
                return new string[0];
            }

            startIndex = Math.Max(0, startIndex);

            var inRangeFiles = new string[endIndex - startIndex + 1];
            for (int i = startIndex, j = 0; i <= endIndex; i++, j++)
            {
                inRangeFiles[j] = sortedFiles[i].FullName;
            }

            return inRangeFiles;
        }

        /// <summary>
        /// Get the Trace Record Session for a particular duration
        /// </summary>
        /// <param name="duration"></param>
        /// <returns></returns>
        protected override TraceRecordSession GetTraceRecordSession(Duration duration)
        {
            var filesInRange = this.GetInRangeTraceFiles(duration);
            return new EtwTraceRecordSession(filesInRange);
        }

        /// <summary>
        /// From the last element towards the first, searches for the file that might contain events around the target time
        /// </summary>
        /// <param name="files">Array of files sorted by creation time</param>
        /// <param name="targetTime">The time of interest</param>
        /// <returns>Index of the file in the array or -1 if targetTime is before the creation of the first file</returns>
        private static int ReverseLinearSearch(FileInfo[] files, DateTime targetTime)
        {
            int i = files.Length - 1;
            for (; i >= 0; i--)
            {
                int comparedValue = targetTime.CompareTo(files[i].CreationTime.ToUniversalTime());
                if (comparedValue >= 0)
                {
                    break;
                }
            }

            return i;
        }

        /// <summary>
        /// Gets the path of the log root directory from FabricEnvironment
        /// </summary>
        /// <returns>Path of the log root directory</returns>
        private string GetLogRoot()
        {
            string logroot = this.localConnectionInfo.FabricLogRoot;
            if (string.IsNullOrEmpty(logroot))
            {
                logroot = Path.Combine(this.localConnectionInfo.FabricDataRoot + LogFolder);
            }

            if (string.IsNullOrEmpty(logroot))
            {
                throw new Exception("Log Root Empty/Null");
            }

            return logroot;
        }
    }
}