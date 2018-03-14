// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ParameterDescription::ParameterDescription()
    : Name(),
    Value(),
    IsEncrypted()
{
}

ParameterDescription::ParameterDescription(
    ParameterDescription const & other)
    : Name(other.Name),
    Value(other.Value),
    IsEncrypted(other.IsEncrypted)
{
}

ParameterDescription::ParameterDescription(ParameterDescription && other) 
    : Name(move(other.Name)),
    Value(move(other.Value)),
    IsEncrypted(move(other.IsEncrypted))
{
}

ParameterDescription const & ParameterDescription::operator = (ParameterDescription const & other)
{
    if (this != &other)
    {
        this->Name = other.Name;
        this->Value = other.Value;
        this->IsEncrypted = other.IsEncrypted;
    }
    return *this;
}

ParameterDescription const & ParameterDescription::operator = (ParameterDescription && other)
{
    if (this != &other)
    {
        this->Name = move(other.Name);
        this->Value = move(other.Value);
        this->IsEncrypted = move(other.IsEncrypted);
    }
    return *this;
}

bool ParameterDescription::operator == (ParameterDescription const & other) const
{
    bool equals = true;

    equals = this->Name == other.Name;
    if (!equals) { return equals; }

    equals = this->Value == other.Value;
    if (!equals) { return equals; }

    equals = this->IsEncrypted == other.IsEncrypted;
    if (!equals) { return equals; }

    return equals;
}

bool ParameterDescription::operator != (ParameterDescription const & other) const
{
    return !(*this == other);
}

void ParameterDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ParameterDescription { ");
    w.Write("Name = {0}", this->Name);
    w.Write("Value = {0}", this->Value);
    w.Write("IsEncrypted = {0}", this->IsEncrypted);
    w.Write("}");
}

void ParameterDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    // <Parameter Name="" Value="" IsEncrypted="">
    xmlReader->StartElement(
        *SchemaNames::Element_Parameter,
        *SchemaNames::Namespace,
        false);

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_Name))
    {
        this->Name = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Name);
    }

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_Value))
    {
        this->Value = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Value);
    }

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_IsEncrypted))
    {
        this->IsEncrypted = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_IsEncrypted);
    }

    // Read the rest of the empty Parameter element 
    xmlReader->ReadElement();
}


ErrorCode ParameterDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	//<Parameter>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_Parameter, L"", *SchemaNames::Namespace);
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
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_IsEncrypted, this->IsEncrypted);
	if (!er.IsSuccess())
	{
		return er;
	}
	//</Paramter>
	return xmlWriter->WriteEndElement();
}

void ParameterDescription::clear()
{
    this->Name.clear();
    this->Value.clear();
    this->IsEncrypted.clear();
}
