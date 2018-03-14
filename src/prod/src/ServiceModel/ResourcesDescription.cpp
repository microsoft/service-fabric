// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ResourcesDescription::ResourcesDescription() 
    : Endpoints()
{
}

ResourcesDescription::ResourcesDescription(
    ResourcesDescription const & other) 
    : Endpoints(other.Endpoints)
{
}

ResourcesDescription::ResourcesDescription(ResourcesDescription && other) 
    : Endpoints(move(other.Endpoints))
{
}

ResourcesDescription const & ResourcesDescription::operator = (ResourcesDescription const & other)
{
    if (this != &other)
    {
        this->Endpoints = other.Endpoints;
    }

    return *this;
}

ResourcesDescription const & ResourcesDescription::operator = (ResourcesDescription && other)
{
    if (this != &other)
    {
        this->Endpoints = move(other.Endpoints);
    }

    return *this;
}

bool ResourcesDescription::operator == (ResourcesDescription const & other) const
{
    bool equals = true;

    equals = this->Endpoints.size() == other.Endpoints.size();
    if (!equals) { return equals; }

    for (size_t ix = 0; ix < this->Endpoints.size(); ++ix)
    {
        auto it = find(other.Endpoints.begin(), other.Endpoints.end(), this->Endpoints[ix]);
        equals = it != other.Endpoints.end();
        if (!equals) { return equals; }
    }

    return equals;

}

bool ResourcesDescription::operator != (ResourcesDescription const & other) const
{
    return !(*this == other);
}

void ResourcesDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ResourcesDescription { ");
    w.Write("Endpoints = {");
    for(auto iter = Endpoints.cbegin(); iter != Endpoints.cend(); ++ iter)
    {
        w.Write("{0},",*iter);
    }
    w.Write("}");

    w.Write("}");
}

void ResourcesDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    xmlReader->StartElement(
        *SchemaNames::Element_Resources,
        *SchemaNames::Namespace,
        false);

    if (xmlReader->IsEmptyElement())
    {
        // <Resources />
        xmlReader->ReadElement();
        return;
    }

    xmlReader->ReadStartElement();

    ParseCertificates(xmlReader);

    ParseEndpoints(xmlReader);

    // </Resources>
    xmlReader->ReadEndElement();
}

ErrorCode ResourcesDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	if (noEndPoints() && noCertificates()) {
		return ErrorCodeValue::Success;
	}
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_Resources, L"",  *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = WriteCertificates(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = WriteEndpoints(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}

	return xmlWriter->WriteEndElement();
}
ErrorCode ResourcesDescription::WriteCertificates(XmlWriterUPtr const &)
{
	return ErrorCodeValue::Success;
}

ErrorCode ResourcesDescription::WriteEndpoints(XmlWriterUPtr const & xmlWriter)
{
	if (noEndPoints())
	{
		return ErrorCodeValue::Success;
	}
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_Endpoints, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	for (vector<EndpointDescription>::iterator it = Endpoints.begin(); it != Endpoints.end(); ++it)
	{
		er = (*it).WriteToXml(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}
	}

	return xmlWriter->WriteEndElement();
}

bool ResourcesDescription::noEndPoints()
{
	return this->Endpoints.empty();
}

bool ResourcesDescription::noCertificates()
{
	return true;
}



void ResourcesDescription::ParseCertificates(XmlReaderUPtr const & xmlReader)
{
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_Certificates,
        *SchemaNames::Namespace,
        false))
    {
        // Read placeholder
        xmlReader->SkipElement(
            *SchemaNames::Element_Certificates, 
            *SchemaNames::Namespace, 
            true);
    }
}

void ResourcesDescription::ParseEndpoints(XmlReaderUPtr const & xmlReader)
{
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_Endpoints,
        *SchemaNames::Namespace,
        false))
    {
        xmlReader->StartElement(
            *SchemaNames::Element_Endpoints,
            *SchemaNames::Namespace);

        xmlReader->ReadStartElement();

        bool done = false;
        while(!done)
        {
            done = true;
            if (xmlReader->IsStartElement(
                *SchemaNames::Element_Endpoint,
                *SchemaNames::Namespace,
                false))
            {
                EndpointDescription endpoint;
                endpoint.ReadFromXml(xmlReader);
                this->Endpoints.push_back(move(endpoint));
                done = false;
            }
        }

        // </Endpoints>
        xmlReader->ReadEndElement();
    }
}

void ResourcesDescription::clear()
{
    this->Endpoints.clear();
}
