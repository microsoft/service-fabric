// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace TStore
    {
        class KeyCheckpointFileProperties :
            public FilePropertySection
        {
            K_FORCE_SHARED(KeyCheckpointFileProperties)

        public:

            static NTSTATUS
                Create(
                    __in KAllocator& allocator,
                    __out KeyCheckpointFileProperties::SPtr& result);

            __declspec(property(get = get_KeysHandle, put = set_KeysHandle)) BlockHandle::SPtr KeysHandle;
            BlockHandle::SPtr get_KeysHandle() const
            {
                return keysHandleSPtr_;
            }
            void set_KeysHandle(__in BlockHandle& value)
            {
                keysHandleSPtr_ = &value;
            }

            __declspec(property(get = get_KeyCount, put = set_KeyCount)) ULONG64 KeyCount;
            ULONG64 get_KeyCount() const
            {
                return keyCount_;
            }            
            void set_KeyCount(__in ULONG64 value)
            {
                keyCount_ = value;
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
            // Serialize KeyCheckpointFileProperties into the given stream.
            // The data is written is 8 bytes aligned.
            // Name            Type        Size
            // 
            // KeyHandle.PID   int         4
            // SerializedSize  VarInt      1
            // RESERVED                    3
            // KeyHandle       bytes       16
            // 
            // KeyCount.PID    int         4
            // SerializedSize  VarInt      1
            // RESERVED                    3
            // KeyCount        long        8
            // 
            // FileId.PID      int         4
            // Size            VarInt      1
            // RESERVED                    3
            // FileId          bytes       4
            // RESERVED                    4
            // 
            void Write(__in BinaryWriter& writer) override;

            //
            // Read the given property value.
            //
            void ReadProperty(
                __in BinaryReader& reader, 
                __in ULONG32 property, 
                __in ULONG32 valueSize) override;

        private:

            enum PropertyId : LONG32
            {
                KeysHandleProp = 1,
                KeyCountProp = 2,
                FileIdProp = 3,
            };

            ULONG64 keyCount_;
            ULONG32 fileId_;
            BlockHandle::SPtr keysHandleSPtr_;

        };
    }
}
