// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace TStore
    {
        class FilePropertySection : public KObject<FilePropertySection>, public KShared<FilePropertySection>
        {
            K_FORCE_SHARED_WITH_INHERITANCE(FilePropertySection);

        public:
            virtual void Write(__in BinaryWriter&);

            template<typename TProperties>
            static KSharedPtr<TProperties> Read(
                __in BinaryReader& reader,
                __in BlockHandle const & handle,
                __in KAllocator& allocator)
            {
                KSharedPtr<TProperties> properties;
                NTSTATUS status = TProperties::Create(allocator, properties);
                Diagnostics::Validate(status);
                if (handle.Offset < 0)
                {
                    throw ktl::Exception(K_STATUS_OBJECT_NOT_ATTACHED); //todo: Pick the right status for InvalidDataException(SR.Error_FilePropertySection_Missing);
                }

                if (handle.Size == 0)
                {
                    return properties;
                }

                // The FileProperties start at the given 'offset' within the stream, and contains properties up to 'offset' + 'size'.
                reader.Position = static_cast<ULONG32>(handle.Offset);

                while (reader.Position < static_cast<ULONG32>(handle.EndOffset()))
                {
                    // Read the property name.
                    ULONG32 property = 0;
                    reader.Read(property);

                    // Read the size in bytes of the property's value.
                    ULONG32 valueSize = VarInt::ReadInt32(reader);
                    if (valueSize < 0)
                    {
                        throw ktl::Exception(K_STATUS_OBJECT_NOT_ATTACHED); //todo: Pick the right status for InvalidDataException(string.Format(CultureInfo.CurrentCulture, SR.Error_FilePropertySection_CorruptSizeNegative_OneArgs, valueSize));
                    }

                    if (reader.Position + valueSize > handle.EndOffset())
                    {
                        throw ktl::Exception(K_STATUS_OBJECT_NOT_ATTACHED); //todo: Pick the right status for InvalidDataException(SR.Error_FilePropertySection_CorruptSizeExtend);
                    }

                    // Read the property's value.
                    properties->ReadProperty(reader, property, valueSize);
                }

                // Validate the properties section ended exactly where expected.
                if (reader.Position != static_cast<ULONG>(handle.EndOffset()))
                {
                    throw ktl::Exception(K_STATUS_OBJECT_NOT_ATTACHED); //todo: InvalidDataException(SR.Error_FilePropertySection_CorruptSizeIncorrect);
                }

                return properties;
            }
            
        protected:
            virtual void ReadProperty(
                __in BinaryReader& reader,
                __in ULONG32 property,
                __in ULONG32 valueSize);
        };
    }
}
