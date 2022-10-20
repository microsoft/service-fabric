// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ConfigParameterOverride::ConfigParameterOverride()
    : Name(),
    Value(),
    IsEncrypted(false),
    Type(L"")
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

void ConfigParameterOverride::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    // ensure that we are positioned on ConfigParameter
    xmlReader->StartElement(
        *SchemaNames::Element_ConfigParameter, 
        *SchemaNames::Namespace);

    this->Name = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Name);
    this->Value = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Value);       
    
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_IsEncrypted))
    {
        this->IsEncrypted = xmlReader->ReadAttributeValueAs<bool>(*SchemaNames::Attribute_IsEncrypted);
    }

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_Type))
    {
        this->Type = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Type);
    }

    xmlReader->ReadElement();
}

ErrorCode ConfigParameterOverride::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
    //Paramter
    ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ConfigParameter, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Name, this->Name);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Value, this->Value);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteBooleanAttribute(*SchemaNames::Attribute_IsEncrypted, this->IsEncrypted);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Type, this->Type);
    if (!er.IsSuccess())
    {
        return er;
    }
    //</Paramter>
    return xmlWriter->WriteEndElement();

}
