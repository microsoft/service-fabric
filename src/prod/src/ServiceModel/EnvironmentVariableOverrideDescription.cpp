// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

EnvironmentVariableOverrideDescription::EnvironmentVariableOverrideDescription()
    : Name(),
    Value(),
    Type()
{
}

EnvironmentVariableOverrideDescription::EnvironmentVariableOverrideDescription(EnvironmentVariableOverrideDescription const & other)
: Name(other.Name),
Value(other.Value),
Type(other.Type)
{
}

EnvironmentVariableOverrideDescription::EnvironmentVariableOverrideDescription(EnvironmentVariableOverrideDescription && other)
: Name(move(other.Name)),
Value(move(other.Value)),
Type(move(other.Type))
{
}

bool EnvironmentVariableOverrideDescription::operator== (EnvironmentVariableOverrideDescription const & other) const
{
    bool equals = (StringUtility::AreEqualCaseInsensitive(Name, other.Name) &&
        StringUtility::AreEqualCaseInsensitive(Value, other.Value));

    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->Type, other.Type);

    return equals;

}

bool EnvironmentVariableOverrideDescription::operator!= (EnvironmentVariableOverrideDescription const & other) const
{
    return !(*this == other);
}

void EnvironmentVariableOverrideDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("EnvironmentVariableOverrideDescription { ");
    w.Write("Name = {0}, ", Name);
    w.Write("Value = {0}, ", Value);
    w.Write("Type = {0}, ", Type);
    w.Write("}");
}

void EnvironmentVariableOverrideDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    xmlReader->StartElement(
        *SchemaNames::Element_EnvironmentVariable,
        *SchemaNames::Namespace);

    this->Name = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Name);
    this->Value = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Value);

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_Type))
    {
        this->Type = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Type);
    }

    // Read the rest of the empty element
    xmlReader->ReadElement();
}

Common::ErrorCode EnvironmentVariableOverrideDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{	//<EnvironmentVariable>
    ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_EnvironmentVariable, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_Name, this->Name);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Value, this->Value);
    if (!er.IsSuccess())
    {
        return er;
    }

    if (!this->Type.empty())
    {
        er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Type, this->Type);
        if (!er.IsSuccess())
        {
            return er;
        }

    }

    //</EnvironmentVariable>
    return xmlWriter->WriteEndElement();
}

void EnvironmentVariableOverrideDescription::clear()
{
    this->Name.clear();
    this->Value.clear();
    this->Type.clear();
}
