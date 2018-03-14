// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define POSTMERGEMETADATAINFO_TAG 'psMI'

namespace Data
{
    namespace TStore
    {
        class PostMergeMetadataTableInformation
            : public KObject<PostMergeMetadataTableInformation>
            , public KShared<PostMergeMetadataTableInformation>
        {
            K_FORCE_SHARED(PostMergeMetadataTableInformation)

        public:
            static NTSTATUS Create(
                __in KSharedArray<ULONG32> & deletedFileIds,
                __in FileMetadata::SPtr newMergedFileSPtr, // Can be null
                __in KAllocator & allocator,
                __out SPtr & result);

            __declspec(property(get = get_DeletedFileIds)) KSharedArray<ULONG32>::SPtr DeletedFileIdsSPtr;
            KSharedArray<ULONG32>::SPtr get_DeletedFileIds() const
            {
                return deletedFileIdsSPtr_;
            }

            __declspec(property(get = get_NewMergedFile)) FileMetadata::SPtr NewMergedFileSPtr;
            FileMetadata::SPtr get_NewMergedFile() const
            {
                return newMergedFileSPtr_;
            }

        private:
            PostMergeMetadataTableInformation(__in KSharedArray<ULONG32> & deletedFileIds, __in FileMetadata::SPtr  newMergedFileSPtr);

            KSharedArray<ULONG32>::SPtr deletedFileIdsSPtr_;
            FileMetadata::SPtr newMergedFileSPtr_;
        };
    }
}
