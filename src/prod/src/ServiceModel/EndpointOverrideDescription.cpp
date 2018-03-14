// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

EndpointOverrideDescription::EndpointOverrideDescription() 
    : Name(),
    Protocol(),
    Type(),
    Port(),
    ExplicitPortSpecified(false),
    UriScheme(),
    PathSuffix()
{
}

EndpointOverrideDescription::EndpointOverrideDescription(
    EndpointOverrideDescription const & other) 
    : Name(other.Name),
    Protocol(other.Protocol),
    Type(other.Type),
    Port(other.Port),
    ExplicitPortSpecified(other.ExplicitPortSpecified),
    UriScheme(other.UriScheme),
    PathSuffix(other.PathSuffix)
{
}

EndpointOverrideDescription::EndpointOverrideDescription(EndpointOverrideDescription && other) 
    : Name(move(other.Name)),
    Protocol(move(other.Protocol)),
    Type(move(other.Type)),
    Port(move(other.Port)),
    ExplicitPortSpecified(other.ExplicitPortSpecified),
    UriScheme(move(other.UriScheme)),
    PathSuffix(move(other.PathSuffix))
{
}

EndpointOverrideDescription & EndpointOverrideDescription::operator = (EndpointOverrideDescription const & other)
{
    if (this != &other)
    {
        this->Name = other.Name;
        this->Protocol = other.Protocol;
        this->Type = other.Type;
        this->Port = other.Port;
        this->ExplicitPortSpecified = other.ExplicitPortSpecified;
        this->UriScheme = other.UriScheme;
        this->PathSuffix = other.PathSuffix;
    }

    return *this;
}

EndpointOverrideDescription & EndpointOverrideDescription::operator = (EndpointOverrideDescription && other)
{
    if (this != &other)
    {
        this->Name = move(other.Name);
        this->Protocol = move(other.Protocol);
        this->Type = move(other.Type);
        this->Port = move(other.Port);
        this->ExplicitPortSpecified = other.ExplicitPortSpecified;
        this->UriScheme = move(other.UriScheme);
        this->PathSuffix = move(other.PathSuffix);
    }

    return *this;
}

bool EndpointOverrideDescription::operator == (EndpointOverrideDescription const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->Name, other.Name);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->Protocol, other.Protocol);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->Type, other.Type);
    if (!equals) { return equals; }    

    equals = StringUtility::AreEqualCaseInsensitive(this->Port, other.Port);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->UriScheme, other.UriScheme);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->PathSuffix, other.PathSuffix);
    if (!equals) { return equals; }

    return equals;
}

bool EndpointOverrideDescription::operator != (EndpointOverrideDescription const & other) const
{
    return !(*this == other);
}

void EndpointOverrideDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("EndpointOverrideDescription { ");
    w.Write("Name = {0}, Protocol = {1}, Type = {2}, Port = {3}, UriScheme={4}, PathSuffix={5}.", 
        Name, 
        Protocol,
        Type,
        Port,
        UriScheme,
        PathSuffix);
    w.Write("}");
}

void EndpointOverrideDescription::ReadFromXml(XmlReaderUPtr const & xmlReader)
{
    clear();

    xmlReader->StartElement(
        *SchemaNames::Element_Endpoint, 
        *SchemaNames::Namespace);

    this->Name = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Name);

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_Protocol))
    {
        this->Protocol = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Protocol);
    }

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_Type))
    {
        this->Type = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Type);
    }

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_Port))
    {
        this->Port = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Port);
        this->ExplicitPortSpecified = true;
    } 

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_UriScheme))
    {
        this->UriScheme = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_UriScheme);
    }
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_PathSuffix))
    {
        this->PathSuffix = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_PathSuffix);
    }
    xmlReader->ReadElement();
}

void EndpointOverrideDescription::clear()
{
    this->Name.clear();
    this->Port.clear();
    this->Type.clear();
    this->Protocol.clear();
    this->UriScheme.clear();
    this->PathSuffix.clear();
}

ErrorCode EndpointOverrideDescription::WriteToXml(Common::XmlWriterUPtr const & xmlWriter)
{
    ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_Endpoint, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Name, this->Name);
    if (!er.IsSuccess())
    {
        return er;
    }

    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Protocol, this->Protocol);
    if (!er.IsSuccess())
    {
        return er;
    }

    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Type, this->Type);
    if (!er.IsSuccess())
    {
        return er;
    }

    if (this->ExplicitPortSpecified)
    {
        er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Port, this->Port);
        if (!er.IsSuccess())
        {
            return er;
        }
    }

    if (!this->UriScheme.empty())
    {
        er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_UriScheme, this->UriScheme);
        if (!er.IsSuccess())
        {
            return er;
        }

    }

    if (!this->PathSuffix.empty())
    {
        er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_PathSuffix, this->PathSuffix);
        if (!er.IsSuccess())
        {
            return er;
        }
    }

    return xmlWriter->WriteEndElement();
}
