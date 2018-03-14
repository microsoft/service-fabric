// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
using namespace Data::TStore;

        StringStateSerializer::StringStateSerializer(Data::Utilities::Encoding encoding) : encoding_(encoding)
        {
        }

        StringStateSerializer::~StringStateSerializer()
        {
        }

        NTSTATUS StringStateSerializer::Create(
            __in KAllocator & allocator,
            __out StringStateSerializer::SPtr & result,
            __in_opt Data::Utilities::Encoding encoding)
        {
            result = _new(STRING_SERIALIZER_TAG, allocator) StringStateSerializer(encoding);

            if (!result)
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            return STATUS_SUCCESS;
        }

        void StringStateSerializer::Write(
            __in KString::SPtr value,
            __in BinaryWriter& binaryWriter)
        {
            binaryWriter.Write(*value, encoding_);
        }

        KString::SPtr StringStateSerializer::Read(__in BinaryReader& binaryReader)
        {
            KString::SPtr value;
            binaryReader.Read(value, encoding_);
            return value;
        }
