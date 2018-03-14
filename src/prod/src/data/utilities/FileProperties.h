// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Utilities
    {
        class FileProperties:
            public KObject<FileProperties>,
            public KShared<FileProperties>
        {
            K_FORCE_SHARED_WITH_INHERITANCE(FileProperties)

        public:
            template<typename TProperties>
            static KSharedPtr<TProperties> Read(
                __in BinaryReader & reader,
                __in BlockHandle const & handle,
                __in KAllocator & allocator);

        public:
            virtual void Write(__in BinaryWriter & binaryWriter) = 0;

            virtual void ReadProperty(
                __in BinaryReader & reader, 
                __in KStringView const & property, 
                __in ULONG32 valueSize);
        };

        template<typename TProperties>
        KSharedPtr<TProperties> FileProperties::Read(
            __in BinaryReader & reader,
            __in BlockHandle const & handle,
            __in KAllocator & allocator)
        {
            KSharedPtr<TProperties> properties;
            NTSTATUS status = TProperties::Create(allocator, properties);
            THROW_ON_FAILURE(status);

            if (handle.Size == 0)
            {
                return properties;
            }

            // The FileProperties start at the given 'offset' within the stream, and contains properties up to 'offset' + 'size'.
            reader.Position = static_cast<ULONG>(handle.Offset);

            while (reader.get_Position() < static_cast<ULONG>(handle.EndOffset()))
            {
                // Read the property name.

                KString::SPtr readString;
                reader.Read(readString);

                // Read the size in bytes of the property's value.
                ULONG valueSize = VarInt::ReadInt32(reader);

                // Read the property's value.
                KStringView propertyName = static_cast<KStringView>(*readString);
                properties->ReadProperty(reader, propertyName, valueSize);
            }

            return properties;
        }
    }
}

