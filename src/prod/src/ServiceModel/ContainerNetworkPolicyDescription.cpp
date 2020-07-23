// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ContainerNetworkPolicyDescription::ContainerNetworkPolicyDescription()
    : NetworkRef(),
    EndpointBindings()
{
}

ContainerNetworkPolicyDescription::ContainerNetworkPolicyDescription(ContainerNetworkPolicyDescription const & other)
    : NetworkRef(other.NetworkRef),
    EndpointBindings(other.EndpointBindings)
{
}

ContainerNetworkPolicyDescription::ContainerNetworkPolicyDescription(ContainerNetworkPolicyDescription && other)
    : NetworkRef(move(other.NetworkRef)),
    EndpointBindings(move(other.EndpointBindings))
{
}

ContainerNetworkPolicyDescription const & ContainerNetworkPolicyDescription::operator = (ContainerNetworkPolicyDescription const & other)
{
    if (this != &other)
    {
        this->NetworkRef = other.NetworkRef;
        this->EndpointBindings = other.EndpointBindings;
    }
    return *this;
}

ContainerNetworkPolicyDescription const & ContainerNetworkPolicyDescription::operator = (ContainerNetworkPolicyDescription && other)
{
    if (this != &other)
    {
        this->NetworkRef = move(other.NetworkRef);
        this->EndpointBindings = move(other.EndpointBindings);
    }
    return *this;
}

bool ContainerNetworkPolicyDescription::operator == (ContainerNetworkPolicyDescription const & other) const
{
    bool equals = true;

    equals = this->NetworkRef == other.NetworkRef;
    if (!equals) { return equals; }

    if (this->EndpointBindings.size() != other.EndpointBindings.size()) { return false;}
    for (auto i = 0; i < EndpointBindings.size(); ++i)
    {
        equals = this->EndpointBindings[i] == other.EndpointBindings[i];
        if (!equals) { return equals; }
    }

    return equals;
}

bool ContainerNetworkPolicyDescription::operator != (ContainerNetworkPolicyDescription const & other) const
{
    return !(*this == other);
}

void ContainerNetworkPolicyDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ContainerNetworkPolicyDescription { ");

    w.Write("NetworkRef = {0}, ", NetworkRef);

    w.Write("EndpointBindings = { ");
    for (auto iter = EndpointBindings.cbegin(); iter != EndpointBindings.cend(); ++iter)
    {
        w.Write("{0}, ", *iter);
    }
    w.Write("} ");

    w.Write("}");

}

ErrorCode ContainerNetworkPolicyDescription::FromString(wstring const & containerNetworkPolicyDescriptionStr, __out ContainerNetworkPolicyDescription & containerNetworkPolicyDescription)
{
    return JsonHelper::Deserialize(containerNetworkPolicyDescription, containerNetworkPolicyDescriptionStr);
}

void ContainerNetworkPolicyDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    // <ContainerNetworkPolicy>
    xmlReader->StartElement(
        *SchemaNames::Element_ContainerNetworkPolicy,
        *SchemaNames::Namespace);

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_NetworkRef))
    {
        this->NetworkRef = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_NetworkRef);
    }

    if (xmlReader->IsEmptyElement())
    {
        // <ContainerNetworkPolicy />
        xmlReader->ReadElement();
        return;
    }

    xmlReader->MoveToNextElement();
    bool hasPolicies = true;
    while (hasPolicies)
    {
        if (xmlReader->IsStartElement(
            *SchemaNames::Element_EndpointBinding,
            *SchemaNames::Namespace))
        {
            ContainerNetworkPolicyEndpointBindingDescription description;
            description.ReadFromXml(xmlReader);
            this->EndpointBindings.push_back(description);
        }
        else
        {
            hasPolicies = false;
        }
    }

    // </ContainerNetworkPolicy>
    xmlReader->ReadEndElement();
}
ErrorCode ContainerNetworkPolicyDescription::WriteToXml(Common::XmlWriterUPtr const & xmlWriter)
{
    // <ContainerNetworkPolicy>
    ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ContainerNetworkPolicy, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_NetworkRef, this->NetworkRef);
    if (!er.IsSuccess())
    {
        return er;
    }

    for (auto i = 0; i < EndpointBindings.size(); i++)
    {
        er = EndpointBindings[i].WriteToXml(xmlWriter);

        if (!er.IsSuccess())
        {
            return er;
        }
    }

    // </ContainerNetworkPolicy>
    return xmlWriter->WriteEndElement();
}

void ContainerNetworkPolicyDescription::clear()
{
    this->NetworkRef.clear();
    this->EndpointBindings.clear();
}
