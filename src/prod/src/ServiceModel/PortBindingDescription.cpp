// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

PortBindingDescription::PortBindingDescription()
	: ContainerPort(0),
	EndpointResourceRef()
{
}

bool PortBindingDescription::operator== (PortBindingDescription const & other) const
{
    return (StringUtility::AreEqualCaseInsensitive(EndpointResourceRef, other.EndpointResourceRef) &&
        ContainerPort == other.ContainerPort);
}

bool PortBindingDescription::operator!= (PortBindingDescription const & other) const
{
    return !(*this == other);
}

void PortBindingDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("PortBindingDescription { ");
    w.Write("ContainerPort = {0}, ", ContainerPort);
    w.Write("EndpointResourceRef = {0}, ", EndpointResourceRef);
 
    w.Write("}");
}

void PortBindingDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    xmlReader->StartElement(
        *SchemaNames::Element_PortBinding,
        *SchemaNames::Namespace);

    this->ContainerPort = xmlReader->ReadAttributeValueAs<ULONG>(*SchemaNames::Attribute_ContainerPort);
    this->EndpointResourceRef = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_EndpointRef);
    // Read the rest of the empty element
    xmlReader->ReadElement();
}

Common::ErrorCode PortBindingDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{	//<PortBinding>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_PortBinding, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
    er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_ContainerPort, this->ContainerPort);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_EndpointRef, this->EndpointResourceRef);
	if (!er.IsSuccess())
	{
		return er;
	}
	//</PortBinding>
	return xmlWriter->WriteEndElement();
}

void PortBindingDescription::clear()
{
    this->EndpointResourceRef.clear();
    this->ContainerPort = 0;
}
