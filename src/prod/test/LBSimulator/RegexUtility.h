// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LBSimulator
{
    class RegexUtility
    {
        DENY_COPY(RegexUtility);
    public:
        static std::wstring const KeyValueDelimiter;

        static std::wstring ExtractStringByKey(std::wstring const & text, std::wstring key);
        static bool ExtractBoolByKey(std::wstring const & text, std::wstring key);
        template <typename T>
        static T ExtractValueByKey(std::wstring const & text, std::wstring key)
        {
            T value;
            wstring stringValue = RegexUtility::ExtractStringByKey(text, key);
            
            if (StringUtility::TryFromWString(stringValue, value))
            {
                return value;
            }
            else
            {
                Assert::CodingError("Value cannot be extracted into a bool from {0} by key {1}", text.c_str(), key.c_str());
            }
        }

        static std::wstring ExtractStringInBetween(std::wstring const & text, std::wstring string1, std::wstring string2, bool gready = false);
        static bool ExtractBoolInBetween(std::wstring const & text, std::wstring string1, std::wstring string2, bool gready = false);
        template <typename T>
        static T ExtractValueInBetween(wstring const & text, wstring string1, wstring string2, bool gready = false)
        {
            T value;
            wstring stringValue = RegexUtility::ExtractStringInBetween(text, string1, string2, stringValue, gready);
            
            if (StringUtility::TryFromWString(stringValue, value))
            {
                return value;
            }
            else
            {
                Assert::CodingError("Value cannot be extracted into a value: {0}", text.c_str());
            }
        }


    };
}
