// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

namespace Naming
{
    INITIALIZE_SIZE_ESTIMATION(EnumeratePropertiesToken)

    const std::wstring EnumeratePropertiesToken::Delimiter = L"$";

    EnumeratePropertiesToken::EnumeratePropertiesToken()
        : lastEnumeratedPropertyName_()
        , propertiesVersion_(-1)
        , isValid_(false)
    {
    }

    EnumeratePropertiesToken::EnumeratePropertiesToken(
        std::wstring const & lastEnumeratedPropertyName,
        _int64 propertiesVersion)
        : lastEnumeratedPropertyName_(lastEnumeratedPropertyName)
        , propertiesVersion_(propertiesVersion)
        , isValid_(true)
    {
    }

    ErrorCode EnumeratePropertiesToken::Create(std::wstring const & escapedToken, __out EnumeratePropertiesToken & token)
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
        
        _int64 propertiesVersion;
        bool parseSuccess = StringUtility::TryFromWString<_int64>(tokenElements[1], propertiesVersion);
        if (!parseSuccess)
        {
            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                wformatString(GET_COMMON_RC(Invalid_Continuation_Token), escapedToken));
        }

        std::wstring lastEnumeratedPropertyName = tokenElements[0];
        token = EnumeratePropertiesToken(lastEnumeratedPropertyName, propertiesVersion);
        return ErrorCode::Success();
    }

    ErrorCode EnumeratePropertiesToken::ToEscapedString(__out std::wstring & escapedToken) const
    {
        std::wstring token;
        token += lastEnumeratedPropertyName_;
        token += Delimiter;
        token += StringUtility::ToWString(propertiesVersion_);

        return NamingUri::EscapeString(token, escapedToken);
    }

    void EnumeratePropertiesToken::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
    {
        if (isValid_)
        {
            w << "Token[LastProperty: " << lastEnumeratedPropertyName_<< " (" << propertiesVersion_ << ")]";
        }
        else
        {
            w << "Token[Invalid]";
        }
    }
}
