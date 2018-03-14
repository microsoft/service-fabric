// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    class Utility
    {
    public:
        static Common::ErrorCode ValidateString(
            std::wstring const& input,
            size_t minSize = 1,
            size_t maxSize = Common::ParameterValidator::MaxStringSize)
        {
            if (input.length() > maxSize || input.length() < minSize)
            {
                return Common::ErrorCodeValue::EntryTooLarge;
            }
            else
            {
                return Common::ErrorCode::Success();
            }
        }

        static Common::ErrorCode TryParseQueryFilter(
            std::wstring const& inputString,
            DWORD defaultValue,
            __out DWORD &value)
        {
            value = defaultValue;
            if (!Common::StringUtility::TryFromWString<DWORD>(inputString, value))
            {
                return Common::ErrorCode(
                    Common::ErrorCodeValue::InvalidArgument,
                    Common::wformatString(GET_HTTP_GATEWAY_RC(Invalid_Filter_Parameter), inputString));
            }

            return Common::ErrorCode::Success();
        }

        static Common::ErrorCode StringToBytesUtf8(std::wstring const & input, Common::ByteBuffer & buffer)
        {
            int ret = WideCharToMultiByte(
                CP_UTF8,
                WC_ERR_INVALID_CHARS,
                input.c_str(),
                (int)input.length(),
                nullptr,
                0,
                NULL,
                NULL);

            if (ret == 0)
            {
                return Common::ErrorCodeValue::SerializationError;
            }

            buffer.resize(ret);

            ret = WideCharToMultiByte(
                CP_UTF8,
                WC_ERR_INVALID_CHARS,
                input.c_str(),
                (int)input.length(),
                (char*)(buffer.data()),
                ret,
                NULL,
                NULL);

            if (ret == 0)
            {
                return Common::ErrorCodeValue::SerializationError;
            }

            return Common::ErrorCode::Success();
        }

/*#if !defined(PLATFORM_UNIX) // WINDOWS

    #define SERIALIZER_FLAGS    JsonSerializerFlags::Default

#else //UNIX*/

    #define SERIALIZER_FLAGS    JsonSerializerFlags::DateTimeInIsoFormat | JsonSerializerFlags::EnumInStringFormat

//#endif

        //
        // This is defined as a macro instead of a static function to prevent compiler from generating a template specialization
        // for each type.
        //
        #define JsonSerialize(Object, OutputBuffer) JsonHelper::Serialize(Object, OutputBuffer, static_cast<JsonSerializerFlags>(SERIALIZER_FLAGS))

        #define JsonDeserialize(Object, InputBuffer) JsonHelper::Deserialize(Object, InputBuffer, static_cast<JsonSerializerFlags>(SERIALIZER_FLAGS))
    };
}
