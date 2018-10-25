// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.IO;

    using Utility = System.Fabric.Dca.Utility;

    // This class manages the files on disk that store backups for the service 
    // package table
    internal class ServicePackageTableBackup
    {
        // Constants
        private const string TraceType = "ServicePackageTableBackup";
        private const string BackupFileExt = "dat";
        private const string BackupFilePrefix = "ServicePackageTable_";
        private const string BackupFileVersionString = "Version: 1";

        // Directory containing the backup file
        private readonly string backupDirectory;

        // Method to invoke when a service package needs to be added or updated
        private readonly ServicePackageTableManager.AddOrUpdateHandler addOrUpdateServicePackageHandler;

        internal ServicePackageTableBackup(
                     string directory,
                     ServicePackageTableManager.AddOrUpdateHandler addOrUpdateServicePackageHandler)
        {
            this.backupDirectory = directory;
            this.addOrUpdateServicePackageHandler = addOrUpdateServicePackageHandler;
            EtwEventTimestamp etwEventTimestamp = new EtwEventTimestamp();
            etwEventTimestamp.Timestamp = DateTime.MinValue;
            etwEventTimestamp.Differentiator = 0;
            this.LatestBackupTime = etwEventTimestamp;
        }

        internal enum TableRecordParts
        {
            NodeName,
            ApplicationInstanceId,
            ApplicationRolloutVersion,
            ServicePackageName,
            ServiceRolloutVersion,
            RunLayoutRoot,

            // This value is not a part of the table record. It
            // represents the total count of parts
            Count
        }

        private enum TimestampParts
        {
            Timestamp,
            Differentiator,

            // This value does not represent a part of the timestamp string. 
            // It represents the count of parts.
            Count
        }

        // Timestamp of the latest backup file
        internal EtwEventTimestamp LatestBackupTime { get; private set; }

        internal static bool GetBackupFileTimestamp(string backupFileName, out EtwEventTimestamp dataTimestamp)
        {
            dataTimestamp = new EtwEventTimestamp();
            dataTimestamp.Timestamp = DateTime.MinValue;
            dataTimestamp.Differentiator = 0;

            string fileNameWithoutExtension = Path.ChangeExtension(
                                                  backupFileName, 
                                                  null);
            string timestampAsString = fileNameWithoutExtension.Remove(
                                           0, 
                                           BackupFilePrefix.Length);

            string[] timestampParts = timestampAsString.Split('_');
            if (timestampParts.Length != (int)TimestampParts.Count)
            {
                Utility.TraceSource.WriteError(
                    TraceType,
                    "Timestamp information {0} in file name {1} is not in the correct format.",
                    timestampAsString,
                    backupFileName);
                return false;
            }

            long timestampAsBinary;
            if (false == long.TryParse(
                             timestampParts[(int)TimestampParts.Timestamp],
                             out timestampAsBinary))
            {
                Utility.TraceSource.WriteError(
                    TraceType,
                    "Unable to parse {0} in file name {1} as a long integer.",
                    timestampParts[(int)TimestampParts.Timestamp],
                    backupFileName);
                return false;
            }

            DateTime timestamp = DateTime.FromBinary(timestampAsBinary);

            int differentiator;
            if (false == int.TryParse(
                             timestampParts[(int)TimestampParts.Differentiator],
                             out differentiator))
            {
                Utility.TraceSource.WriteError(
                    TraceType,
                    "Unable to parse {0} in file name {1} as a long integer.",
                    timestampParts[(int)TimestampParts.Differentiator],
                    backupFileName);
                return false;
            }

            dataTimestamp.Timestamp = timestamp;
            dataTimestamp.Differentiator = differentiator;
            return true;
        }

        internal bool Read()
        {
            // Get the backup files
            // Note: There might be more than one backup file if we encountered
            // errors during previous attempts to delete one or more files.
            string backupFilePattern = string.Concat(
                                           BackupFilePrefix,
                                           "*.",
                                           BackupFileExt);
            DirectoryInfo dirInfo = new DirectoryInfo(this.backupDirectory);
            FileInfo[] backupFiles = dirInfo.GetFiles(backupFilePattern);

            // If we didn't find any backup files, then return immediately
            if (backupFiles.Length == 0)
            {
                Utility.TraceSource.WriteInfo(
                    TraceType,
                    "No backup files found in directory {0}",
                    this.backupDirectory);
                return true;
            }

            // Get the latest backup file. We only care about the latest.
            Array.Sort(backupFiles, this.CompareFileLastWriteTimes);
            FileInfo backupFile = backupFiles[0];

            // Get the timestamp up to which up to which the backup file is valid.
            EtwEventTimestamp dataTimestamp;
            if (false == GetBackupFileTimestamp(
                             backupFile.Name, 
                             out dataTimestamp))
            {
                return false;
            }

            this.LatestBackupTime = dataTimestamp;

            Utility.TraceSource.WriteInfo(
                TraceType,
                "Backup file {0} is valid up to {1} ({2}, {3}).",
                backupFile.FullName,
                dataTimestamp.Timestamp,
                dataTimestamp.Timestamp.Ticks,
                dataTimestamp.Differentiator);

            Utility.TraceSource.WriteInfo(
                TraceType,
                "Reading backup file {0} ...",
                backupFile.FullName);

            // Read data from the backup file
            return this.ReadFromBackupFile(backupFile.FullName, dataTimestamp);
        }

        internal bool Update(Dictionary<string, ServicePackageTableRecord> servicePackageTable, EtwEventTimestamp latestDataTimestamp)
        {
            // Create a new temp file
            string tempFilePath = Utility.GetTempFileName();

            string backupFilePath = string.Empty;
            try
            {
                // Open the temp file
                StreamWriter writer = null;
                try
                {
                    Utility.PerformIOWithRetries(
                        () =>
                            {
                                FileStream file = FabricFile.Open(tempFilePath, FileMode.Create, FileAccess.Write);
#if !DotNetCoreClrLinux
                                Helpers.SetIoPriorityHint(file.SafeFileHandle, Kernel32Types.PRIORITY_HINT.IoPriorityHintVeryLow);
#endif
                                writer = new StreamWriter(file);
                            });
                }
                catch (Exception e)
                {
                    Utility.TraceSource.WriteExceptionAsError(
                        TraceType,
                        e,
                        "Failed to open temp file {0}.",
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
                                writer.WriteLine(BackupFileVersionString);
                            });
                    }
                    catch (Exception e)
                    {
                        Utility.TraceSource.WriteExceptionAsError(
                            TraceType,
                            e,
                            "Failed to write version information to temp file {0}.",
                            tempFilePath);
                        return false;
                    }

                    // Write the table records to the backup file
                    foreach (string tableKey in servicePackageTable.Keys)
                    {
                        string tableRecord = string.Concat(
                                                 servicePackageTable[tableKey].NodeName,
                                                 ", ",
                                                 servicePackageTable[tableKey].ApplicationInstanceId,
                                                 ", ",
                                                 servicePackageTable[tableKey].ApplicationRolloutVersion,
                                                 ", ",
                                                 servicePackageTable[tableKey].ServicePackageName,
                                                 ", ",
                                                 servicePackageTable[tableKey].ServiceRolloutVersion,
                                                 ", ",
                                                 servicePackageTable[tableKey].RunLayoutRoot);

                        try
                        {
                            Utility.PerformIOWithRetries(
                                () =>
                                {
                                    writer.WriteLine(tableRecord);
                                });
                        }
                        catch (Exception e)
                        {
                            Utility.TraceSource.WriteExceptionAsError(
                                TraceType,
                                e,
                                "Failed to write record {0} to temp file {1}",
                                tableRecord,
                                tempFilePath);
                            return false;
                        }
                    }
                }
                finally
                {
                    writer.Dispose();
                }

                // Compute the name of the backup file
                long timstampBinary = latestDataTimestamp.Timestamp.ToBinary();
                string fileName = string.Concat(
                                      BackupFilePrefix,
                                      timstampBinary.ToString("D20", CultureInfo.InvariantCulture),
                                      "_",
                                      latestDataTimestamp.Differentiator.ToString("D10", CultureInfo.InvariantCulture),
                                      ".",
                                      BackupFileExt);
                backupFilePath = Path.Combine(this.backupDirectory, fileName);

                // Copy the temp file as the new backup file
                try
                {
                    Utility.PerformIOWithRetries(
                        () =>
                        {
                            FabricFile.Copy(tempFilePath, backupFilePath, true);
                        });
                }
                catch (Exception e)
                {
                    Utility.TraceSource.WriteExceptionAsError(
                        TraceType,
                        e,
                        "Failed to copy file {0} to {1}",
                        tempFilePath,
                        backupFilePath);
                    return false;
                }

                Utility.TraceSource.WriteInfo(
                    TraceType,
                    "Backup file {0} created. The backup file is valid up to timestamp {1} ({2}, {3}).",
                    backupFilePath,
                    latestDataTimestamp.Timestamp,
                    latestDataTimestamp.Timestamp.Ticks,
                    latestDataTimestamp.Differentiator);
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
                        TraceType,
                        e,
                        "Failed to delete temp file {0}",
                        tempFilePath);
                }
            }

            // Update the latest backup time
            this.LatestBackupTime = latestDataTimestamp;

            // Delete older backup files
            string backupFilePattern = string.Concat(
                                           BackupFilePrefix,
                                           "*.",
                                           BackupFileExt);
            string[] backupFiles = FabricDirectory.GetFiles(
                                       this.backupDirectory, 
                                       backupFilePattern);
            foreach (string fileToDelete in backupFiles)
            {
                if (fileToDelete.Equals(
                        backupFilePath, 
                        StringComparison.OrdinalIgnoreCase))
                {
                    // Don't delete the current backup file
                    continue;
                }

                try
                {
                    Utility.PerformIOWithRetries(
                        () =>
                        {
                            FabricFile.Delete(fileToDelete);
                        });
                }
                catch (Exception e)
                {
                    // Deletion is on a best-effort basis. Log an error and 
                    // continue.
                    Utility.TraceSource.WriteExceptionAsError(
                        TraceType,
                        e,
                        "Failed to delete old backup file {0}",
                        fileToDelete);
                }
            }

            return true;
        }

        private int CompareFileLastWriteTimes(FileInfo file1, FileInfo file2)
        {
            // We want the file with the more recent last write time to come first
            return DateTime.Compare(file2.LastWriteTime, file1.LastWriteTime);
        }

        private bool ReadFromBackupFile(string backupFilePath, EtwEventTimestamp dataTimestamp)
        {
            // Open the backup file
            StreamReader reader = null;
            try
            {
                Utility.PerformIOWithRetries(
                    () =>
                    {
                        FileStream file = FabricFile.Open(backupFilePath, FileMode.Open, FileAccess.Read);
#if !DotNetCoreClrLinux
                        Helpers.SetIoPriorityHint(file.SafeFileHandle, Kernel32Types.PRIORITY_HINT.IoPriorityHintVeryLow);
#endif
                        reader = new StreamReader(file);
                    });
            }
            catch (Exception e)
            {
                Utility.TraceSource.WriteExceptionAsError(
                    TraceType,
                    e,
                    "Unable to open backup file {0}",
                    backupFilePath);
                return false;
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
                        TraceType,
                        e,
                        "Unable to read version line from backup file {0}",
                        backupFilePath);
                    return false;
                }

                if (null == versionLine)
                {
                    Utility.TraceSource.WriteError(
                        TraceType,
                        "Backup file {0} does not contain a version line.",
                        backupFilePath);
                    return false;
                }

                // Ensure that the version line is compatible
                if (false == versionLine.Equals(
                                 BackupFileVersionString,
                                 StringComparison.Ordinal))
                {
                    Utility.TraceSource.WriteError(
                        TraceType,
                        "Backup file {0} has incompatible file version '{1}'.",
                        backupFilePath,
                        versionLine);
                    return false;
                }

                // Read the service package table rows from the file
                string tableRecord = null;
                for (;;)
                {
                    tableRecord = string.Empty;
                    try
                    {
                        Utility.PerformIOWithRetries(
                            () =>
                            {
                                tableRecord = reader.ReadLine();
                            });
                    }
                    catch (Exception e)
                    {
                        Utility.TraceSource.WriteExceptionAsError(
                            TraceType,
                            e,
                            "Unable to retrieve table record from backup file {0}",
                            backupFilePath);
                        return false;
                    }

                    if (null == tableRecord)
                    {
                        // Reached end of file
                        Utility.TraceSource.WriteInfo(
                            TraceType,
                            "Reached end of backup file {0}",
                            backupFilePath);
                        break;
                    }

                    this.ProcessTableRecord(tableRecord, backupFilePath, dataTimestamp);
                }
            }
            finally
            {
                reader.Dispose();
            }

            return true;
        }

        private void ProcessTableRecord(string tableRecord, string backupFilePath, EtwEventTimestamp dataTimestamp)
        {
            string[] tableRecordParts = tableRecord.Split(',');
            if (tableRecordParts.Length != (int)TableRecordParts.Count)
            {
                Utility.TraceSource.WriteError(
                    TraceType,
                    "Table record {0} in backup file {1} is not in the correct format.",
                    tableRecord,
                    backupFilePath);
                return;
            }

            string nodeName = tableRecordParts[(int)TableRecordParts.NodeName].Trim();
            string appInstanceId = tableRecordParts[(int)TableRecordParts.ApplicationInstanceId].Trim();
            string appRolloutVersion = tableRecordParts[(int)TableRecordParts.ApplicationRolloutVersion].Trim();
            string servicePackageName = tableRecordParts[(int)TableRecordParts.ServicePackageName].Trim();
            string serviceRolloutVersion = tableRecordParts[(int)TableRecordParts.ServiceRolloutVersion].Trim();
            string runLayoutRoot = tableRecordParts[(int)TableRecordParts.RunLayoutRoot].Trim();

            ServicePackageTableRecord record = new ServicePackageTableRecord(
                nodeName,
                appInstanceId,
                appRolloutVersion,
                servicePackageName,
                serviceRolloutVersion,
                runLayoutRoot,
                default(DateTime));

            this.addOrUpdateServicePackageHandler(
                nodeName, 
                appInstanceId, 
                DateTime.MaxValue, // service package activation time unknown
                servicePackageName, 
                record, 
                dataTimestamp, 
                false);
        }
    }
}