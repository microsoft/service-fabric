// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define NULLABLE_STRING_SERIALIZER_TAG 'tsSN'

namespace Data
{
    namespace TStore
    {
        class NullableStringStateSerializer :
            public KObject<NullableStringStateSerializer>
            , public KShared<NullableStringStateSerializer>
            , public Data::StateManager::IStateSerializer<KString::SPtr>
        {
            K_FORCE_SHARED(NullableStringStateSerializer)
            K_SHARED_INTERFACE_IMP(IStateSerializer)
        public:
            static NTSTATUS Create(
                __in KAllocator & allocator,
                __out NullableStringStateSerializer::SPtr & result,
                __in_opt Data::Utilities::Encoding encoding = Data::Utilities::UTF8);

            void Write(
                __in KString::SPtr value,
                __in Data::Utilities::BinaryWriter& binaryWriter);

            KString::SPtr Read(__in Data::Utilities::BinaryReader& binaryReader);
        private:
            NullableStringStateSerializer(__in Data::Utilities::Encoding encoding);
            
            Data::Utilities::Encoding encoding_;
        };
    }
}
