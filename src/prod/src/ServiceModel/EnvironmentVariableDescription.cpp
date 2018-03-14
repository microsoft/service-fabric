// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

EnvironmentVariableDescription::EnvironmentVariableDescription()
	: Name(),
	Value()
{
}

EnvironmentVariableDescription::EnvironmentVariableDescription(EnvironmentVariableDescription const & other)
: Name(other.Name),
Value(other.Value)
{
}

EnvironmentVariableDescription::EnvironmentVariableDescription(EnvironmentVariableDescription && other)
: Name(other.Name),
Value(move(other.Value))
{
}

EnvironmentVariableDescription const & EnvironmentVariableDescription::operator=(EnvironmentVariableDescription const & other)
{
    if (this != &other)
    {
        this->Name = other.Name;
        this->Value = other.Value;
    }
    return *this;
}

EnvironmentVariableDescription const & EnvironmentVariableDescription::operator=(EnvironmentVariableDescription && other)
{
    if (this != &other)
    {
        this->Name = other.Name;
        this->Value = move(other.Value);
    }
    return *this;
}

bool EnvironmentVariableDescription::operator== (EnvironmentVariableDescription const & other) const
{
    return (StringUtility::AreEqualCaseInsensitive(Name, other.Name) &&
        StringUtility::AreEqualCaseInsensitive(Value, other.Value));
}

bool EnvironmentVariableDescription::operator!= (EnvironmentVariableDescription const & other) const
{
    return !(*this == other);
}

void EnvironmentVariableDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("EnvironmentVariableDescription { ");
    w.Write("Name = {0}, ", Name);
    w.Write("Value = {0}, ", Value);
 
    w.Write("}");
}

void EnvironmentVariableDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    xmlReader->StartElement(
        *SchemaNames::Element_EnvironmentVariable,
        *SchemaNames::Namespace);

    this->Name = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Name);
    this->Value = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Value);
    // Read the rest of the empty element
    xmlReader->ReadElement();
}

Common::ErrorCode EnvironmentVariableDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
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
	//</EnvironmentVariable>
	return xmlWriter->WriteEndElement();
}

void EnvironmentVariableDescription::clear()
{
    this->Name.clear();
    this->Value.clear();
}
