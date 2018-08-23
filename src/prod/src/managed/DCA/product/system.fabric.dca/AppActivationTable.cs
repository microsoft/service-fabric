// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.IO;

    // Class that maintains the activation time for each application instance
    internal static class AppActivationTable
    {
        private const string LogSourceId = "AppActivationTable";
        private const string VersionString = "Version: 1";
        private const string BackupFileName = "AppActivationTable.dat";
        private static Dictionary<string, Dictionary<string, DateTime>> applicationEtwTracesStartTime;
        private static string backupFileFullPath;

        internal enum ActivationRecordParts
        {
            AppInstanceId,
            AppType,
            ActivationTime,
            ActivationTimeAsString,

            // This value is not a part of the activation record. It
            // represents the total count of parts.
            Count
        }

        internal static bool Initialize(string backupFilePath)
        {
            backupFileFullPath = Path.Combine(backupFilePath, BackupFileName);
            applicationEtwTracesStartTime = new Dictionary<string, Dictionary<string, DateTime>>();
            bool result;
            lock (applicationEtwTracesStartTime)
            {
                result = LoadFromFile();
            }

            return result;
        }

        internal static bool AddRecord(string applicationInstanceId, string applicationType, DateTime activationTime)
        {
            lock (applicationEtwTracesStartTime)
            {
                if (false == applicationEtwTracesStartTime.ContainsKey(applicationType))
                {
                    applicationEtwTracesStartTime[applicationType] = new Dictionary<string, DateTime>();
                }

                applicationEtwTracesStartTime[applicationType][applicationInstanceId] = activationTime;
            }

            return SaveToFile();
        }

        internal static bool RemoveRecord(string applicationInstanceId)
        {
            lock (applicationEtwTracesStartTime)
            {
                foreach (string applicationType in applicationEtwTracesStartTime.Keys)
                {
                    if (applicationEtwTracesStartTime[applicationType].Remove(applicationInstanceId))
                    {
                        break;
                    }
                }
            }

            return SaveToFile();
        }

        internal static DateTime GetEtwTracesStartTimeForApplicationType(string applicationType)
        {
            DateTime startTime = DateTime.MaxValue;
            lock (applicationEtwTracesStartTime)
            {
                if (applicationEtwTracesStartTime.ContainsKey(applicationType))
                {
                    foreach (DateTime time in applicationEtwTracesStartTime[applicationType].Values)
                    {
                        if (startTime.CompareTo(time) > 0)
                        {
                            startTime = time;
                        }
                    }
                }
            }

            return startTime;
        }

        private static bool LoadFromFile()
        {
            // Open the backup file
            StreamReader reader = null;
            bool backupFileNotPresent = false;
            try
            {
                Utility.PerformIOWithRetries(
                    () =>
                    {
                        try
                        {
                            FileStream fileStream = FabricFile.Open(backupFileFullPath, FileMode.Open, FileAccess.Read);
#if !DotNetCoreClrLinux
                            Helpers.SetIoPriorityHint(fileStream.SafeFileHandle, Kernel32Types.PRIORITY_HINT.IoPriorityHintVeryLow);
#endif
                            reader = new StreamReader(fileStream);
                        }
                        catch (FileNotFoundException)
                        {
                            backupFileNotPresent = true;
                        }
                        catch (DirectoryNotFoundException)
                        {
                            backupFileNotPresent = true;
                        }
                    });
            }
            catch (Exception e)
            {
                Utility.TraceSource.WriteExceptionAsError(
                    LogSourceId,
                    e,
                    "Unable to open application activation table backup file {0}",
                    backupFileFullPath);
                return false;
            }

            if (backupFileNotPresent)
            {
                return true;
            }

            try
            {
                // Get the version line
                string versionLine = string.Empty;
                try
                {
                    Utility.PerformIOWithRetries(
                        () =>
                        {
                            versionLine = reader.ReadLine();
                        });
                }
                catch (Exception e)
                {
                    Utility.TraceSource.WriteExceptionAsError(
                        LogSourceId,
                        e,
                        "Unable to read version line from application activation table backup file {0}",
                        backupFileFullPath);
                    return false;
                }

                if (null == versionLine)
                {
                    Utility.TraceSource.WriteError(
                        LogSourceId,
                        "Application activation table backup file {0} does not contain a version line.",
                        backupFileFullPath);
                    return false;
                }

                // Ensure that the version line is compatible
                if (false == versionLine.Equals(VersionString, StringComparison.Ordinal))
                {
                    Utility.TraceSource.WriteError(
                        LogSourceId,
                        "Application activation table backup file {0} has incompatible file version '{1}'.",
                        backupFileFullPath,
                        versionLine);
                    return false;
                }

                // Read the application activation records from the file
                string activationRecord = null;
                for (;;)
                {
                    activationRecord = string.Empty;
                    try
                    {
                        Utility.PerformIOWithRetries(
                            () =>
                            {
                                activationRecord = reader.ReadLine();
                            });
                    }
                    catch (Exception e)
                    {
                        Utility.TraceSource.WriteExceptionAsError(
                            LogSourceId,
                            e,
                            "Unable to retrieve record from application activation table backup file {0}",
                            backupFileFullPath);
                        return false;
                    }

                    if (null == activationRecord)
                    {
                        // Reached end of file
                        Utility.TraceSource.WriteInfo(
                            LogSourceId,
                            "Reached end of application activation table backup file {0}",
                            backupFileFullPath);
                        break;
                    }

                    ProcessActivationRecord(activationRecord, backupFileFullPath);
                }
            }
            finally
            {
                reader.Dispose();
            }

            return true;
        }

        private static void ProcessActivationRecord(string activationRecord, string backupFile)
        {
            string[] activationRecordParts = activationRecord.Split(',');
            if (activationRecordParts.Length != (int)ActivationRecordParts.Count)
            {
                Utility.TraceSource.WriteError(
                    LogSourceId,
                    "Application activation record {0} in application activation table backup file {1} is not in the correct format.",
                    activationRecord,
                    backupFile);
                return;
            }

            string appInstanceId = activationRecordParts[(int)ActivationRecordParts.AppInstanceId].Trim();
            string appType = activationRecordParts[(int)ActivationRecordParts.AppType].Trim();
            string activationTimeBinaryAsString = activationRecordParts[(int)ActivationRecordParts.ActivationTime].Trim();

            long activationTimeBinary;
            if (false == long.TryParse(activationTimeBinaryAsString, out activationTimeBinary))
            {
                Utility.TraceSource.WriteError(
                    LogSourceId,
                    "[Activation record {0}, application activation table backup file {1}]: {2} cannot be parsed as a 64-bit integer value.",
                    activationRecord,
                    backupFile,
                    activationTimeBinaryAsString);
                return;
            }

            DateTime activationTime = DateTime.FromBinary(activationTimeBinary);

            if (false == applicationEtwTracesStartTime.ContainsKey(appType))
            {
                applicationEtwTracesStartTime[appType] = new Dictionary<string, DateTime>();
            }

            applicationEtwTracesStartTime[appType][appInstanceId] = activationTime;
        }

        private static bool SaveToFile()
        {
            // Create a new temp file
            string tempFilePath = Utility.GetTempFileName();

            try
            {
                // Open the temp file
                StreamWriter writer = null;
                try
                {
                    Utility.PerformIOWithRetries(
                        () =>
                            {
                                FileStream fileStream = FabricFile.Open(tempFilePath, FileMode.Create, FileAccess.Write);
#if !DotNetCoreClrLinux
                                Helpers.SetIoPriorityHint(fileStream.SafeFileHandle, Kernel32Types.PRIORITY_HINT.IoPriorityHintVeryLow);
#endif
                                writer = new StreamWriter(fileStream);
                            });
                }
                catch (Exception e)
                {
                    Utility.TraceSource.WriteExceptionAsError(
                        LogSourceId,
                        e,
                        "Failed to open temp file {0} for backing up application activation table.",
                        tempFilePath);
                    return false;
                }

                try
                {
                    // Write the version information to the backup file
                    try
                    {
                        Utility.PerformIOWithRetries(
                            () =>
                            {
                                writer.WriteLine(VersionString);
                            });
                    }
                    catch (Exception e)
                    {
                        Utility.TraceSource.WriteExceptionAsError(
                            LogSourceId,
                            e,
                            "Failed to write version information to temp file {0} for backing up application activation table.",
                            tempFilePath);
                        return false;
                    }

                    // Write the activation records to the backup file
                    foreach (string appType in applicationEtwTracesStartTime.Keys)
                    {
                        foreach (string appInstanceId in applicationEtwTracesStartTime[appType].Keys)
                        {
                            DateTime activationTime = applicationEtwTracesStartTime[appType][appInstanceId];
                            string activationRecord = string.Join(
                                                          ",",
                                                          appInstanceId,
                                                          appType,
                                                          activationTime.ToBinary().ToString(),
                                                          activationTime.ToString());
                            try
                            {
                                Utility.PerformIOWithRetries(
                                    () =>
                                    {
                                        writer.WriteLine(activationRecord);
                                    });
                            }
                            catch (Exception e)
                            {
                                Utility.TraceSource.WriteExceptionAsError(
                                    LogSourceId,
                                    e,
                                    "Failed to write record {0} to temp file {1} for backing up application activation table.",
                                    activationRecord,
                                    tempFilePath);
                                return false;
                            }
                        }
                    }
                }
                finally
                {
                    writer.Dispose();
                }

                // Copy the temp file as the new backup file
                try
                {
                    Utility.PerformIOWithRetries(
                        () =>
                        {
                            FabricFile.Copy(tempFilePath, backupFileFullPath, true);
                        });
                }
                catch (Exception e)
                {
                    Utility.TraceSource.WriteExceptionAsError(
                        LogSourceId,
                        e,
                        "Failed to copy file {0} to {1} for backing up application activation table.",
                        tempFilePath,
                        backupFileFullPath);
                    return false;
                }

                Utility.TraceSource.WriteInfo(
                    LogSourceId,
                    "Application activation table backup file {0} created.",
                    backupFileFullPath);
            }
            finally
            {
                try
                {
                    Utility.PerformIOWithRetries(
                        () =>
                        {
                            FabricFile.Delete(tempFilePath);
                        });
                }
                catch (Exception e)
                {
                    Utility.TraceSource.WriteExceptionAsError(
                        LogSourceId,
                        e,
                        "Failed to delete temp file {0} which was created for backing up application activation table.",
                        tempFilePath);
                }
            }

            return true;
        }
    }
}