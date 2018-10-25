// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

ConfigParameterOverride::ConfigParameterOverride()
    : Name(),
    Value(),
    IsEncrypted(false),
    Type(L"")
{
}

ConfigParameterOverride::ConfigParameterOverride(
    std::wstring && name,
    std::wstring && value,
    bool isEncrypted,
    std::wstring && type) :
    Name(move(name)),
    Value(move(value)),
    IsEncrypted(isEncrypted),
    Type(move(type))
{
}

ConfigParameterOverride::ConfigParameterOverride(ConfigParameterOverride const & other)
    : Name(other.Name),
    Value(other.Value),
    IsEncrypted(other.IsEncrypted),
    Type(other.Type)
{
}

ConfigParameterOverride::ConfigParameterOverride(ConfigParameterOverride && other)
    : Name(move(other.Name)),
    Value(move(other.Value)),
    IsEncrypted(other.IsEncrypted),
    Type(move(other.Type))
{
}

bool ConfigParameterOverride::operator == (ConfigParameterOverride const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->Name, other.Name);
    if (!equals) { return equals; }

    equals = this->Value == other.Value;
    if (!equals) { return equals; }

    equals = this->IsEncrypted == other.IsEncrypted;
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->Type, other.Type);
    if (!equals) { return equals; }

    return equals;
}

bool ConfigParameterOverride::operator != (ConfigParameterOverride const & other) const
{
    return !(*this == other);
}

void ConfigParameterOverride::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ConfigParameterOverride {");
    w.Write("Name = {0},", this->Name);
    w.Write("Value = {0},", this->Value);
    w.Write("IsEncrypted = {0},", this->IsEncrypted);
    w.Write("IsType = {0}", this->Type);
    w.Write("}");
}
