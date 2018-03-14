// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ServiceCorrelationDescription::ServiceCorrelationDescription() 
    : ServiceName(),
    Scheme()
{
}

ServiceCorrelationDescription::ServiceCorrelationDescription(ServiceCorrelationDescription const & other)
    : ServiceName(other.ServiceName),
    Scheme(other.Scheme)
{
}

ServiceCorrelationDescription::ServiceCorrelationDescription(ServiceCorrelationDescription && other)
    : ServiceName(move(other.ServiceName)),
    Scheme(move(other.Scheme))
{
}

ServiceCorrelationDescription const & ServiceCorrelationDescription::operator = (ServiceCorrelationDescription const & other)
{
    if (this != &other)
    {
        this->ServiceName = other.ServiceName;
        this->Scheme = other.Scheme;
    }

    return *this;
}

ServiceCorrelationDescription const & ServiceCorrelationDescription::operator = (ServiceCorrelationDescription && other)
{
    if (this != &other)
    {
        this->ServiceName = move(other.ServiceName);
        this->Scheme = move(other.Scheme);
    }

    return *this;
}

bool ServiceCorrelationDescription::operator == (ServiceCorrelationDescription const & other) const
{
    bool equals = true;
    
    equals = StringUtility::AreEqualCaseInsensitive(this->ServiceName, other.ServiceName);
    if (!equals) { return equals; }

    equals = this->Scheme == other.Scheme;
    if (!equals) { return equals; }
   
    return equals;
}

bool ServiceCorrelationDescription::operator != (ServiceCorrelationDescription const & other) const
{
    return !(*this == other);
}

void ServiceCorrelationDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ServiceCorrelationDescription { ");

    w.Write("ServiceName = {0}, ", this->ServiceName);    
    w.Write("Scheme = {0} ", this->Scheme);      

    w.Write("}");
}

void ServiceCorrelationDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    // <ServiceCorrelation ServiceName="" Scheme="" />
    xmlReader->StartElement(
        *SchemaNames::Element_ServiceCorrelation,
        *SchemaNames::Namespace);

    this->ServiceName = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ServiceName);
    this->Scheme = ServiceCorrelationScheme::GetScheme(
        xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Scheme));

    xmlReader->ReadElement();
}

wstring ServiceCorrelationDescription::EnumToString()
{
	if (this->Scheme == ServiceCorrelationScheme::Affinity)
	{
		return L"Affinity";
	}
	else if (this->Scheme == ServiceCorrelationScheme::AlignedAffinity)
	{
		return L"AlignedAffinity";
	}
	else
	{
		return L"NonAlignedAffinity";
	}
}

ErrorCode ServiceCorrelationDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ServiceCorrelation, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_ServiceName, this->ServiceName);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Scheme, this->EnumToString());
	if (!er.IsSuccess())
	{
		return er;
	}
	return xmlWriter->WriteEndElement();
}
void ServiceCorrelationDescription::clear()
{
    this->ServiceName.clear();    
}
