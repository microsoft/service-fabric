// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define FILEMETA_COMPARER_TAG 'cdMF'

namespace Data
{
    namespace TStore
    {
        class FileMetaDataComparer : 
            public IComparer<FileMetadata::SPtr>
            , public KObject<FileMetaDataComparer>
            , public KShared<FileMetaDataComparer>
        {
            K_FORCE_SHARED(FileMetaDataComparer)
            K_SHARED_INTERFACE_IMP(IComparer)

        public:
            static NTSTATUS Create(
                __in KAllocator & allocator,
                __out FileMetaDataComparer::SPtr & result);

            int Compare(__in const FileMetadata::SPtr & keyOne, __in const FileMetadata::SPtr & keyTwo) const override;
        };
    }
}
