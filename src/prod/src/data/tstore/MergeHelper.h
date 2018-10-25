// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data 
{
    namespace TStore
    {
        class MergeHelper : public KObject<MergeHelper>, public KShared<MergeHelper>
        {
            K_FORCE_SHARED(MergeHelper)

        public:
            static NTSTATUS
                Create(
                    __in KAllocator& Allocator,
                    __out MergeHelper::SPtr& Result);

            ktl::Awaitable<bool> ShouldMerge(
                __in MetadataTable& mergeTable,
                __out KSharedArray<ULONG32>::SPtr& mergeList);

            //
            // Determine which file ids to merge.
            //
            KSharedArray<ULONG32>::SPtr GetMergeList(__in MetadataTable & mergeTable);

            //
            // Get or set the file count for merge.
            // Exposed for testability only.
            //
            __declspec (property(get = get_mergeFilesCountThreshold, put = set_mergeFilesCountThreshold)) ULONG32 MergeFilesCountThreshold;
            ULONG32 get_mergeFilesCountThreshold() const
            {
                return mergeFilesCountThreshold_;
            }
            void set_mergeFilesCountThreshold(__in ULONG32 value)
            {
                mergeFilesCountThreshold_ = value;
            }

            //
            // Get or sets the percentage of invalid entries that will result in merge.
            // Exposed for testability only.
            //
            __declspec (property(get = get_percentageOfInvalidEntriesPerFile, put = set_percentageOfInvalidEntriesPerFile)) ULONG32 PercentageOfInvalidEntriesPerFile;
            ULONG32 get_percentageOfInvalidEntriesPerFile() const
            {
                return percentageOfInvalidEntriesPerFile_;
            }
            void set_percentageOfInvalidEntriesPerFile(__in ULONG32 value)
            {
                percentageOfInvalidEntriesPerFile_ = value;
            }

            //
            // Gets or sets the percentage of deleted entries that will result in merge
            //
            __declspec(property(get = get_PercentageOfDeletedEntriesPerFile, put = set_percentageOfDeletedEntriesPerFile)) ULONG32 PercentageOfDeletedEntriesPerFile;
            ULONG32 get_percentageOfDeletedEntriesPerFile() const
            {
                return percentageOfDeletedEntriesPerFile_;
            }
            void set_percentageOfDeletedEntriesPerFile(__in ULONG32 value)
            {
                percentageOfDeletedEntriesPerFile_ = value;
            }

            // Exposed for testability only.
            __declspec (property(get = get_FileCountMergeConfiguration, put = set_FileCountMergeConfiguration)) FileCountMergeConfiguration::SPtr FileCountMergeConfigurationSPtr;
            FileCountMergeConfiguration::SPtr get_FileCountMergeConfiguration()
            {
                return fileCountMergeConfigurationSPtr_;
            }

            void set_FileCountMergeConfiguration(__in FileCountMergeConfiguration & config)
            {
                fileCountMergeConfigurationSPtr_ = &config;
            }

            // Default is zero.
            ULONG32 NumberOfInvalidEntries;

            ULONG64 SizeOnDiskThreshold;

            // Gets the Merge Policy
            __declspec (property(get = get_mergePolicy, put = set_mergePolicy)) MergePolicy CurrentMergePolicy;
            MergePolicy get_mergePolicy() const
            {
                return mergePolicy_;
            }

            void set_mergePolicy(__in MergePolicy value)
            {
                mergePolicy_ = value;
            }

        
        private:
            static ULONG FileTypeHashFunc(__in const USHORT & key)
            {
                return ~key;
            }


            ktl::Awaitable<bool> ShouldMergeDueToFileCountPolicy(
                __in MetadataTable& mergeTable,
                __out KSharedArray<ULONG32>::SPtr& filesToBeMerged);

            ktl::Awaitable<bool> ShouldMergeForSizeOnDiskPolicy(__in MetadataTable& mergeTable);

            bool IsFileQualifiedForInvalidEntriesMergePolicy(__in Data::KeyValuePair<ULONG32, FileMetadata::SPtr> item);
            bool IsFileQualifiedForDeletedEntriesMergePolicy(__in Data::KeyValuePair<ULONG32, FileMetadata::SPtr> item);

            bool IsMergePolicyEnabled(__in MergePolicy mergePolicy);
            void CleanMap();
            void AssertIfMapIsNotClean();

            // Approximate size, 200 MB, of a typical unmerged checkpoint file.
            static const LONG64 ApproxCheckpointFileSize = 200LL << 20;

            //
            // Default file count threshold for invalid entries merge policy.
            //
            static const ULONG32 DefaultMergeFilesCountThreshold = 3;

            //
            // Default unutilization threshold for invalid entries merge policy.
            //
            static const ULONG32 DefaultPercentageOfInvalidEntriesPerFile = 33;

            //
            // Default threshold for percentage of deleted entries for a file to be considered for merge.
            //
            static const ULONG32 DefaultPercentageOfDeletedEntriesPerFile = 33;

            // Default threshold, ~30 200MB files = 6GB, for total size on disk for Size on Disk Merge Policy to be triggered.
            static const LONG64 DefaultSizeForSizeOnDiskPolicy = 30 * ApproxCheckpointFileSize;

            // Gets or sets the file count merge configuration.  File count merge configruation.
            FileCountMergeConfiguration::SPtr fileCountMergeConfigurationSPtr_;

            //
            // Invalid Entries Merge Policy Configuration
            //
            ULONG32 mergeFilesCountThreshold_;
            ULONG32 percentageOfInvalidEntriesPerFile_;
            ULONG32 percentageOfDeletedEntriesPerFile_;

            MergePolicy mergePolicy_;

            Dictionary<ULONG32, KSharedArray<ULONG32>::SPtr>::SPtr fileTypeToMergeList_;

        };
    }
}
