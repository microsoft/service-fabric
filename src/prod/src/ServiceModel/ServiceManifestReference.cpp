// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ServiceManifestReference::ServiceManifestReference() 
    : Name(),
    Version()
{
}

ServiceManifestReference::ServiceManifestReference(ServiceManifestReference const & other)
    : Name(other.Name),
    Version(other.Version)
{
}

ServiceManifestReference::ServiceManifestReference(ServiceManifestReference && other)
    : Name(move(other.Name)),
    Version(move(other.Version))
{
}

ServiceManifestReference const & ServiceManifestReference::operator = (ServiceManifestReference const & other)
{
    if (this != &other)
    {
        this->Name = other.Name;
        this->Version = other.Version;
    }

    return *this;
}

ServiceManifestReference const & ServiceManifestReference::operator = (ServiceManifestReference && other)
{
    if (this != &other)
    {
        this->Name = move(other.Name);
        this->Version = move(other.Version);
    }

    return *this;
}

bool ServiceManifestReference::operator == (ServiceManifestReference const & other) const
{
    bool equals = true;

    equals = this->Name == other.Name;
    if (!equals) { return equals; }

    equals = this->Version == other.Version;
    if (!equals) { return equals; }

    return equals;
}

bool ServiceManifestReference::operator != (ServiceManifestReference const & other) const
{
    return !(*this == other);
}

void ServiceManifestReference::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ServiceManifestReference { ");
    w.Write("Name = {0}, ", Name);
    w.Write("Version = {0}", Version);    
    w.Write("}");
}

void ServiceManifestReference::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    // <ServiceManifestRef>
    xmlReader->StartElement(
        *SchemaNames::Element_ServiceManifestRef,
        *SchemaNames::Namespace);

    this->Name = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ServiceManifestName);

    this->Version = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ServiceManifestVersion);

    xmlReader->ReadElement();
}


Common::ErrorCode ServiceManifestReference::WriteToXml(XmlWriterUPtr const & xmlWriter)
{	//<ServiceManifestRef>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ServiceManifestRef, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	xmlWriter->WriteAttribute(*SchemaNames::Attribute_ServiceManifestName, this->Name);
	if (!er.IsSuccess())
	{
		return er;
	}
	xmlWriter->WriteAttribute(*SchemaNames::Attribute_ServiceManifestVersion, this->Version);
	if (!er.IsSuccess())
	{
		return er;
	}

	//</ServiceManifestRef>
	return xmlWriter->WriteEndElement();

}

void ServiceManifestReference::clear()
{
    this->Name.clear();    
    this->Version.clear();
}

