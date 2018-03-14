// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace std;
    using namespace Common;

    const std::wstring EnumerateSubNamesToken::Delimiter = L"$";

    EnumerateSubNamesToken::EnumerateSubNamesToken()
        : lastEnumeratedName_()
        , subnamesVersion_(-1)
        , isValid_(false)
    {
    }

    EnumerateSubNamesToken::EnumerateSubNamesToken(
        NamingUri const & lastEnumeratedName,
        _int64 subnamesVersion)
        : lastEnumeratedName_(lastEnumeratedName)
        , subnamesVersion_(subnamesVersion)
        , isValid_(true)
    {
    }

    ErrorCode EnumerateSubNamesToken::Create(std::wstring const & escapedToken, __out EnumerateSubNamesToken & token)
    {
        std::wstring unescapedToken;
        auto error = NamingUri::UnescapeString(escapedToken, unescapedToken);
        if (!error.IsSuccess())
        {
            return error;
        }

        StringCollection tokenElements;
        StringUtility::Split<std::wstring>(unescapedToken, tokenElements, Delimiter);
        if (tokenElements.size() != 2)
        {
            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                wformatString(GET_COMMON_RC(Invalid_Continuation_Token), escapedToken));
        }

        _int64 subNamesVersion;
        bool parseSuccess = StringUtility::TryFromWString<_int64>(tokenElements[1], subNamesVersion);
        if (!parseSuccess)
        {
            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                wformatString(GET_COMMON_RC(Invalid_Continuation_Token), escapedToken));
        }

        NamingUri lastEnumeratedName;
        parseSuccess = NamingUri::TryParse(tokenElements[0], lastEnumeratedName);
        if (!parseSuccess)
        {
            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                wformatString(GET_COMMON_RC(Invalid_Continuation_Token), escapedToken));
        }

        token = EnumerateSubNamesToken(lastEnumeratedName, subNamesVersion);
        return ErrorCode::Success();
    }

    ErrorCode EnumerateSubNamesToken::ToEscapedString(__out std::wstring & escapedToken) const
    {
        std::wstring token;
        token += lastEnumeratedName_.ToString();
        token += Delimiter;
        token += StringUtility::ToWString(subnamesVersion_);

        return NamingUri::EscapeString(token, escapedToken);
    }

    void EnumerateSubNamesToken::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
    {
        if (isValid_)
        {
            w << "Token[LastName: " << lastEnumeratedName_<< "(" << subnamesVersion_ << ")]";
        }
        else
        {
            w << "Token[Invalid]";
        }
    }
}
