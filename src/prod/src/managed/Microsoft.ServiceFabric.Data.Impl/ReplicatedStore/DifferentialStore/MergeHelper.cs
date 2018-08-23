// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Collections.Generic;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Data.Common;
    using Microsoft.ServiceFabric.Data;

    /// <summary>
    /// Helper class that decides on merge.
    /// </summary>
    internal class MergeHelper
    {
        private static readonly ConditionalValue<List<uint>> DoNotMergeConditionalValue = new ConditionalValue<List<uint>>();

        /// <summary>
        /// Approximate size, 200 MB, of a typical unmerged checkpoint file
        /// </summary>
        private const long ApproxCheckpointFileSize = 200L << 20;

        /// <summary>
        /// Default file count threshold for invalid and deleted entries merge policy.
        /// </summary>
        private const int DefaultMergeFilesCountThreshold = 3;

        /// <summary>
        /// Default unutilization threshold for invalid entries merge policy.
        /// </summary>
        private const int DefaultPercentageOfInvalidEntriesPerFile = 33;

        /// <summary>
        /// Default threshold for percentage of deleted entries for a file to be considered for merge
        /// </summary>
        private const int DefaultPercentageOfDeletedEntriesPerFile = 33;

        /// <summary>
        /// Default threshold, for percentage of deleted/invalid data on disk for Size on Disk Merge Policy to be triggered
        /// </summary>
        private const int DefaultMaxPercentOfDeletedInvalidBytes = 50;

        /// <summary>
        /// File count threshold for invalid and deleted entries merge policy.
        /// </summary>
        private int mergeFilesCountThreshold;

        /// <summary>
        /// Unutilization threshold for invalid entries merge policy.
        /// </summary>
        private int percentageOfInvalidEntriesPerFile;

        /// <summary>
        /// Threshold for percentage of deleted entries for a file to be considered for merge
        /// </summary>
        private int percentageOfDeletedEntriesPerFile;

        /// <summary>
        /// File count merge configruation.
        /// </summary>
        private FileCountMergeConfiguration fileCountMergeConfiguration;

        /// <summary>
        /// File type to merge list used for file count merge policy.
        /// </summary>
        private Dictionary<ushort, List<uint>> fileTypeToMergeList;

        /// <summary>
        /// Get or set gdpr merge threshold.
        /// Default value is 15 days
        /// </summary>
        public long GDPRMergeThresholdInTicks { get; set; } = 15 * TimeSpan.TicksPerDay;

        /// <summary>
        /// Get or set the file count for merge.
        /// </summary>
        /// <devnote>Exposed for testability only.</devnote>
        public int MergeFilesCountThreshold
        {
            get { return this.mergeFilesCountThreshold; }
            set { this.mergeFilesCountThreshold = value; }
        }

        /// <summary>
        /// Get or sets the percentage of invalid entries that will result in merge.
        /// </summary>
        /// <devnote>Exposed for testability only.</devnote>
        public int PercentageOfInvalidEntriesPerFile
        {
            get { return this.percentageOfInvalidEntriesPerFile; }
            set { this.percentageOfInvalidEntriesPerFile = value; }
        }

        /// <summary>
        /// Get or sets the percentage of deleted entries that will result in merge. (0-100)
        /// </summary>
        /// <devnote>Exposed for testability only.</devnote>
        public int PercentageOfDeletedEntriesPerFile
        {
            get { return this.percentageOfDeletedEntriesPerFile; }
            set { this.percentageOfDeletedEntriesPerFile  = value; }
        }

        // Default is zero.
        public int NumberOfInvalidEntries { get; set; }

        public long MaxPercentOfDeletedInvalidBytes { get; set; }

        /// <summary>
        /// Gets the Merge Policy
        /// </summary>
        public MergePolicy MergePolicy { get; internal set; }

        /// <summary>
        /// Gets or sets the file count merge configuration.
        /// </summary>
        public FileCountMergeConfiguration FileCountMergeConfiguration
        {
            get { return this.fileCountMergeConfiguration; }
            set { this.fileCountMergeConfiguration = value; }
        }

        private readonly string traceType;

        /// <summary>
        /// Initializes a new instance of the MergeHelper class.
        /// </summary>
        public MergeHelper(string traceType)
        {
            this.MergePolicy = MergePolicy.All;

            this.traceType = traceType;

            // Invalid entries merge policy
            this.mergeFilesCountThreshold = DefaultMergeFilesCountThreshold;
            this.percentageOfInvalidEntriesPerFile = DefaultPercentageOfInvalidEntriesPerFile;
            
            // Deleted entries 
            this.percentageOfDeletedEntriesPerFile = DefaultPercentageOfDeletedEntriesPerFile;

            // File count merge policy
            this.fileCountMergeConfiguration = new FileCountMergeConfiguration(this.traceType);
            this.fileTypeToMergeList = new Dictionary<ushort, List<uint>>();

            // Size on disk merge policy
            this.MaxPercentOfDeletedInvalidBytes = DefaultMaxPercentOfDeletedInvalidBytes;
        }

        public bool ShouldMerge(MetadataTable mergeTable, ConsolidationMode mode, out List<uint> mergeList)
        {
            if (mode == ConsolidationMode.GDPR && ShouldPerformGDPRMerge(mergeTable, out mergeList))
            {
#if !DotNetCoreClr
                FabricEvents.Events.MergePolicy(this.traceType, MergePolicy.GDPR.ToString());
#endif
                return true;
            }

            if (this.IsMergePolicyEnabled(MergePolicy.SizeOnDisk))
            {
                bool shouldMerge = this.ShouldMergeForSizeOnDiskPolicy(mergeTable);

                if (shouldMerge)
                {
                    mergeList = new List<uint>(mergeTable.Table.Keys);
#if !DotNetCoreClr
                    FabricEvents.Events.MergePolicy(this.traceType, MergePolicy.SizeOnDisk.ToString());
#endif
                    return true;
                }
            }

            // If we have enough files on disk to merge, then check the merge policies
            if (mergeTable.Table.Count < this.MergeFilesCountThreshold)
            {
                mergeList = null;
                return false;
            }

            if (this.IsMergePolicyEnabled(MergePolicy.FileCount))
            {
                ConditionalValue<List<uint>> fileCountMergeConditionalValue = this.ShouldMergeDueToFileCountPolicy(mergeTable);
                if (fileCountMergeConditionalValue.HasValue)
                {
                    mergeList = fileCountMergeConditionalValue.Value;
#if !DotNetCoreClr
                    FabricEvents.Events.MergePolicy(this.traceType, MergePolicy.FileCount.ToString());
#endif
                    return true;
                }
            }

            if (this.IsMergePolicyEnabled(MergePolicy.InvalidEntries) || this.IsMergePolicyEnabled(MergePolicy.DeletedEntries))
            {
                mergeList = this.GetMergeList(mergeTable);
                if (mergeList.Count >= this.MergeFilesCountThreshold)
                {
#if !DotNetCoreClr
                    FabricEvents.Events.MergePolicy(this.traceType, "MergePolicy.Invalid/DeletedEntries");
#endif
                    return true;
                }
            }

            mergeList = null;
            return false;
        }

        public bool ShouldPerformGDPRMerge(MetadataTable mergeTable, out List<uint> mergeList)
        {
            var mergeSet = new HashSet<uint>();

            var currentDateTime = DateTime.UtcNow;
            var cutOffTimeStamp = currentDateTime.Subtract(TimeSpan.FromTicks(GDPRMergeThresholdInTicks)).Ticks;
            long latestDeletedEntryTimeStamp = FileMetadata.InvalidTimeStamp;

            if (this.IsMergePolicyEnabled(MergePolicy.GDPR))
            {
                foreach (var item in mergeTable.Table)
                {
                    var fileMetadata = item.Value;
                    if (fileMetadata.NumberOfDeletedEntries > 0)
                    {
                        if (fileMetadata.OldestDeletedEntryTimeStamp <= cutOffTimeStamp)
                        {
                            mergeSet.Add(item.Key);

                            // Selected checkpoint file could have other deleted entries eligible for removal
                            if (fileMetadata.LatestDeletedEntryTimeStamp > latestDeletedEntryTimeStamp)
                                latestDeletedEntryTimeStamp = fileMetadata.LatestDeletedEntryTimeStamp;
                        }
                    }

                    /*
                     * It is possible in the same differential set after a key is deleted it is added back
                     * again. In this case deleted entry is then not written to checkpoint files instead
                     * entries in old checkpoint file are invalidated. In order to delete those values in old
                     * checkpoint we add following condition. Invalid entry currently does not move to newer 
                     * checkpoint files on merge. 
                     */
                    if (fileMetadata.NumberOfInvalidEntries > 0 && fileMetadata.TimeStamp <= cutOffTimeStamp)
                    {
                        mergeSet.Add(item.Key);
                    }
                }

                if (latestDeletedEntryTimeStamp != FileMetadata.InvalidTimeStamp)
                {
                    foreach (var item in mergeTable.Table)
                    {
                        var fileMetadata = item.Value;
                        if (fileMetadata.TimeStamp <= latestDeletedEntryTimeStamp)
                            mergeSet.Add(item.Key);
                    }
                }

                if (mergeSet.Count > 0)
                {
                    mergeList = new List<uint>(mergeSet);
                    return true;
                }
            }

            mergeList = null;
            return false;
        }

        /// <summary>
        /// Determine which file ids should be merged.
        /// </summary>
        /// <param name="mergeTable">Table of file metadata tables</param>
        /// <returns>List of fileIds to be merged</returns>
        public List<uint> GetMergeList(MetadataTable mergeTable)
        {
            var mergeFileIds = new List<uint>();

            bool invalidEntriesEnabled = this.IsMergePolicyEnabled(MergePolicy.InvalidEntries);
            bool deletedEntriesEnabled = this.IsMergePolicyEnabled(MergePolicy.DeletedEntries);

            var mergeSet = new HashSet<uint>();
            foreach (var item in mergeTable.Table)
            {
                if (invalidEntriesEnabled && IsFileQualifiedForInvalidEntriesMergePolicy(item))
                {
                    mergeSet.Add(item.Key);
                }
                else if (deletedEntriesEnabled && IsFileQualifiedForDeletedEntriesMergePolicy(item))
                {
                    mergeSet.Add(item.Key);
                }
            }

            return new List<uint>(mergeSet);
        }

        /// <summary>
        /// Check if the input file qualifies for invalid entries merge
        /// </summary>
        /// <param name="item">File to be check for merging</param>
        /// <returns>True if the file should be merged, false otherwise</returns>
        private bool IsFileQualifiedForInvalidEntriesMergePolicy(KeyValuePair<uint, FileMetadata> item)
        {
            var fileMetadata = item.Value;

            // TODO: Fix the workaround below until item 5861402 is fixed.
            var noOfValidEntries = fileMetadata.NumberOfValidEntries;
            if (noOfValidEntries < 0)
            {
                noOfValidEntries = 0;
            }

            var invalidEntries = fileMetadata.TotalNumberOfEntries - noOfValidEntries;

            if (this.NumberOfInvalidEntries > 0)
            {
                if (invalidEntries >= this.NumberOfInvalidEntries)
                {
                    return true;
                }
            }
            else
            {
                // Calculate percentage of invalid entries.
                var percentageOfInvalidEntries = (int)((invalidEntries * 100.0f) / fileMetadata.TotalNumberOfEntries);

                if (percentageOfInvalidEntries >= this.PercentageOfInvalidEntriesPerFile)
                {
                    return true;
                }
            }

            return false;
        }

        /// <summary>
        /// Check if the input file qualifies for deleted entries merge
        /// </summary>
        /// <param name="item">File to be check for merging</param>
        /// <returns>True if the file should be merged, false otherwise</returns>
        private bool IsFileQualifiedForDeletedEntriesMergePolicy(KeyValuePair<uint, FileMetadata> item)
        {
            var fileMetadata = item.Value;

            var numDeletedEntries = fileMetadata.NumberOfDeletedEntries;

            if (numDeletedEntries > 0)
            {
                var percentageOfDeletedEntries = (int)((numDeletedEntries * 100.0f) / fileMetadata.TotalNumberOfEntries);

                // If the percentage of deleted entries in this file is greater that the per file threshold, pick it up for merge.
                if (percentageOfDeletedEntries >= this.percentageOfDeletedEntriesPerFile)
                {
                    return true;
                }
            }

            return false;
        }

        /// <summary>
        /// Decides whether files should be merged and which files according to File Count Merge Policy
        /// </summary>
        /// <param name="mergeTable">Current metadata table.</param>
        /// <returns>Conditional value whose value is the list of files to be merged.</returns>
        internal ConditionalValue<List<uint>> ShouldMergeDueToFileCountPolicy(MetadataTable mergeTable)
        {
            // If total number of checkpoint files is below the merge threshold, merge is not required.
            if (mergeTable.Table.Count < this.fileCountMergeConfiguration.FileCountMergeThreshold)
            {
                return DoNotMergeConditionalValue;
            }

            // Following code assumes that there is one ShouldMerge call at a time.
            this.AssertIfMapIsNotClean();

            foreach (var item in mergeTable.Table)
            {
                uint fileId = item.Key;
                FileMetadata fileMetadata = item.Value;
                long fileSize = fileMetadata.GetFileSize();

                ushort fileType = this.fileCountMergeConfiguration.GetFileType(fileSize);
                bool listExists = this.fileTypeToMergeList.ContainsKey(fileType);
                if (listExists == false)
                {
                    this.fileTypeToMergeList.Add(fileType, new List<uint>());
                }

                this.fileTypeToMergeList[fileType].Add(fileId);
                if (this.fileTypeToMergeList[fileType].Count == this.fileCountMergeConfiguration.FileCountMergeThreshold)
                {
                    var mergeList = this.fileTypeToMergeList[fileType];
                    bool isRemoved = this.fileTypeToMergeList.Remove(fileType);
                    Diagnostics.Assert(isRemoved, this.traceType, "Remove must be successful.");
                    this.CleanMap();
                    return new ConditionalValue<List<uint>>(true, mergeList);
                }
            }

            this.CleanMap();
            return DoNotMergeConditionalValue;
        }

        /// <summary>
        /// Gets the amount of deleted and invalid entries in checkpoint file in bytes
        /// Assumes all the keys are of uniform size
        /// Assumes all values are of uniform size
        /// </summary>
        /// <param name="fileMetadata"></param>
        /// <returns></returns>
        private long GetInvalidDeleteEntrySizeInBytes(FileMetadata fileMetadata)
        {
            long keyFileSize = fileMetadata.CheckpointFile.GetKeyCheckpointFileSize();
            long valueFileSize = fileMetadata.CheckpointFile.GetValueCheckpointFileSize();

            Diagnostics.Assert(fileMetadata.NumberOfDeletedEntries + fileMetadata.NumberOfInvalidEntries <= fileMetadata.TotalNumberOfEntries,
                this.traceType, "TotalNumberOfEntries({0}) is less than deleted({1}) + invalid({2}) for fileId {3}.",
                fileMetadata.TotalNumberOfEntries,
                fileMetadata.NumberOfDeletedEntries,
                fileMetadata.NumberOfInvalidEntries,
                fileMetadata.FileId);

            Diagnostics.Assert(fileMetadata.TotalNumberOfEntries >= 0,
                this.traceType, "TotalNumberOfEntries({0}) is less than zero for fileID {1}",
                fileMetadata.TotalNumberOfEntries,
                fileMetadata.FileId);

            // deleted entries are only present in key checkpoint file
            long keyCheckpointFileBytesRecoverable = 0;
            if (fileMetadata.TotalNumberOfEntries > 0)
            {
                keyCheckpointFileBytesRecoverable = (long)((double)(fileMetadata.NumberOfDeletedEntries + fileMetadata.NumberOfInvalidEntries) * keyFileSize / fileMetadata.TotalNumberOfEntries);
            }

            Diagnostics.Assert(fileMetadata.NumberOfInvalidEntries <= fileMetadata.CheckpointFile.ValueCount,
                this.traceType, "NumberOfInvalidEntries({0}) is more than number of entries in value checkpointfile({1}) for fileId {2}.",
                fileMetadata.NumberOfInvalidEntries,
                fileMetadata.CheckpointFile.ValueCount,
                fileMetadata.FileId);

            Diagnostics.Assert(fileMetadata.CheckpointFile.ValueCount >= 0,
                this.traceType, "Number of values in ValueCheckpointFile({0}) is less than zero for fileID {1}",
                fileMetadata.CheckpointFile.ValueCount,
                fileMetadata.FileId);

            long valueCheckpointFileBytesRecoverable = 0;
            if (fileMetadata.CheckpointFile.ValueCount > 0)
            {
                valueCheckpointFileBytesRecoverable = (long)((double)(fileMetadata.NumberOfInvalidEntries) * valueFileSize / fileMetadata.CheckpointFile.ValueCount);
            }

            var totalRecoverableBytes = keyCheckpointFileBytesRecoverable + valueCheckpointFileBytesRecoverable;
            return totalRecoverableBytes;
        }

        internal bool ShouldMergeForSizeOnDiskPolicy(MetadataTable mergeTable)
        {
            long totalSize = 0;
            long totalRecoverableBytes = 0;
            if (mergeTable == null || mergeTable.Table == null)
                return false;
            foreach (var fileMetadata in mergeTable.Table.Values)
            {
                totalSize += fileMetadata.GetFileSize();

                totalRecoverableBytes += GetInvalidDeleteEntrySizeInBytes(fileMetadata);
            }

            var percentOfDeletedInvalidBytes = (int)(totalRecoverableBytes * 100.0f / totalSize);
            return percentOfDeletedInvalidBytes >= this.MaxPercentOfDeletedInvalidBytes;
        }

        private bool IsMergePolicyEnabled(MergePolicy mergePolicy)
        {
            int tmpMergePolicy = (int)mergePolicy;
            return ((int)this.MergePolicy & tmpMergePolicy) == tmpMergePolicy;
        }

        private void CleanMap()
        {
            foreach (var list in this.fileTypeToMergeList)
            {
                list.Value.Clear();
            }
        }

        private void AssertIfMapIsNotClean()
        {
            foreach (var list in this.fileTypeToMergeList)
            {
                Diagnostics.Assert(list.Value.Count == 0, this.traceType, "All lists must be empty. Key {0} Count {1}", list.Key, list.Value.Count);
            }
        }
    }
}
 
