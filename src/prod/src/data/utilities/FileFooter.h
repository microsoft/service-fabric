// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Utilities
    {
        class FileFooter : 
            public KObject<FileFooter>, 
            public KShared<FileFooter>
        {
            K_FORCE_SHARED(FileFooter)

        public:
            static NTSTATUS Create(
                __in BlockHandle & propertiesHandle,
                __in ULONG32 version,
                __in KAllocator & allocator,
                __out FileFooter::SPtr & result) noexcept;

            static ULONG32 SerializedSize();

            FileFooter(
                __in BlockHandle & propertiesHandle,
                __in ULONG32 version);

            __declspec(property(get = get_Version)) const ULONG32 Version;
            ULONG32 get_Version() const { return Version_; }

            __declspec(property(get = get_PropertiesHandle)) BlockHandle::SPtr PropertiesHandle;
            const BlockHandle::SPtr get_PropertiesHandle() const { return PropertiesHandle_; }

            static FileFooter::SPtr Read(
                __in BinaryReader & binaryReader,
                __in BlockHandle const & handle,
                __in KAllocator & allocator);

            void Write(__in BinaryWriter & binaryWriter);

        private:
            ULONG32 Version_;
            BlockHandle::SPtr PropertiesHandle_;
        };
    }
}
