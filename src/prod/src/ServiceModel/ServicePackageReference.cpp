// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ServicePackageReference::ServicePackageReference() 
    : Name(),
    RolloutVersionValue()
{
}

ServicePackageReference::ServicePackageReference(ServicePackageReference const & other)
    : Name(other.Name),
    RolloutVersionValue(other.RolloutVersionValue)
{
}

ServicePackageReference::ServicePackageReference(ServicePackageReference && other)
    : Name(move(other.Name)),
    RolloutVersionValue(move(other.RolloutVersionValue))
{
}

ServicePackageReference const & ServicePackageReference::operator = (ServicePackageReference const & other)
{
    if (this != &other)
    {
        this->Name = other.Name;
        this->RolloutVersionValue = other.RolloutVersionValue;
    }

    return *this;
}

ServicePackageReference const & ServicePackageReference::operator = (ServicePackageReference && other)
{
    if (this != &other)
    {
        this->Name = move(other.Name);
        this->RolloutVersionValue = move(other.RolloutVersionValue);
    }

    return *this;
}

bool ServicePackageReference::operator == (ServicePackageReference const & other) const
{
    bool equals = true;

    equals = this->Name == other.Name;
    if (!equals) { return equals; }

    equals = this->RolloutVersionValue == other.RolloutVersionValue;
    if (!equals) { return equals; }

    return equals;
}

bool ServicePackageReference::operator != (ServicePackageReference const & other) const
{
    return !(*this == other);
}

void ServicePackageReference::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ServicePackageReference { ");
    w.Write("Name = {0}, ", Name);
    w.Write("RolloutVersionValue = {0}", RolloutVersionValue);    
    w.Write("}");
}

void ServicePackageReference::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    // <ServicePackageRef>
    xmlReader->StartElement(
        *SchemaNames::Element_ServicePackageRef,
        *SchemaNames::Namespace);

    this->Name = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Name);

    wstring rolloutVersionString = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_RolloutVersion);
    if(!RolloutVersion::FromString(rolloutVersionString, this->RolloutVersionValue).IsSuccess())
    {
        Assert::CodingError("The format of RolloutVersion is invalid: {0}", rolloutVersionString);
    }

    xmlReader->ReadElement();
}

Common::ErrorCode ServicePackageReference::WriteToXml(XmlWriterUPtr const & xmlWriter)
{	//<ServicePackageREf>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ServicePackageRef, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Name, this->Name);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_RolloutVersion, this->RolloutVersionValue.ToString());
	if (!er.IsSuccess())
	{
		return er;
	}
	//</ServicePackageREf>
	return xmlWriter->WriteEndElement();
}
void ServicePackageReference::clear()
{
    this->Name.clear();    
}
