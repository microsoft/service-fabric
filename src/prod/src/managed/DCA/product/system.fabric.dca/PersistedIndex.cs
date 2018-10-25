// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.IO;

    // Class that maintains a dictionary that is persisted to a file
    internal class PersistedIndex<TItem>
    {
        private const string VersionPrefix = "Version: ";
        private static readonly object FileLock = new object();
        private readonly Dictionary<TItem, int> dictionary;
        private FabricEvents.ExtensionsEvents traceSource;
        private string logSourceId;
        private string fileFullPath;
        private int fileFormatVersion;

        internal PersistedIndex(FabricEvents.ExtensionsEvents traceSource, string logSourceId, string fileFullPath, int fileFormatVersion)
        {
            this.dictionary = new Dictionary<TItem, int>();
            this.InitializeCommon(traceSource, logSourceId, fileFullPath, fileFormatVersion);
        }

        internal PersistedIndex(FabricEvents.ExtensionsEvents traceSource, string logSourceId, IEqualityComparer<TItem> comparer, string fileFullPath, int fileFormatVersion)
        {
            this.dictionary = new Dictionary<TItem, int>(comparer);
            this.InitializeCommon(traceSource, logSourceId, fileFullPath, fileFormatVersion);
        }

        private enum MapRecordParts
        {
            Item,
            Index,

            // This value is not a part of the map record. It
            // represents the total count of parts.
            Count
        }

        internal bool GetIndex(TItem key, out int index)
        {
            index = -1;
            lock (FileLock)
            {
                if (false == this.LoadFromFile())
                {
                    return false;
                }

                if (this.dictionary.ContainsKey(key))
                {
                    index = this.dictionary[key];
                    return true;
                }

                index = this.dictionary.Keys.Count;
                this.dictionary[key] = index;
                if (false == this.SaveToFile())
                {
                    this.dictionary.Remove(key);
                    return false;
                }

                return true;
            }
        }

        private void InitializeCommon(FabricEvents.ExtensionsEvents traceSource, string logSourceId, string fileFullPath, int fileFormatVersion)
        {
            this.traceSource = traceSource;
            this.logSourceId = logSourceId;
            this.fileFullPath = fileFullPath;
            this.fileFormatVersion = fileFormatVersion;
        }

        private bool LoadFromFile()
        {
            // Open the map file
            StreamReader reader = null;
            bool mapFileNotPresent = false;
            try
            {
                Utility.PerformIOWithRetries(
                    () =>
                    {
                        try
                        {
                            FileStream fileStream = FabricFile.Open(this.fileFullPath, FileMode.Open, FileAccess.Read);
#if !DotNetCoreClrLinux
                            Helpers.SetIoPriorityHint(fileStream.SafeFileHandle, Kernel32Types.PRIORITY_HINT.IoPriorityHintVeryLow);
#endif
                            reader = new StreamReader(fileStream);
                        }
                        catch (FileNotFoundException)
                        {
                            mapFileNotPresent = true;
                        }
                        catch (DirectoryNotFoundException)
                        {
                            mapFileNotPresent = true;
                        }
                    });
            }
            catch (Exception e)
            {
                this.traceSource.WriteExceptionAsError(
                    this.logSourceId,
                    e,
                    "Unable to open index map file {0}",
                    this.fileFullPath);
                return false;
            }

            if (mapFileNotPresent)
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
                    this.traceSource.WriteExceptionAsError(
                        this.logSourceId,
                        e,
                        "Unable to read version line from index map file {0}",
                        this.fileFullPath);
                    return false;
                }

                if (null == versionLine)
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Index map file {0} does not contain a version line.",
                        this.fileFullPath);
                    return false;
                }

                // Ensure that the version line is compatible
                string versionString = versionLine.Remove(0, VersionPrefix.Length);
                int version;
                if (false == int.TryParse(
                                versionString,
                                out version))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Version {0} in index map file {1} cannot be parsed as an integer.",
                        versionString,
                        this.fileFullPath);
                    return false;
                }

                if (version != this.fileFormatVersion)
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Index map file {0} has incompatible file version '{1}'.",
                        this.fileFullPath,
                        version);
                    return false;
                }

                // Read the map records from the file
                string mapRecord = null;
                for (;;)
                {
                    mapRecord = string.Empty;
                    try
                    {
                        Utility.PerformIOWithRetries(
                            () =>
                            {
                                mapRecord = reader.ReadLine();
                            });
                    }
                    catch (Exception e)
                    {
                        this.traceSource.WriteExceptionAsError(
                            this.logSourceId,
                            e,
                            "Unable to retrieve record from index map file {0}",
                            this.fileFullPath);
                        return false;
                    }

                    if (null == mapRecord)
                    {
                        // Reached end of file
                        this.traceSource.WriteInfo(
                            this.logSourceId,
                            "Reached end of index map file {0}",
                            this.fileFullPath);
                        break;
                    }

                    this.ProcessMapRecord(mapRecord, this.fileFullPath);
                }
            }
            finally
            {
                reader.Dispose();
            }

            return true;
        }

        private void ProcessMapRecord(string mapRecord, string mapFile)
        {
            string[] mapRecordParts = mapRecord.Split(',');
            if (mapRecordParts.Length != (int)MapRecordParts.Count)
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Map record {0} in index map file {1} is not in the correct format.",
                    mapRecord,
                    mapFile);
                return;
            }

            string itemAsString = mapRecordParts[(int)MapRecordParts.Item].Trim();
            string indexAsString = mapRecordParts[(int)MapRecordParts.Index].Trim();

            TItem item;
            try
            {
                item = (TItem)Convert.ChangeType(itemAsString, typeof(TItem), CultureInfo.InvariantCulture);
            }
            catch (Exception e)
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "[Map record {0}, index map file {1}]: {2} cannot be parsed as {3}. Exception: {4}",
                    mapRecord,
                    mapFile,
                    itemAsString,
                    typeof(TItem),
                    e);
                return;
            }

            int index;
            if (false == int.TryParse(indexAsString, out index))
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "[Map record {0}, index map file {1}]: {2} cannot be parsed as an integer.",
                    mapRecord,
                    mapFile,
                    indexAsString);
                return;
            }

            this.dictionary[item] = index;
        }

        private bool SaveToFile()
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
                    this.traceSource.WriteExceptionAsError(
                        this.logSourceId,
                        e,
                        "Failed to open temp file {0} for persisting index map.",
                        tempFilePath);
                    return false;
                }

                try
                {
                    // Write the version information to the map file
                    string versionString = string.Concat(VersionPrefix, this.fileFormatVersion.ToString(CultureInfo.InvariantCulture));
                    try
                    {
                        Utility.PerformIOWithRetries(
                            () =>
                            {
                                writer.WriteLine(versionString);
                            });
                    }
                    catch (Exception e)
                    {
                        this.traceSource.WriteExceptionAsError(
                            this.logSourceId,
                            e,
                            "Failed to write version information to temp file {0} for persisting index map.",
                            tempFilePath);
                        return false;
                    }

                    // Write the map records to the map file
                    foreach (TItem item in this.dictionary.Keys)
                    {
                        string mapRecord = string.Concat(
                                                 item.ToString(),
                                                 ", ",
                                                 this.dictionary[item].ToString(CultureInfo.InvariantCulture));

                        try
                        {
                            Utility.PerformIOWithRetries(
                                () =>
                                {
                                    writer.WriteLine(mapRecord);
                                });
                        }
                        catch (Exception e)
                        {
                            this.traceSource.WriteExceptionAsError(
                                this.logSourceId,
                                e,
                                "Failed to write record {0} to temp file {1} for persisting index map.",
                                mapRecord,
                                tempFilePath);
                            return false;
                        }
                    }
                }
                finally
                {
                    writer.Dispose();
                }

                // Copy the temp file as the new map file
                try
                {
                    Utility.PerformIOWithRetries(
                        () =>
                        {
                            FabricFile.Copy(tempFilePath, this.fileFullPath, true);
                        });
                }
                catch (Exception e)
                {
                    this.traceSource.WriteExceptionAsError(
                        this.logSourceId,
                        e,
                        "Failed to copy file {0} to {1} for persisting index map",
                        tempFilePath,
                        this.fileFullPath);
                    return false;
                }

                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Index map file {0} created.",
                    this.fileFullPath);
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
                    this.traceSource.WriteExceptionAsError(
                        this.logSourceId,
                        e,
                        "Failed to delete temp file {0} which was created for persisting index map.",
                        tempFilePath);
                }
            }

            return true;
        }
    }
}