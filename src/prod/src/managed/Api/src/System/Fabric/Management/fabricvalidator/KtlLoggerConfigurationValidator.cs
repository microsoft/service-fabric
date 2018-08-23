// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.WindowsFabricValidator
{
    using System;
    using System.Globalization;
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;

    /// <summary>
    /// This is the class handles the validation of configuration for the hosting subsystem
    /// </summary>
    class KtlLoggerConfigurationValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.KtlLogger; }
        }

        public static void ValidateKtlLoggerSharedPathAndId(String sharedLogPath, String sharedLogId, string sectionName)
        {
            if (!string.IsNullOrEmpty(sharedLogId))
            {
                bool formatOk = true;
                try
                {
                    Guid g = Guid.Parse(sharedLogId);
                }
                catch (FormatException)
                {
                    formatOk = false;
                }

                if (!formatOk)
                {
                    throw new ArgumentException(
                        string.Format(
                        "Section '{0}'- SharedLogId should be a well-formed guid such as {F67D29E7-DFAB-437e-9F9F-5AC901D3F0BC} or F67D29E7-DFAB-437e-9F9F-5AC901D3F0BC",
                        sectionName));
                }

                if (sharedLogId == FabricValidatorConstants.ParameterNames.TransactionalReplicator.DefaultSharedLogContainerGuid)
                {
                    throw new ArgumentException(
                        string.Format(
                        "Section '{0}'- SharedLogId should not be the default guid {3CA2CCDA-DD0F-49c8-A741-62AAC0D4EB62}",
                        sectionName));
                }

            }

            if (!string.IsNullOrEmpty(sharedLogPath))
            {
                bool isAbsolutePath;

                isAbsolutePath = System.IO.Path.IsPathRooted(sharedLogPath);

                if (!isAbsolutePath)
                {
                    throw new ArgumentException(
                        string.Format(
                        "Section '{0}'- SharedLogPath " + sharedLogPath + " should be a well-formed absolute pathname",
                        sectionName));
                }
            }

            if ((string.IsNullOrEmpty(sharedLogId)) && (!string.IsNullOrEmpty(sharedLogPath)))
            {
                throw new ArgumentException(
                    string.Format(
                    "Section '{0}'- if shared log id is empty then shared log path should be empty",
                    sectionName));
            }

            if ((!string.IsNullOrEmpty(sharedLogId)) && (string.IsNullOrEmpty(sharedLogPath)))
            {
                throw new ArgumentException(
                    string.Format(
                    "Section '{0}'- if shared log id is not empty then shared log path should be set",
                    sectionName));
            }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);

            string PeriodicFlushTimeString = settingsForThisSection[FabricValidatorConstants.ParameterNames.KtlLogger.PeriodicFlushTime].GetValue<string>();
            int PeriodicFlushTime = int.Parse(PeriodicFlushTimeString);
            int PeriodicFlushTimeMin = 15;
            int PeriodicFlushTimeMax = 5 * 60;
            if ((PeriodicFlushTime < PeriodicFlushTimeMin) ||
                (PeriodicFlushTime > PeriodicFlushTimeMax))
            {
                throw new ArgumentException(
                                string.Format(
                                "Section '{0}'- PeriodicFlushTime (" + PeriodicFlushTime.ToString() + ") must be greater than " + 
                                PeriodicFlushTimeMin.ToString() + " and less than " + PeriodicFlushTimeMax.ToString(), 
                                this.SectionName));
            }

            string PeriodicTimerIntervalString = settingsForThisSection[FabricValidatorConstants.ParameterNames.KtlLogger.PeriodicTimerInterval].GetValue<string>();
            int PeriodicTimerInterval = int.Parse(PeriodicTimerIntervalString);
            int PeriodicTimerIntervalMin = 1;
            int PeriodicTimerIntervalMax = 60;
            if ((PeriodicTimerInterval < PeriodicTimerIntervalMin) ||
                (PeriodicTimerInterval > PeriodicTimerIntervalMax))
            {
                throw new ArgumentException(
                                string.Format(
                                "Section '{0}'- PeriodicTimerInterval (" + PeriodicTimerInterval.ToString() + ") must be greater than " + 
                                PeriodicTimerIntervalMin.ToString() + " and less than " + PeriodicTimerIntervalMax.ToString(),
                                this.SectionName));
            }
            if (PeriodicTimerInterval > PeriodicFlushTime)
            {
                throw new ArgumentException(
                                string.Format(
                                "Section '{0}'- PeriodicTimerInterval (" + PeriodicTimerInterval.ToString() + 
                                        ") must be less than PeriodicFlushTime" + PeriodicFlushTime.ToString(),
                                            this.SectionName));
            }

            string AllocationTimeoutString = settingsForThisSection[FabricValidatorConstants.ParameterNames.KtlLogger.AllocationTimeout].GetValue<string>();
            int AllocationTimeout = int.Parse(AllocationTimeoutString);
            int AllocationTimeoutMin = 0;
            int AllocationTimeoutMax = 60*60;
            if ((AllocationTimeout < AllocationTimeoutMin) ||
                (AllocationTimeout > AllocationTimeoutMax))
            {
                throw new ArgumentException(
                                string.Format(
                                "Section '{0}'- AllocationTimeout (" + AllocationTimeout.ToString() + ") must be greater than " + 
                                AllocationTimeoutMin.ToString() + " and less than " + AllocationTimeoutMax.ToString(),
                                this.SectionName));
            }

            int PinnedMemoryLimitInKB = settingsForThisSection[FabricValidatorConstants.ParameterNames.KtlLogger.PinnedMemoryLimitInKB].GetValue<int>();
            if (PinnedMemoryLimitInKB != 0)
            {
                throw new ArgumentException(
                    string.Format(
                    "Section '{0}'- PinnedMemoryLimitInKB (" + PinnedMemoryLimitInKB.ToString() + ") must be set to no limit (0)",
                    SectionName));
            }

            int WriteBufferMemoryPoolMinimumInKB = settingsForThisSection[FabricValidatorConstants.ParameterNames.KtlLogger.WriteBufferMemoryPoolMinimumInKB].GetValue<int>();
            int WriteBufferMemoryPoolMinimumInKBMin = 16 * 1024;
            if ((WriteBufferMemoryPoolMinimumInKB != 0) && (WriteBufferMemoryPoolMinimumInKB < WriteBufferMemoryPoolMinimumInKBMin))
            {
                throw new ArgumentException(
                    string.Format(
                    "Section '{0}'- WriteBufferMemoryPoolMinimumInKB (" + WriteBufferMemoryPoolMinimumInKB +") must be greater than " + WriteBufferMemoryPoolMinimumInKBMin,
                    SectionName));
            }
            
            int WriteBufferMemoryPoolMaximumInKB = settingsForThisSection[FabricValidatorConstants.ParameterNames.KtlLogger.WriteBufferMemoryPoolMaximumInKB].GetValue<int>();
            
            int WriteBufferMemoryPoolPerStreamInKB = settingsForThisSection[FabricValidatorConstants.ParameterNames.KtlLogger.WriteBufferMemoryPoolPerStreamInKB].GetValue<int>();
            int WriteBufferMemoryPoolPerStreamInKBMin = 1024;
            if ((WriteBufferMemoryPoolPerStreamInKB != -1) && (WriteBufferMemoryPoolPerStreamInKB != 0))
            {
                if (WriteBufferMemoryPoolPerStreamInKB < WriteBufferMemoryPoolPerStreamInKBMin)
                {
                    throw new ArgumentException(
                                    string.Format(
                                    "Section '{0}'- WriteBufferMemoryPoolPerStreamInKB (" + WriteBufferMemoryPoolPerStreamInKB.ToString() + ") must be greater than " +
                                    WriteBufferMemoryPoolPerStreamInKBMin.ToString(),
                                    this.SectionName));
                }
            }

            if (WriteBufferMemoryPoolMaximumInKB != 0)
            {
                if (WriteBufferMemoryPoolMinimumInKB == 0)
                {
                    throw new ArgumentException(
                        string.Format(
                        "Section '{0}'- WriteBufferMemoryPoolMinimumInKB must not have no limit if WriteBufferMemoryPoolMaximumInKB does have a limit",
                        SectionName));
                }

                if (WriteBufferMemoryPoolMinimumInKB > WriteBufferMemoryPoolMaximumInKB)
                {
                    throw new ArgumentException(
                            string.Format(
                        "Section '{0}'- WriteBufferMemoryPoolMinimumInKB (" + WriteBufferMemoryPoolMinimumInKB +
                                      ") must be less than or equal to WriteBufferMemoryPoolMaximumInKB (" + WriteBufferMemoryPoolMaximumInKB + ")",
                            SectionName));
                }
            }

            uint sharedLogCreateFlags = settingsForThisSection[FabricValidatorConstants.ParameterNames.KtlLogger.SharedLogCreateFlags].GetValue<uint>();

            int sharedLogSizeInMB = settingsForThisSection[FabricValidatorConstants.ParameterNames.KtlLogger.SharedLogSizeInMB].GetValue<int>();
            int sharedLogSizeInMBMin = 512;
            
            string sharedLogId = settingsForThisSection[FabricValidatorConstants.ParameterNames.KtlLogger.SharedLogId].GetValue<string>();
            string sharedLogPath = settingsForThisSection[FabricValidatorConstants.ParameterNames.KtlLogger.SharedLogPath].GetValue<string>();

            int sharedLogNumberStreams = settingsForThisSection[FabricValidatorConstants.ParameterNames.KtlLogger.SharedLogNumberStreams].GetValue<int>();
            int sharedLogNumberStreamsMin = 3 * 512;
            int sharedLogNumberStreamsMax = 3 * 8192;

            int sharedLogMaximumRecordSizeInKB = settingsForThisSection[FabricValidatorConstants.ParameterNames.KtlLogger.SharedLogMaximumRecordSizeInKB].GetValue<int>();

            if (sharedLogCreateFlags != 0)
            {
                throw new ArgumentException(
                    string.Format(
                    "Section '{0}'- SharedLogCreateFlags (" + sharedLogCreateFlags +
                                  ") must be zero",
                    SectionName));
            }
            if (sharedLogMaximumRecordSizeInKB != 0)
            {
                throw new ArgumentException(
                    string.Format(
                    "Section '{0}'- SharedLogMaximumRecordSizeInKB (" + sharedLogMaximumRecordSizeInKB +
                                  ") must be zero",
                    SectionName));
            }

            if ((sharedLogNumberStreams < sharedLogNumberStreamsMin) ||
                (sharedLogNumberStreams > sharedLogNumberStreamsMax))
            {
                throw new ArgumentException(
                    string.Format(
                    "Section '{0}'- SharedLogNumberOfStreams (" + sharedLogNumberStreams +
                                  ") must be greater than or equal to " + sharedLogNumberStreamsMin +
                                  " and less than " + sharedLogNumberStreamsMax,
                    SectionName));
            }

            if (sharedLogSizeInMB < sharedLogSizeInMBMin)
            {
                throw new ArgumentException(
                    string.Format(
                    "Section '{0}'- SharedLogSizeInMB (" + sharedLogSizeInMB +
                                  ") must be greater than or equal to " + sharedLogSizeInMBMin,
                    SectionName));

            }

            ValidateKtlLoggerSharedPathAndId(sharedLogPath, sharedLogId, SectionName);
        }
        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }
}