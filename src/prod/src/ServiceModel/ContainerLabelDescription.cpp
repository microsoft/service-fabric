// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ContainerLabelDescription::ContainerLabelDescription()
	: Name(),
	Value()
{
}

bool ContainerLabelDescription::operator== (ContainerLabelDescription const & other) const
{
    return (StringUtility::AreEqualCaseInsensitive(Name, other.Name) &&
        StringUtility::AreEqualCaseInsensitive(Value, other.Value));
}

bool ContainerLabelDescription::operator!= (ContainerLabelDescription const & other) const
{
    return !(*this == other);
}

void ContainerLabelDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ContainerLabelDescription { ");
    w.Write("Name = {0}, ", this->Name);
    w.Write("Value = {0}, ", this->Value);
 
    w.Write("}");
}

void ContainerLabelDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    xmlReader->StartElement(
        *SchemaNames::Element_Label,
        *SchemaNames::Namespace);

    this->Name = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Name);
    this->Value = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Value);
    // Read the rest of the empty element
    xmlReader->ReadElement();
}

Common::ErrorCode ContainerLabelDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{	//<Label>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_Label, L"", *SchemaNames::Namespace);
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
	//</Label>
	return xmlWriter->WriteEndElement();
}

void ContainerLabelDescription::clear()
{
    this->Name.clear();
    this->Value.clear();
}

ErrorCode ContainerLabelDescription::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_CONTAINER_LABEL_DESCRIPTION & fabricLabelDesc) const
{
    fabricLabelDesc.Name = heap.AddString(this->Name);
    fabricLabelDesc.Value = heap.AddString(this->Value);
    fabricLabelDesc.Reserved = nullptr;

    return ErrorCode::Success();
}