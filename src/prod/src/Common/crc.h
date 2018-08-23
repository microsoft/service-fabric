// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
// https://msdn.microsoft.com/en-us/library/dd905031.aspx

#pragma once

namespace Common
{
    class crc32
    {
    public:
        crc32() = default;

        crc32(void const* buf, size_t len)
        {
            AddData(buf, len);
        }

        inline void AddData(void const* buf, size_t len)
        {
            auto cursor = (const uint8_t *)buf;
            while (len--)
            {
                value_ = table_[(value_ ^ *(cursor++)) & 0xFF] ^ (value_ >> 8);
            }
        }

        inline auto Value() const { return value_ ^ ~0U; }

        uint32_t value_ = ~0U;

        static const uint32_t table_[256];
    };

    class crc8
    {
    public:
        crc8() = default;

        crc8(void const* buf, size_t len)
        {
            AddData(buf, len);
        }

        inline void AddData(void const* buf, size_t len)
        {
            auto cursor = (const uint8_t *)buf;
            while (len--)
            {
                value_ = table_[value_ ^ *(cursor++)];
            }
        }

        inline auto Value() const { return value_; }

        unsigned char value_ = 0;

        static const unsigned char table_[256];
    };
}
