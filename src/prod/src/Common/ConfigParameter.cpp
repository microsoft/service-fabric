// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ConfigParameter::ConfigParameter()
    : Name(),
    Value(),
    MustOverride(false),
    IsEncrypted(false)
{
}

ConfigParameter::ConfigParameter(ConfigParameter const & other)
    : Name(other.Name),
    Value(other.Value),
    MustOverride(other.MustOverride),
    IsEncrypted(other.IsEncrypted)
{
}

ConfigParameter::ConfigParameter(ConfigParameter && other)
    : Name(move(other.Name)),
    Value(move(other.Value)),
    MustOverride(other.MustOverride),
    IsEncrypted(other.IsEncrypted)
{
}

ConfigParameter const & ConfigParameter::operator = (ConfigParameter const & other)
{
    if (this != &other)
    {
        this->Name = other.Name;
        this->Value = other.Value;
        this->MustOverride = other.MustOverride;
        this->IsEncrypted = other.IsEncrypted;
    }

    return *this;
}

ConfigParameter const & ConfigParameter::operator = (ConfigParameter && other)
{
    if (this != &other)
    {
        this->Name = move(other.Name);
        this->Value = move(other.Value);
        this->MustOverride = other.MustOverride;
        this->IsEncrypted = other.IsEncrypted;
    }

    return *this;
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
    w.Write("}");
}

void ConfigParameter::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    // ensure that we are positioned on ConfigParameter
    xmlReader->StartElement(
        *SchemaNames::Element_ConfigParameter, 
        *SchemaNames::Namespace);

    this->Name = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Name);
    this->Value = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Value);
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_MustOverride))
    {
        this->MustOverride = xmlReader->ReadAttributeValueAs<bool>(*SchemaNames::Attribute_MustOverride);
    }

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_IsEncrypted))
    {
        this->IsEncrypted = xmlReader->ReadAttributeValueAs<bool>(*SchemaNames::Attribute_IsEncrypted);
    }

    xmlReader->ReadElement();
}

void ConfigParameter::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_CONFIGURATION_PARAMETER & publicParameter) const
{
    publicParameter.Name = heap.AddString(this->Name);
    publicParameter.Value = heap.AddString(this->Value);    
    publicParameter.MustOverride = this->MustOverride;
    publicParameter.IsEncrypted = this->IsEncrypted;
}

void ConfigParameter::clear()
{
    this->Name.clear();
    this->Value.clear();
    this->MustOverride = false;
	this->IsEncrypted = false;
}
