// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

ConfigParameter::ConfigParameter()
    : Name(),
    Value(),
    MustOverride(false),
    IsEncrypted(false),
    Type(L"")
{
}

ConfigParameter::ConfigParameter(
    std::wstring && name,
    std::wstring && value,
    bool mustOverride,
    bool isEncrypted,
    std::wstring && type) :
    Name(move(name)),
    Value(move(value)),
    MustOverride(mustOverride),
    IsEncrypted(isEncrypted),
    Type(move(type))
{
}
      
ConfigParameter::ConfigParameter(ConfigParameter const & other)
    : Name(other.Name),
    Value(other.Value),
    MustOverride(other.MustOverride),
    IsEncrypted(other.IsEncrypted),
    Type(other.Type)
{
}

ConfigParameter::ConfigParameter(ConfigParameter && other)
    : Name(move(other.Name)),
    Value(move(other.Value)),
    MustOverride(other.MustOverride),
    IsEncrypted(other.IsEncrypted),
    Type(move(other.Type))
{
}

bool ConfigParameter::operator == (ConfigParameter const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->Name, other.Name);
    if (!equals) { return equals; }

    equals = this->Value == other.Value;
    if (!equals) { return equals; }

    equals = this->MustOverride == other.MustOverride;
    if (!equals) { return equals; }

    equals = this->IsEncrypted == other.IsEncrypted;
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->Type, other.Type);
    if (!equals) { return equals; }

    return equals;
}

bool ConfigParameter::operator != (ConfigParameter const & other) const
{
    return !(*this == other);
}

void ConfigParameter::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ConfigParameter {");
    w.Write("Name = {0},", this->Name);
    w.Write("Value = {0},", this->Value);
    w.Write("MustOverride = {0}", this->MustOverride);
    w.Write("IsEncrypted = {0}", this->IsEncrypted);
    w.Write("Type = {0}", this->Type);
    w.Write("}");
}

void ConfigParameter::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_CONFIGURATION_PARAMETER & publicParameter) const
{
    publicParameter.Name = heap.AddString(this->Name);
    publicParameter.Value = heap.AddString(this->Value);    
    publicParameter.MustOverride = this->MustOverride;
    publicParameter.IsEncrypted = this->IsEncrypted;

    auto ex1 = heap.AddItem<FABRIC_CONFIGURATION_PARAMETER_EX1>();
    ex1->Type = heap.AddString(this->Type);

    publicParameter.Reserved = ex1.GetRawPointer();
}

void ConfigParameter::clear()
{
    this->Name.clear();
    this->Value.clear();
    this->MustOverride = false;
    this->IsEncrypted = false;
    this->Type.clear();
}
