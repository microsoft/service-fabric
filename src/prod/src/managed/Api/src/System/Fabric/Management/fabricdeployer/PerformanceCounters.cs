// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.Common;
    using System.Fabric.Common.ImageModel;
    using System.Fabric.Dca;
    using System.Fabric.Management.ServiceModel;
    using System.Globalization;
    using System.Threading;
    using System.IO;
    using System.Linq;

    using Microsoft.Win32;

    /// <summary>
    /// This class implements performance counter collection on the node
    /// </summary>
    internal static class PerformanceCounters
    {
        #region Private Fields
        private const string CounterFileNamePrefix = "fabric_counters";
        private const int SchtasksSuccessExitCode = 0;
        private const int LogmanSuccessExitCode = 0;
        private const int LogmanDataCollectorAlreadyCreatedExitCode = -2144337655;
        private const int LogmanDataCollectorAlreadyStartedExitCode = -2147216609;
        private const int LogmanDataCollectorNotRunningExitCode = -2144337660;
        private const int LogmanDataCollectorNotFoundExitCode = -2144337918;
        private const string DataCollectorName = "FabricCounters";
        private const int PerformanceCategoryActionRetryCount = 1;
        private static readonly TimeSpan ReloadPerformanceCounterIndexTimeout = TimeSpan.FromSeconds(5);
        private static readonly TimeSpan DefaultLogmanWaitTimeout = TimeSpan.FromSeconds(10);
        #endregion

        #region Internal Functions
        internal static bool StartCollection(SettingsOverridesTypeSection[] fabricSettings, FabricDeploymentSpecification deploymentSpecification)
        {
            if (!StopDataCollector())
            {
                return false;
            }

            if (!DeleteDataCollector())
            {
                return false;
            }

            var samplingInterval = PerformanceCounterCommon.DefaultSamplingInterval;
            HashSet<string> counters = null;
            int maxFileSizeInMB = PerformanceCounterCommon.MaxPerformanceCountersBinaryFileSizeInMB;
            var newFileCreationInterval = PerformanceCounterCommon.NewPerfCounterBinaryFileCreationInterval;
            string outputFolderPath = deploymentSpecification.GetPerformanceCountersBinaryFolder();
            string outputFileNamePrefix = CounterFileNamePrefix;
            bool includeMachineNameInOuputFile = false;
            bool enableCircularTraceSession = FabricEnvironment.GetEnableCircularTraceSession();

            SettingsOverridesTypeSection manifestSection = fabricSettings.FirstOrDefault(section => section.Name.Equals(Constants.SectionNames.PerformanceCounterLocalStore, StringComparison.OrdinalIgnoreCase));
            if (manifestSection != null)
            {
                var isEnabledParameter = manifestSection.Parameter.FirstOrDefault(param => param.Name.Equals(System.Fabric.FabricValidatorConstants.ParameterNames.IsEnabled, StringComparison.OrdinalIgnoreCase));
                if (isEnabledParameter != null)
                {
                    bool isEnabled = bool.Parse(isEnabledParameter.Value);
                    if (!isEnabled)
                    {
                        DeployerTrace.WriteInfo(
                            "Performance counter collection is not enabled because parameter {0} in section {1} is set to false.",
                            System.Fabric.FabricValidatorConstants.ParameterNames.IsEnabled,
                            Constants.SectionNames.PerformanceCounterLocalStore);
                        return true;
                    }
                }

                var samplingIntervalSecondsParameter = manifestSection.Parameter.FirstOrDefault(param => param.Name.Equals(Constants.ParameterNames.SamplingIntervalInSeconds, StringComparison.OrdinalIgnoreCase));
                if (samplingIntervalSecondsParameter != null)
                {
                    samplingInterval = TimeSpan.FromSeconds(int.Parse(samplingIntervalSecondsParameter.Value, CultureInfo.InvariantCulture));
                }

                var countersParameter = manifestSection.Parameter.FirstOrDefault(param => param.Name.Equals(System.Fabric.FabricValidatorConstants.ParameterNames.Counters, StringComparison.OrdinalIgnoreCase));
                if (countersParameter != null)
                {
                    PerformanceCounterCommon.ParseCounterList(countersParameter.Value, out counters);
                }

                var maxFileSizeInMBParameter = manifestSection.Parameter.FirstOrDefault(param => param.Name.Equals(Constants.ParameterNames.MaxCounterBinaryFileSizeInMB, StringComparison.OrdinalIgnoreCase));
                if (maxFileSizeInMBParameter != null)
                {
                    maxFileSizeInMB = int.Parse(maxFileSizeInMBParameter.Value, CultureInfo.InvariantCulture);
                }

                var newFileCreationIntervalMinutesParameter = manifestSection.Parameter.FirstOrDefault(param => param.Name.Equals(System.Fabric.FabricValidatorConstants.ParameterNames.NewCounterBinaryFileCreationIntervalMinutes, StringComparison.OrdinalIgnoreCase));
                if (newFileCreationIntervalMinutesParameter != null)
                {
                    newFileCreationInterval = TimeSpan.FromMinutes(int.Parse(newFileCreationIntervalMinutesParameter.Value, CultureInfo.InvariantCulture));
                }

                var testOnlyCounterFilePath = manifestSection.Parameter.FirstOrDefault(param => param.Name.Equals(System.Fabric.FabricValidatorConstants.ParameterNames.TestOnlyCounterFilePath, StringComparison.OrdinalIgnoreCase));
                if (testOnlyCounterFilePath != null)
                {
                    outputFolderPath = testOnlyCounterFilePath.Value;
                }

                var testOnlyCounterFileNamePrefix = manifestSection.Parameter.FirstOrDefault(param => param.Name.Equals(System.Fabric.FabricValidatorConstants.ParameterNames.TestOnlyCounterFileNamePrefix, StringComparison.OrdinalIgnoreCase));
                if (testOnlyCounterFileNamePrefix != null)
                {
                    outputFileNamePrefix = testOnlyCounterFileNamePrefix.Value;
                }

                var testOnlyIncludeMachineNameInCounterFileName = manifestSection.Parameter.FirstOrDefault(param => param.Name.Equals(Constants.ParameterNames.TestOnlyIncludeMachineNameInCounterFileName, StringComparison.OrdinalIgnoreCase));
                if (testOnlyIncludeMachineNameInCounterFileName != null)
                {
                    includeMachineNameInOuputFile = bool.Parse(testOnlyIncludeMachineNameInCounterFileName.Value);
                }
            }

            if (counters == null)
            {
                PerformanceCounterCommon.ParseCounterList(PerformanceCounterCommon.LabelDefault, out counters);
            }

            DeployerTrace.WriteInfo("Performance counter sampling interval: {0} seconds.", samplingInterval);

            if (!StartCollectionForCounterSet(
                counters, 
                outputFolderPath, 
                outputFileNamePrefix, 
                includeMachineNameInOuputFile, 
                (int)samplingInterval.TotalSeconds, 
                maxFileSizeInMB, 
                (int)newFileCreationInterval.TotalMinutes,
                enableCircularTraceSession))
            {
                return false;
            }

            return true;
        }

        internal static bool StopDataCollector()
        {
            string arguments = String.Format(
                                   CultureInfo.InvariantCulture,
                                   "stop {0}",
                                   DataCollectorName);
            var retryCount = 3;
            for (;;)
            {
                int errorCode = Utility.ExecuteCommand("logman", arguments, DefaultLogmanWaitTimeout);
                if ((errorCode == LogmanSuccessExitCode) ||
                    (errorCode == LogmanDataCollectorNotFoundExitCode) ||
                    (errorCode == LogmanDataCollectorNotRunningExitCode))
                {
                    return true;
                }

                if (retryCount-- == 0)
                {
                    DeployerTrace.WriteError(
                        "Unable to stop data collector for performance counters. The command \"logman {0}\" failed with error code {1}.",
                        arguments,
                        errorCode);
                    return false;
                }

                DeployerTrace.WriteInfo("Removing scheduled task and then retrying.");
                DeletePlaScheduledTask(DataCollectorName);
                Thread.Sleep(TimeSpan.FromSeconds(5));
            }
        }

        internal static bool DeleteDataCollector()
        {
            string arguments = String.Format(
                                   CultureInfo.InvariantCulture,
                                   "delete {0}",
                                   DataCollectorName);
            int errorCode = Utility.ExecuteCommand("logman", arguments);
            if ((errorCode != LogmanSuccessExitCode) &&
                (errorCode != LogmanDataCollectorNotFoundExitCode))
            {
                DeployerTrace.WriteError("Unable to delete data collector for performance counters. The command \"logman {0}\" failed with error code {1}.", arguments, errorCode);
                return false;
            }
            return true;
        }
        #endregion

        #region Private Functions
        private static bool StartCollectionForCounterSet(
                                HashSet<string> counterList, 
                                string outputFilePath, 
                                string outputFileNamePrefix, 
                                bool includeMachineNameInOuputFile,
                                int samplingIntervalSeconds,
                                int maxFileSizeMB,
                                int newFileCreationIntervalMinutes,
                                bool enableCircularTraceSession)
        {
            string counterFilePath;
            try
            {
                CreateCounterFile(counterList, out counterFilePath);
            }
            catch (IOException e)
            {
                DeployerTrace.WriteError("Exception occurred while creating temporary file containing performance counters to collect. Exception info: {0}", e);
                return false;
            }

            try
            {
                if (!CreateDataCollector(
                    counterFilePath, 
                    outputFilePath, 
                    outputFileNamePrefix, 
                    includeMachineNameInOuputFile, 
                    samplingIntervalSeconds, 
                    maxFileSizeMB, 
                    newFileCreationIntervalMinutes,
                    enableCircularTraceSession))
                {
                    return false;
                }

                if (!StartDataCollector())
                {
                    return false;
                }
            }
            finally
            {
                try
                {
                    File.Delete(counterFilePath);
                }
                catch (IOException e)
                {
                    DeployerTrace.WriteError("Exception occurred while deleting temporary file containing performance counters to collect. Exception info: {0}", e);
                }
            }

            return true;
        }

        private static bool CreateDataCollector(
                                string counterFilePath, 
                                string outputFilePath, 
                                string outputFileNamePrefix, 
                                bool includeMachineNameInOuputFile, 
                                int samplingIntervalSeconds,
                                int maxFileSizeMB,
                                int newFileCreationIntervalMinutes,
                                bool enableCircularTraceSession)
        {
            string fileNamePattern;

            if (includeMachineNameInOuputFile)
            {
                fileNamePattern = String.Format(
                                      CultureInfo.InvariantCulture,
                                      "{0}_{1}_{2}",
                                      outputFileNamePrefix,
                                      Environment.MachineName,
                                      DateTime.Now.Ticks);
            }
            else
            {
                fileNamePattern = String.Format(
                                      CultureInfo.InvariantCulture,
                                      "{0}_{1}",
                                      outputFileNamePrefix,
                                      DateTime.Now.Ticks);
            }
            string outputFilePattern = Path.Combine(
                                           outputFilePath,
                                           fileNamePattern);
            int newFileCreationIntervalSeconds = newFileCreationIntervalMinutes * 60;
            string arguments = String.Format(
                                   CultureInfo.InvariantCulture,
                                   "create counter {0} -cf {1} -f {2} -si {3} -o \"{4}\" -v nnnnnn -max {5} -cnf {6}",
                                   DataCollectorName,
                                   counterFilePath,
                                   enableCircularTraceSession ? "bincirc" : "bin",
                                   samplingIntervalSeconds,
                                   outputFilePattern,
                                   maxFileSizeMB,
                                   newFileCreationIntervalSeconds);
            int errorCode = Utility.ExecuteCommand("logman", arguments);
            if ((errorCode != LogmanSuccessExitCode) &&
                (errorCode != LogmanDataCollectorAlreadyCreatedExitCode))
            {
                DeployerTrace.WriteError("Unable to create data collector for performance counters. The command \"logman {0}\" failed with error code {1}.", arguments, errorCode);
                return false;
            }
            return true;
        }

        private static bool StartDataCollector()
        {
            string arguments = String.Format(
                                   CultureInfo.InvariantCulture,
                                   "start {0}",
                                   DataCollectorName);

            var retryCount = 3;
            for(;;)
            {
                int errorCode = Utility.ExecuteCommand("logman", arguments, DefaultLogmanWaitTimeout);
                if (errorCode == LogmanSuccessExitCode ||
                  errorCode == LogmanDataCollectorAlreadyStartedExitCode)
                {
                    return true;
                }

                if (retryCount-- == 0)
                {
                    DeployerTrace.WriteError("Unable to start data collector for performance counters. The command \"logman {0}\" failed with error code {1}.", arguments, errorCode);
                    return false;
                }

                Thread.Sleep(TimeSpan.FromSeconds(5));
            }
        }

        private static void DeletePlaScheduledTask(string taskName)
        {
            var args = string.Format(@"/delete /tn \Microsoft\Windows\PLA\{0} /f", taskName);
            var errorCode = Utility.ExecuteCommand("schtasks", args);
            if (errorCode != SchtasksSuccessExitCode)
            {
                DeployerTrace.WriteWarning(
                    "Unable to remove scheduled task for performance counters. The command \"schtasks {0}\" failed with error code {1}.",
                    args,
                    errorCode);
            }
        }

        private static void CreateCounterFile(HashSet<string> counterList, out string counterFilePath)
        {
            counterFilePath = Path.GetTempFileName();
            using (FileStream fileStream = new FileStream(counterFilePath, FileMode.Create, FileAccess.Write, FileShare.Read))
            {
                using (StreamWriter writer = new StreamWriter(fileStream))
                {
                    foreach (string counter in counterList)
                    {
                        writer.WriteLine(counter);
                    }
                }
            }
        }

        #endregion
    }
}