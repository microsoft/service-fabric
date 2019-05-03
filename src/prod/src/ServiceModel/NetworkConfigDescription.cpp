// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

NetworkConfigDescription::NetworkConfigDescription()
    : Type()
{
}

NetworkConfigDescription::NetworkConfigDescription(NetworkConfigDescription const & other)
    : Type(other.Type)
{
}

NetworkConfigDescription::NetworkConfigDescription(NetworkConfigDescription && other)
    : Type(other.Type)
{
}

NetworkConfigDescription const & NetworkConfigDescription::operator=(NetworkConfigDescription const & other)
{
    if (this != &other)
    {
        this->Type = other.Type;
    }
    return *this;
}

NetworkConfigDescription const & NetworkConfigDescription::operator=(NetworkConfigDescription && other)
{
    if (this != &other)
    {
        this->Type = move(other.Type);
    }
    return *this;
}

bool NetworkConfigDescription::operator== (NetworkConfigDescription const & other) const
{
    return Type == other.Type;
}

bool NetworkConfigDescription::operator!= (NetworkConfigDescription const & other) const
{
    return !(*this == other);
}

void NetworkConfigDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("NetworkConfigDescription { ");
    w.Write("Type = {0}, ", Type);
    w.Write("}");
}

void NetworkConfigDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    xmlReader->StartElement(
        *SchemaNames::Element_NetworkConfig,
        *SchemaNames::Namespace);

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_NetworkType))
    {
        NetworkType::FromString(xmlReader->ReadAttributeValue(*SchemaNames::Attribute_NetworkType), Type);
    }
    else
    {
        Type = NetworkType::Enum::Other;
    }
    // Read the rest of the empty element
    xmlReader->ReadElement();
}

Common::ErrorCode NetworkConfigDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{	//<NetworkConfig>
    ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_NetworkConfig, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Type, NetworkType::EnumToString(this->Type));
    if (!er.IsSuccess())
    {
        return er;
    }

    //</NetworkConfig>
    return xmlWriter->WriteEndElement();
}

void NetworkConfigDescription::clear()
{
}
