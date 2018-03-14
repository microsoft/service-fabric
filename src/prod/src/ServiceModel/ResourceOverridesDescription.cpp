// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ResourceOverridesDescription::ResourceOverridesDescription() 
    : Endpoints()
{
}

ResourceOverridesDescription::ResourceOverridesDescription(
    ResourceOverridesDescription const & other) 
    : Endpoints(other.Endpoints)
{
}

ResourceOverridesDescription::ResourceOverridesDescription(ResourceOverridesDescription && other) 
    : Endpoints(move(other.Endpoints))
{
}

ResourceOverridesDescription & ResourceOverridesDescription::operator = (ResourceOverridesDescription const & other)
{
    if (this != &other)
    {
        this->Endpoints = other.Endpoints;
    }

    return *this;
}

ResourceOverridesDescription & ResourceOverridesDescription::operator = (ResourceOverridesDescription && other)
{
    if (this != &other)
    {
        this->Endpoints = move(other.Endpoints);
    }

    return *this;
}

bool ResourceOverridesDescription::operator == (ResourceOverridesDescription const & other) const
{
    bool equals = true;

    equals = this->Endpoints.size() == other.Endpoints.size();
    if (!equals) { return equals; }

    for (auto const& endpoint: Endpoints)
    {
        auto it = find(other.Endpoints.begin(), other.Endpoints.end(), endpoint);
        equals = it != other.Endpoints.end();
        if (!equals) { return equals; }
    }

    return equals;

}

bool ResourceOverridesDescription::operator != (ResourceOverridesDescription const & other) const
{
    return !(*this == other);
}

void ResourceOverridesDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ResourceOverrides { ");
    w.Write("Endpoints = {");
    for (auto const& endpoint : Endpoints)
    {
        w.Write("{0},", endpoint);
    }
    w.Write("}");

    w.Write("}");
}

void ResourceOverridesDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    xmlReader->StartElement(
        *SchemaNames::Element_ResourceOverrides,
        *SchemaNames::Namespace,
        false);

    if (xmlReader->IsEmptyElement())
    {
        // <ResourceOverrides />
        xmlReader->ReadElement();
        return;
    }

    xmlReader->ReadStartElement();


    ParseEndpoints(xmlReader);

    // </ResourceOverrides>
    xmlReader->ReadEndElement();
}

ErrorCode ResourceOverridesDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
    if (NoEndPoints())
    {
        return ErrorCodeValue::Success;
    }

    ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ResourceOverrides, L"", *SchemaNames::Namespace);
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


ErrorCode ResourceOverridesDescription::WriteEndpoints(XmlWriterUPtr const & xmlWriter)
{
    if (NoEndPoints())
    {
        return ErrorCodeValue::Success;
    }
    ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_Endpoints, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }
    for (auto endpoint : Endpoints)
    {
        er = endpoint.WriteToXml(xmlWriter);
        if (!er.IsSuccess())
        {
            return er;
        }
    }

    return xmlWriter->WriteEndElement();
}

bool ResourceOverridesDescription::NoEndPoints()
{
    return this->Endpoints.empty();
}

void ResourceOverridesDescription::ParseEndpoints(XmlReaderUPtr const & xmlReader)
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
                EndpointOverrideDescription endpoint;
                endpoint.ReadFromXml(xmlReader);
                this->Endpoints.push_back(move(endpoint));
                done = false;
            }
        }

        // </Endpoints>
        xmlReader->ReadEndElement();
    }
}

void ResourceOverridesDescription::clear()
{
    this->Endpoints.clear();
}
