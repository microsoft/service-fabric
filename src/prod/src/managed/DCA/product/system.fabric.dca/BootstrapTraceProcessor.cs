// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.Fabric.Dca.EtlConsumerHelper;
    using System.Globalization;
    using System.IO;
    using System.Linq;

    internal class BootstrapTraceProcessor : IDisposable
    {
        private const string FabricSetupTraceFilePrefix = "FabricSetup-";
        private const string FabricDeployerTraceFilePrefix = "FabricDeployer-";
        private const string FabricUpgradeScriptTraceFilePrefix = "Fabric_UpgradeScript_";
        private const string FabricMsiLogFilePrefix = "MsiInstaller_";
        private const string BootstrapTraceFileSuffix = ".trace";
        private const string BootstrapLogFileSuffix = ".log";

        private static readonly System.Collections.ObjectModel.ReadOnlyDictionary<string, string> TraceFilePatterns = new System.Collections.ObjectModel.ReadOnlyDictionary<string, string>(
            new Dictionary<string, string>
            {
                { FabricSetupTraceFilePrefix, BootstrapTraceFileSuffix },
                { FabricDeployerTraceFilePrefix, BootstrapTraceFileSuffix },
                { FabricUpgradeScriptTraceFilePrefix, BootstrapTraceFileSuffix },
                { FabricMsiLogFilePrefix, BootstrapLogFileSuffix }
            });

        private readonly string traceDirectory;
        private readonly string markerFileDirectory;
        private readonly ReadOnlyCollection<ITraceOutputWriter> traceOutputWriters;
        private readonly FabricEvents.ExtensionsEvents traceSource;
        private readonly string logSourceId;

        private readonly DcaTimer timer;

        private bool disposed;

        public BootstrapTraceProcessor(
            string traceDirectory,
            string markerFileDirectory,
            IEnumerable<ITraceOutputWriter> traceOutputWriters,
            TimeSpan scanInterval,
            FabricEvents.ExtensionsEvents traceSource,
            string logSourceId)
        {
            this.traceDirectory = traceDirectory;
            this.markerFileDirectory = markerFileDirectory;
            this.traceOutputWriters = traceOutputWriters.ToList().AsReadOnly();
            this.traceSource = traceSource;
            this.logSourceId = logSourceId;
            this.timer = new DcaTimer(
                string.Format("{0}_bootstrap", logSourceId),
                this.DoScan,
                scanInterval);
        }

        public void Start()
        {
            if (this.disposed)
            {
                throw new ObjectDisposedException("BootstrapTraceProcessor");
            }

            this.timer.StartOnce(TimeSpan.Zero);
        }

        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }

            this.disposed = true;

            this.timer.StopAndDispose();
            this.timer.DisposedEvent.WaitOne();
        }

        private void DoScan(object ignored)
        {
            this.OnBootstrapTraceScanStart();
            foreach (var filePrefix in TraceFilePatterns.Keys)
            {
                var traceFileNames = this.GetBootstrapTraceFileNames(
                    filePrefix,
                    TraceFilePatterns[filePrefix]);
                this.ProcessBootstrapTraceFiles(traceFileNames);
            }

            this.OnBootstrapTraceScanStop();
            this.timer.Start();
        }

        private string[] GetBootstrapTraceFileNames(string fileNamePrefix, string fileSuffix)
        {
            // Build the pattern of the trace file name using wildcards
            var fileNameWithWildcard = string.Format(
                CultureInfo.InvariantCulture,
                "{0}*{1}",
                fileNamePrefix,
                fileSuffix);

            // Get all file names that match the pattern
            var bootstrapTraceFileNames = FabricDirectory.GetFiles(
                this.traceDirectory,
                fileNameWithWildcard);
            return bootstrapTraceFileNames;
        }

        private void ProcessBootstrapTraceFiles(string[] traceFileNames)
        {
            foreach (var bootstrapTraceFileName in traceFileNames)
            {
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Processing bootstrap trace file: {0}.",
                    bootstrapTraceFileName);

                if (!this.OnBootstrapTraceAvailable(bootstrapTraceFileName))
                {
                    // Some writer nack'ed the file. Try again later.
                    continue;
                }

                // Now that we are done with this file, move it to the archived
                // traces directory. If we fail to move that probably means the
                // file is still in use.  We will retry on next pass.
                var destFileName = Path.Combine(
                    this.markerFileDirectory,
                    Path.GetFileName(bootstrapTraceFileName));

                var moveFailed = false;
                try
                {
                    FabricFile.Move(bootstrapTraceFileName, destFileName);
                }
                catch (Exception e)
                {
                    if (false == ((e is IOException) || (e is FabricException)))
                    {
                        throw;
                    }

                    // The Azure wrapper keeps an open handle to the bootstrap 
                    // trace file for an extended period of time. In this case,
                    // an attempt to move the file throws an IO exception. This
                    // is not a fatal error, so catch the exception and continue.
                    moveFailed = true;
                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "An exception has been handled and execution will continue. Exception information: {0}",
                        e);
                }

                if (moveFailed)
                {
                    this.traceSource.WriteNoise(
                        this.logSourceId,
                        "Bootstrap trace file {0} was processed. It could not be moved to the archived traces directory, possibly because the Azure wrapper has an open handle to it.",
                        bootstrapTraceFileName,
                        destFileName);
                }
                else
                {
                    this.traceSource.WriteNoise(
                        this.logSourceId,
                        "Bootstrap trace file was processed and moved. Source: {0}, Destination: {1}.",
                        bootstrapTraceFileName,
                        destFileName);
                }
            }
        }

        private void OnBootstrapTraceScanStart()
        {
            foreach (var traceOutputWriter in this.traceOutputWriters)
            {
                traceOutputWriter.OnBootstrapTraceFileScanStart();
            }
        }

        private void OnBootstrapTraceScanStop()
        {
            foreach (var traceOutputWriter in this.traceOutputWriters)
            {
                traceOutputWriter.OnBootstrapTraceFileScanStop();
            }
        }

        private bool OnBootstrapTraceAvailable(string bootstrapTraceFileName)
        {
            var success = true;
            foreach (var traceOutputWriter in this.traceOutputWriters)
            {
                if (!traceOutputWriter.OnBootstrapTraceFileAvailable(bootstrapTraceFileName))
                {
                    success = false;
                }
            }

            return success;
        }
    }
}