// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

DescriptionExtension::DescriptionExtension() :
    Name(),
    Value()
{
}

DescriptionExtension::DescriptionExtension(wstring name, wstring value) :
    Name(name),
    Value(value)
{
}

DescriptionExtension::DescriptionExtension(DescriptionExtension const & other) :
    Name(other.Name),
    Value(other.Value)
{
}

DescriptionExtension::DescriptionExtension(DescriptionExtension && other) :
    Name(move(other.Name)),
    Value(move(other.Value))
{
}

DescriptionExtension const & DescriptionExtension::operator = (DescriptionExtension const & other)
{
    if (this != & other)
    {
        this->Name = other.Name;
        this->Value = other.Value;
    }

    return *this;
}

DescriptionExtension const & DescriptionExtension::operator = (DescriptionExtension && other)
{
    if (this != & other)
    {
        this->Name = move(other.Name);
        this->Value = move(other.Value);
    }

    return *this;
}

bool DescriptionExtension::operator == (DescriptionExtension const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->Name, other.Name);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->Value, other.Value);
    if (!equals) { return equals; }

    return equals;
}

bool DescriptionExtension::operator != (DescriptionExtension const & other) const
{
    return !(*this == other);
}

void DescriptionExtension::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("DescriptionExtension { ");
    w.Write("Name = {0}, ", Name);
    w.Write("Value = {0}", Value);
    w.Write("}");
}

void DescriptionExtension::ReadFromXml(Common::XmlReaderUPtr const & xmlReader)   
{
    this->clear();
    // ensure that we are on <Extension
    xmlReader->StartElement(
        *SchemaNames::Element_Extension,
        *SchemaNames::Namespace);

    this->Name = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Name);

    xmlReader->ReadStartElement();
    XmlNodeType nodeType = xmlReader->MoveToContent();
    ASSERT_IF(nodeType != ::XmlNodeType_Element, "Unexpected node type under <Extension>");
    this->Value = xmlReader->ReadOuterXml();
    xmlReader->ReadEndElement();
}

ErrorCode DescriptionExtension::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	//Extension
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_Extension, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Name, this->Name);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteElementWithContent(*SchemaNames::Attribute_Value, this->Value, L"", *SchemaNames::Namespace);
	//</Extension>
	return xmlWriter->WriteEndElement();
}

void DescriptionExtension::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_SERVICE_TYPE_DESCRIPTION_EXTENSION & publicDescription) const
{
    publicDescription.Name = heap.AddString(this->Name);
    publicDescription.Value = heap.AddString(this->Value);
}

ErrorCode DescriptionExtension::FromPublicApi(__in FABRIC_SERVICE_TYPE_DESCRIPTION_EXTENSION const & publicDescription)
{
    this->Name = publicDescription.Name;
    this->Value = publicDescription.Value;

    return ErrorCode(ErrorCodeValue::Success);
}

void DescriptionExtension::clear()
{
    this->Name.clear();
    this->Value.clear();
}

wstring DescriptionExtension::ToString() const
{
    return wformatString("Name = '{0}'; Value = '{1}'", Name, Value);    
}
