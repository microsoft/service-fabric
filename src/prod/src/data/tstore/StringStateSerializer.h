// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define STRING_SERIALIZER_TAG 'moSS'

namespace Data
{
    namespace TStore
    {
        class StringStateSerializer :
            public KObject<StringStateSerializer>
            , public KShared<StringStateSerializer>
            , public Data::StateManager::IStateSerializer<KString::SPtr>
        {
            K_FORCE_SHARED(StringStateSerializer)
            K_SHARED_INTERFACE_IMP(IStateSerializer)
        public:
            static NTSTATUS Create(
                __in KAllocator & allocator,
                __out StringStateSerializer::SPtr & result,
                __in_opt Data::Utilities::Encoding encoding = Data::Utilities::UTF8);

            void Write(
                __in KString::SPtr value,
                __in Data::Utilities::BinaryWriter& binaryWriter);

            KString::SPtr Read(__in Data::Utilities::BinaryReader& binaryReader);
        private:
            StringStateSerializer(__in Data::Utilities::Encoding encoding);
            
            Data::Utilities::Encoding encoding_;
        };
    }
}
