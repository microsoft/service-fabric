// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

InfrastructureNodeDescription::InfrastructureNodeDescription()
    : NodeName(),
    NodeTypeRef(),
    RoleOrTierName(),
    IsSeedNode(false),
    IPAddressOrFQDN(),
    FaultDomain(),
    UpgradeDomain()
{
}

InfrastructureNodeDescription::InfrastructureNodeDescription(InfrastructureNodeDescription const & other)
    : NodeName(other.NodeName),
    NodeTypeRef(other.NodeTypeRef),
    RoleOrTierName(other.RoleOrTierName),
    IsSeedNode(other.IsSeedNode),
    IPAddressOrFQDN(other.IPAddressOrFQDN),
    FaultDomain(other.FaultDomain),
    UpgradeDomain(other.UpgradeDomain)
{
}

InfrastructureNodeDescription::InfrastructureNodeDescription(InfrastructureNodeDescription && other) 
    : NodeName(move(other.NodeName)),
    NodeTypeRef(move(other.NodeTypeRef)),
    RoleOrTierName(move(other.RoleOrTierName)),
    IsSeedNode(other.IsSeedNode),
    IPAddressOrFQDN(move(other.IPAddressOrFQDN)),
    FaultDomain(move(other.FaultDomain)),
    UpgradeDomain(move(other.UpgradeDomain))
{
}

InfrastructureNodeDescription const & InfrastructureNodeDescription::operator = (InfrastructureNodeDescription const & other)
{
    if (this != & other)
    {
        this->NodeName = other.NodeName;
        this->IPAddressOrFQDN = other.IPAddressOrFQDN;
        this->RoleOrTierName = other.RoleOrTierName;
        this->NodeTypeRef = other.NodeTypeRef;
        this->IsSeedNode = other.IsSeedNode;
        this->FaultDomain = other.FaultDomain;
        this->UpgradeDomain = other.UpgradeDomain;
    }

    return *this;
}

InfrastructureNodeDescription const & InfrastructureNodeDescription::operator = (InfrastructureNodeDescription && other)
{
    if (this != & other)
    {
        this->NodeName = move(other.NodeName);
        this->NodeTypeRef = move(other.NodeTypeRef);
        this->RoleOrTierName = move(other.RoleOrTierName);
        this->IsSeedNode = other.IsSeedNode;
        this->IPAddressOrFQDN = move(other.IPAddressOrFQDN);
        this->FaultDomain = move(other.FaultDomain);
        this->UpgradeDomain = move(other.UpgradeDomain);
    }

    return *this;
}

bool InfrastructureNodeDescription::operator == (InfrastructureNodeDescription const & other) const
{
    bool equals = true;

    equals = this->NodeName == other.NodeName;
    if (!equals) { return equals; }

    equals = this->NodeTypeRef == other.NodeTypeRef;
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->RoleOrTierName, other.RoleOrTierName);
    if (!equals) { return equals; }

    equals = this->IsSeedNode == other.IsSeedNode;
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->IPAddressOrFQDN, other.IPAddressOrFQDN);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->FaultDomain, other.FaultDomain);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->UpgradeDomain, other.UpgradeDomain);
    if (!equals) { return equals; }

    return equals;
}

bool InfrastructureNodeDescription::operator != (InfrastructureNodeDescription const & other) const
{
    return !(*this == other);
}

void InfrastructureNodeDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("Node { ");
    w.Write("NodeName = {0}, ", NodeName);
    w.Write("NodeTypeRef = {0}, ", NodeTypeRef);
    w.Write("RoleOrTierName = {0}, ", RoleOrTierName);
    w.Write("IsSeedNode = {0}, ", IsSeedNode);
    w.Write("IPAddressOrFQDN = {0}, ", IPAddressOrFQDN);
    w.Write("FaultDomain = {0}, ", FaultDomain);
    w.Write("UpgradeDomain = {0}, ", UpgradeDomain);
    w.Write("}");
}

void InfrastructureNodeDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    // ensure that we are on <Node ..
    xmlReader->StartElement(
        *SchemaNames::Element_Node,
        *SchemaNames::Namespace);

    // Read attributes on Node
    {
        this->NodeName = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_NodeName);
        this->NodeTypeRef = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_NodeTypeRef);
        this->RoleOrTierName = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_RoleOrTierName);
    
	    if (xmlReader->HasAttribute(*SchemaNames::Attribute_IsSeedNode))
	    {
	        wstring attrValue = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_IsSeedNode);
	        if (!StringUtility::TryFromWString<bool>(
		        attrValue,
		        this->IsSeedNode))
	        {
		        Parser::ThrowInvalidContent(xmlReader, L"true/false", attrValue);
	        }
	    }
    
        this->IPAddressOrFQDN = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_IPAddressOrFQDN);
        if (xmlReader->HasAttribute(*SchemaNames::Attribute_FaultDomain))
        {
	        this->FaultDomain = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_FaultDomain);
        }

	    if (xmlReader->HasAttribute(*SchemaNames::Attribute_UpgradeDomain))
	    {
	        this->UpgradeDomain = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_UpgradeDomain);
	    }
    }

    // Read child elements
    if (xmlReader->IsEmptyElement())
    {
        // <Node .../>
        xmlReader->ReadElement();
    }
    else
    {
        // <Node ...>
        xmlReader->ReadStartElement();

        xmlReader->SkipElement(
            *SchemaNames::Element_Endpoints,
            *SchemaNames::Namespace,
            true);

        xmlReader->SkipElement(
            *SchemaNames::Element_Certificates,
            *SchemaNames::Namespace,
            true);

        // </Node>
        xmlReader->ReadEndElement();
    }
}

 void InfrastructureNodeDescription::clear()
{
    this->NodeName.clear();
    this->NodeTypeRef.clear();
    this->RoleOrTierName.clear();
    this->IsSeedNode = false;
    this->IPAddressOrFQDN.clear();
    this->FaultDomain.clear();
    this->UpgradeDomain.clear();
}
