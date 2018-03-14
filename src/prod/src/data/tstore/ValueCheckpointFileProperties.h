// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace TStore
    {
        //
        // Properties for the ValueCheckpointFile.
        //
        class ValueCheckpointFileProperties :
            public FilePropertySection
        {
            K_FORCE_SHARED(ValueCheckpointFileProperties)

        public:

            static NTSTATUS
                Create(
                    __in KAllocator& allocator,
                    __out ValueCheckpointFileProperties::SPtr& result);

            __declspec(property(get = get_ValuesHandle, put = set_ValuesHandle)) BlockHandle::SPtr ValuesHandle;
            BlockHandle::SPtr get_ValuesHandle() const
            {
                return valuesHandleSPtr_;
            }
            void set_ValuesHandle(__in BlockHandle& value)
            {
                valuesHandleSPtr_ = &value;
            }

            __declspec(property(get = get_ValueCount, put = set_ValueCount)) ULONG64 ValueCount;
            ULONG64 get_ValueCount() const
            {
                return valueCount_;
            }
            void set_ValueCount(__in ULONG64 value)
            {
                valueCount_ = value;
            }

            __declspec(property(get = get_FileId, put = set_FileId)) ULONG32 FileId;
            ULONG32 get_FileId() const
            {
                return fileId_;
            }
            void set_FileId(__in ULONG32 value)
            {
                fileId_ = value;
            }

            //
            // Serialize ValueCheckpointFileProperties into the given stream.
            // The data is written is 8 bytes aligned.
            // Name                Type        Size
            // 
            // ValuesHandle.PID    int         4
            // SerializedSize      VarInt      1
            // RESERVED                        3
            // KeyHandle           bytes       16
            // 
            // ValueCount.PID      int         4
            // SerializedSize      VarInt      1
            // RESERVED                        3
            // ValueCount          long        8
            // 
            // FileId.PID          int         4
            // Size                VarInt      1
            // RESERVED                        3
            // FileId              bytes       4
            // RESERVED                        4
            // 
            // RESERVED: Fixed padding that is usable to add fields in future.
            // PADDING:  Due to dynamic size, cannot be used for adding fields.
            //
            void Write(__in BinaryWriter& writer) override;
            
            //
            // the given property value.
            //
            void ReadProperty(
                __in BinaryReader& reader,
                __in ULONG32 property,
                __in ULONG32 valueSize) override;

        private:

            enum PropertyId : LONG32
            {
                ValuesHandleProp = 1,
                ValueCountProp = 2,
                FileIdProp = 3,
            };

            BlockHandle::SPtr valuesHandleSPtr_;
            ULONG64 valueCount_;
            ULONG32 fileId_;

        };
    }
}
