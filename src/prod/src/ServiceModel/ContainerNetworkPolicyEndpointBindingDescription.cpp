// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ContainerNetworkPolicyEndpointBindingDescription::ContainerNetworkPolicyEndpointBindingDescription()
    : EndpointRef()
{
}

ContainerNetworkPolicyEndpointBindingDescription::ContainerNetworkPolicyEndpointBindingDescription(ContainerNetworkPolicyEndpointBindingDescription const & other)
    : EndpointRef(other.EndpointRef)
{
}

ContainerNetworkPolicyEndpointBindingDescription::ContainerNetworkPolicyEndpointBindingDescription(ContainerNetworkPolicyEndpointBindingDescription && other)
    : EndpointRef(move(other.EndpointRef))
{
}

ContainerNetworkPolicyEndpointBindingDescription const & ContainerNetworkPolicyEndpointBindingDescription::operator = (ContainerNetworkPolicyEndpointBindingDescription const & other)
{
    if (this != &other)
    {
        this->EndpointRef = other.EndpointRef;
    }

    return *this;
}

ContainerNetworkPolicyEndpointBindingDescription const & ContainerNetworkPolicyEndpointBindingDescription::operator = (ContainerNetworkPolicyEndpointBindingDescription && other)
{
    if (this != &other)
    {
        this->EndpointRef = move(other.EndpointRef);
    }

    return *this;
}

bool ContainerNetworkPolicyEndpointBindingDescription::operator == (ContainerNetworkPolicyEndpointBindingDescription const & other) const
{
    return (this->EndpointRef == other.EndpointRef);
}

bool ContainerNetworkPolicyEndpointBindingDescription::operator != (ContainerNetworkPolicyEndpointBindingDescription const & other) const
{
    return !(*this == other);
}

void ContainerNetworkPolicyEndpointBindingDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ContainerNetworkPolicyEndpointBindingDescription { ");
    w.Write("EndpointRef = {0} ", EndpointRef);
    w.Write("}");
}

ErrorCode ContainerNetworkPolicyEndpointBindingDescription::FromString(wstring const & containerNetworkPolicyEndpointBindingDescriptionStr, __out ContainerNetworkPolicyEndpointBindingDescription & containerNetworkPolicyEndpointBindingDescription)
{
    return JsonHelper::Deserialize(containerNetworkPolicyEndpointBindingDescription, containerNetworkPolicyEndpointBindingDescriptionStr);
}

void ContainerNetworkPolicyEndpointBindingDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    // <EndpointBinding>
    xmlReader->StartElement(
        *SchemaNames::Element_EndpointBinding,
        *SchemaNames::Namespace);

    this->EndpointRef = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_EndpointRef);

    xmlReader->ReadElement();
}


Common::ErrorCode ContainerNetworkPolicyEndpointBindingDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
    // <EndpointBinding>
    ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_EndpointBinding, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }

    xmlWriter->WriteAttribute(*SchemaNames::Attribute_EndpointRef, this->EndpointRef);
    if (!er.IsSuccess())
    {
        return er;
    }

    // </EndpointBinding>
    return xmlWriter->WriteEndElement();

}

void ContainerNetworkPolicyEndpointBindingDescription::clear()
{
    this->EndpointRef.clear();
}

