// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

X509Identity::X509Identity()
{
}

X509Identity::~X509Identity()
{
}

bool X509Identity::operator != (X509Identity const & rhs) const
{
    return !(*this == rhs);
}

void X509IdentitySet::Add(X509Identity::SPtr && x509Identity)
{
    Invariant(x509Identity);
    set_.emplace(move(x509Identity));
}

void X509IdentitySet::Add(X509Identity::SPtr const & x509Identity)
{
    Invariant(x509Identity);
    set_.emplace(x509Identity);
}

void X509IdentitySet::Add(X509IdentitySet const & other)
{
    set_.insert(other.set_.cbegin(), other.set_.cend());
}

ErrorCode X509IdentitySet::AddThumbprint(wstring const & thumbprintString)
{
    Thumbprint::SPtr thumbprint;
    auto error = Thumbprint::Create(thumbprintString, thumbprint);
    if (error.IsSuccess())
    {
        Add(move(thumbprint));
    }

    return error;
}

void X509IdentitySet::AddPublicKey(PCCertContext certContext)
{
    Add(make_shared<X509PubKey>(certContext));
}

size_t X509IdentitySet::Size() const
{
    return set_.size();
}

bool X509IdentitySet::IsEmpty() const
{
    return set_.empty();
}

ErrorCode X509IdentitySet::SetToThumbprints(std::wstring const & thumbprints)
{
    set_.clear();

    auto thumbprintsCopy(thumbprints);
    StringUtility::TrimWhitespaces(thumbprintsCopy);
    if (thumbprintsCopy.empty())
    {
        return ErrorCode();
    }

    vector<wstring> thumbprintList;
    StringUtility::Split<wstring>(thumbprintsCopy, thumbprintList, L",", true);
    if (thumbprintList.empty())
    {
        return ErrorCodeValue::InvalidArgument;
    }

    for (auto & thumbprintString : thumbprintList)
    {
        StringUtility::TrimWhitespaces(thumbprintString);
        ErrorCode error = AddThumbprint(thumbprintString);
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    return ErrorCode();
}

bool X509IdentitySet::Contains(X509Identity::SPtr const & x509Identity) const
{
    return x509Identity && (set_.find(x509Identity) != set_.cend());
}

bool X509IdentitySet::Contains(X509IdentitySet const & rhs) const
{
    if (set_.size() < rhs.set_.size()) return false;

    for (auto const & element : rhs.set_)
    {
        if (!Contains(element)) return false;
    }

    return true;
}

bool X509IdentitySet::operator==(X509IdentitySet const & rhs) const
{
    if (set_.size() != rhs.set_.size()) return false;

    return Contains(rhs) && rhs.Contains(*this);
}

bool X509IdentitySet::operator!=(X509IdentitySet const & rhs) const
{
    return !(*this == rhs);
}

void X509IdentitySet::WriteTo(TextWriter & w, FormatOptions const &) const
{
    bool outputGenerated = false;
    for (auto const & id : set_)
    {
        if (outputGenerated)
        {
            w.Write(",{0}", id);
        }
        else
        {
            w.Write("{0}", id);
            outputGenerated = true;
        }
    }
}

wstring X509IdentitySet::ToString() const
{
    wstring result;
    StringWriter(result).Write(*this);
    return result;
}

vector<wstring> X509IdentitySet::ToStrings() const
{
    vector<wstring> result;
    for (auto const & id : set_)
    {
        // Only thumbprint is supported for now, as this is used by SecuritySettings::ToPublicApi. Ignore otherwise.
        if (id->IdType() == X509Identity::Thumbprint)
        {
            result.emplace_back(wformatString(*id));
        }      
    }

    return result;
}
