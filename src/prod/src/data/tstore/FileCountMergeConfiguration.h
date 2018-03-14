// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data 
{
    namespace TStore
    {
        //todo: incomplete class, more work required.
        class FileCountMergeConfiguration : public KObject<FileCountMergeConfiguration>, public KShared<FileCountMergeConfiguration>
        {
            K_FORCE_SHARED(FileCountMergeConfiguration)

        public:
            // 
            // File size threshold for a file to be considered very small: 1 MB
            // 
            static const ULONG64 DefaultVerySmallFileSizeThreshold = 1024 * 1024;

            //
            // File size threshold for a file to be considered small: 16 MB
            //
            static const ULONG64 DefaultSmallFileSizeThreshold = DefaultVerySmallFileSizeThreshold * 16;

            //
            // File size threshold for a file to be considered medium: 256 MB
            //
            static const ULONG64 DefaultMediumFileSizeThreshold = DefaultSmallFileSizeThreshold * 16;

            //
            // File size threshold for a file to be considered large: 4 GB
            //
            static const ULONG64 DefaultLargeFileSizeThreshold = DefaultMediumFileSizeThreshold * 16;

            static NTSTATUS Create(__in ULONG32 mergeThreshold, __in KAllocator& allocator, __out SPtr& result);
            static NTSTATUS Create(__in KAllocator& allocator, __out SPtr& result);

            __declspec (property(get = get_fileCountMergeThreshold, put = set_fileCountMergeThreshold)) ULONG32 FileCountMergeThreshold;
            ULONG32 get_fileCountMergeThreshold() const
            {
                return fileCountMergeThreshold_;
            }

            void set_fileCountMergeThreshold(__in ULONG32 count)
            {
                fileCountMergeThreshold_ = count;
            }

            ULONG32 GetFileType(__in ULONG64 fileSize);

            FileCountMergeConfiguration(__in ULONG32 mergeThreshold);

        private:
            static const ULONG32 DefaultMergeThreshold = 16;
            ULONG32 fileCountMergeThreshold_;

        };

    }
}
