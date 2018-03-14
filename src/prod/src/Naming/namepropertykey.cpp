// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace std;
    using namespace Common;

    NamePropertyKey::NamePropertyKey(NamingUri const & uri, std::wstring const & propertyName)
        : uri_(uri)
        , propertyName_(propertyName)
        , key_(CreateKey(uri_, propertyName_))
    {
    }

    bool NamePropertyKey::operator == (NamePropertyKey const & other) const
    {
        return key_ == other.key_;
    }

    bool NamePropertyKey::operator < (NamePropertyKey const & other) const
    {
        return (key_ < other.key_);
    }

    wstring NamePropertyKey::CreateKey(NamingUri const & uri, std::wstring const & propertyName)
    {
        wstring result;

        result = uri.Name;
        result.append(NamingUri::NamingUriConcatenationReservedTokenDelimiter);
        result.append(propertyName);

        return move(result);
    }

    void NamePropertyKey::WriteTo(TextWriter & writer, FormatOptions const &) const
    {
        writer.Write("{0}", key_);
    }
}
