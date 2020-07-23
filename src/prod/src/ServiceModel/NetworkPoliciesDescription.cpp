// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

NetworkPoliciesDescription::NetworkPoliciesDescription()
    : ContainerNetworkPolicies()
{
}

NetworkPoliciesDescription::NetworkPoliciesDescription(NetworkPoliciesDescription const & other)
    : ContainerNetworkPolicies(other.ContainerNetworkPolicies)    
{
}

NetworkPoliciesDescription::NetworkPoliciesDescription(NetworkPoliciesDescription && other)
    : ContainerNetworkPolicies(move(other.ContainerNetworkPolicies))
{
}

NetworkPoliciesDescription const & NetworkPoliciesDescription::operator = (NetworkPoliciesDescription const & other)
{
    if (this != &other)
    {
        this->ContainerNetworkPolicies = other.ContainerNetworkPolicies;
    }
    return *this;
}

NetworkPoliciesDescription const & NetworkPoliciesDescription::operator = (NetworkPoliciesDescription && other)
{
    if (this != &other)
    {
        this->ContainerNetworkPolicies = move(other.ContainerNetworkPolicies);
    }
    return *this;
}

bool NetworkPoliciesDescription::operator == (NetworkPoliciesDescription const & other) const
{
    bool equals = true;

    if (this->ContainerNetworkPolicies.size() != other.ContainerNetworkPolicies.size()) { return false;}
    for (auto i = 0; i < ContainerNetworkPolicies.size(); ++i)
    {
        equals = this->ContainerNetworkPolicies[i] == other.ContainerNetworkPolicies[i];
        if (!equals) { return equals; }
    }

    return equals;
}

bool NetworkPoliciesDescription::operator != (NetworkPoliciesDescription const & other) const
{
    return !(*this == other);
}

void NetworkPoliciesDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("NetworkPoliciesDescription { ");    

    w.Write("ContainerNetworkPolicies = { ");
    for (auto iter = ContainerNetworkPolicies.cbegin(); iter != ContainerNetworkPolicies.cend(); ++iter)
    {
        w.Write("{0}, ", *iter);
    }
    w.Write("} ");

    w.Write("}");

}

ErrorCode NetworkPoliciesDescription::FromString(wstring const & networkPoliciesDescriptionStr, __out NetworkPoliciesDescription & networkPoliciesDescription)
{
    return JsonHelper::Deserialize(networkPoliciesDescription, networkPoliciesDescriptionStr);
}

void NetworkPoliciesDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    // <NetworkPolicies>
    xmlReader->StartElement(
        *SchemaNames::Element_NetworkPolicies,
        *SchemaNames::Namespace,
        false);

    if (xmlReader->IsEmptyElement())
    {
        // <NetworkPolicies />
        xmlReader->ReadElement();
        return;
    }

    xmlReader->MoveToNextElement();
    
    bool hasPolicies = true;
    while (hasPolicies)
    {
        if (xmlReader->IsStartElement(
            *SchemaNames::Element_ContainerNetworkPolicy,
            *SchemaNames::Namespace))
        {
            ContainerNetworkPolicyDescription description;
            description.ReadFromXml(xmlReader);
            this->ContainerNetworkPolicies.push_back(description);
        }
        else
        {
            hasPolicies = false;
        }
    }

    // </NetworkPolicies>
    xmlReader->ReadEndElement();
}
ErrorCode NetworkPoliciesDescription::WriteToXml(Common::XmlWriterUPtr const & xmlWriter)
{
    // <NetworkPolicies>
    ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_NetworkPolicies, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }

    for (auto i = 0; i < ContainerNetworkPolicies.size(); i++)
    {
        er = ContainerNetworkPolicies[i].WriteToXml(xmlWriter);

        if (!er.IsSuccess())
        {
            return er;
        }
    }

    // </NetworkPolicies>
    return xmlWriter->WriteEndElement();
}

void NetworkPoliciesDescription::clear()
{
    this->ContainerNetworkPolicies.clear();
}
