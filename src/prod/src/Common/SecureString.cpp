// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

GlobalWString SecureString::TracePlaceholder = make_global<std::wstring>(L"<SecureString>");

SecureString::SecureString()
    : value_()
{    
}

SecureString::SecureString(wstring const & secureValue)
    : value_(secureValue)
{    
}

SecureString::SecureString(wstring && secureValue)
    : value_(move(secureValue))
{    
}

SecureString::~SecureString()
{
    this->Clear();
}

bool SecureString::operator == (SecureString const & other) const
{    
    return this->value_ == other.value_;    
}

bool SecureString::operator != (SecureString const & other) const
{
    return !(*this == other);
}

SecureString const & SecureString::operator = (SecureString const & other)
{
    if (this != & other)
    {
        this->value_ = other.value_;
    }

    return *this;
}

SecureString const & SecureString::operator = (SecureString && other)
{
    if (this != & other)
    {
        this->value_ = move(other.value_);
    }

    return *this;
}

wstring const & SecureString::GetPlaintext() const
{
    return value_;
}

bool SecureString::IsEmpty() const
{
    return value_.empty();
}

void SecureString::Append(wstring const & value)
{    
    value_.append(value);
}

void SecureString::Append(SecureString const & value)
{    
    value_.append(value.GetPlaintext());
}

void SecureString::TrimTrailing(wstring const & trimChars)
{    
    StringUtility::TrimTrailing<wstring>(value_, trimChars);
}

void SecureString::Replace(std::wstring const & stringToReplace, std::wstring const & stringToInsert)
{    
    StringUtility::Replace<wstring>(value_, stringToReplace, stringToInsert);
}

void SecureString::WriteTo(TextWriter& w, FormatOptions const &) const
{
    w.Write(*TracePlaceholder);
}

void SecureString::Clear() const
{
    SecureZeroMemory((void *) value_.c_str(), value_.size() * sizeof(wchar_t));
}
