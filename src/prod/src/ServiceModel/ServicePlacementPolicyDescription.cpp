// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(ServicePlacementPolicyDescription)

ServicePlacementPolicyDescription::ServicePlacementPolicyDescription()
    : domainName_(), 
    type_(FABRIC_PLACEMENT_POLICY_INVALID)
{
}

ServicePlacementPolicyDescription::ServicePlacementPolicyDescription(
    std::wstring const& domainName, FABRIC_PLACEMENT_POLICY_TYPE type )
    :  domainName_(domainName),
    type_(type)
{
}

ServicePlacementPolicyDescription::ServicePlacementPolicyDescription(ServicePlacementPolicyDescription const& other)
    : domainName_(other.domainName_), 
    type_(other.type_)
{
}

ServicePlacementPolicyDescription::ServicePlacementPolicyDescription(ServicePlacementPolicyDescription && other)
    : domainName_(std::move(other.domainName_)), 
    type_(other.type_)
{
}

ServicePlacementPolicyDescription & ServicePlacementPolicyDescription::operator = (ServicePlacementPolicyDescription const& other)
{
    if (this != &other)
    {
        domainName_ = other.domainName_;
        type_ = other.type_;
    }

    return *this;
}

ServicePlacementPolicyDescription & ServicePlacementPolicyDescription::operator = (ServicePlacementPolicyDescription && other)
{
    if (this != &other)
    {
        domainName_ = move(other.domainName_);
        type_ = other.type_;
    }

    return *this;
}

bool ServicePlacementPolicyDescription::operator == (ServicePlacementPolicyDescription const & other) const
{
    return Equals(other).IsSuccess();
}

bool ServicePlacementPolicyDescription::operator != (ServicePlacementPolicyDescription const & other) const
{
    return !(*this == other);
}

ErrorCode ServicePlacementPolicyDescription::Equals(ServicePlacementPolicyDescription const & other) const
{
    if (!StringUtility::AreEqualCaseInsensitive(domainName_, other.domainName_))
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(
                GET_FM_RC(ServicePlacementPolicyDescription_DomainName_Changed), domainName_, other.domainName_));
    }

    if (type_ != other.type_)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(
                GET_FM_RC(ServicePlacementPolicyDescription_Type_Changed), type_, other.type_));
    }

    return ErrorCode::Success();
}

void ServicePlacementPolicyDescription::WriteToEtw(uint16 contextSequenceId) const
{
    ServiceModel::ServiceModelEventSource::Trace->ServicePlacementPolicyDescription(
        contextSequenceId,
        domainName_,
        static_cast<int>(type_));
}

void ServicePlacementPolicyDescription::WriteTo(TextWriter& w, FormatOptions const&) const
{
    w.Write("ServicePlacementPolicyDescription { ");

    w.Write("DomainName = {0}, ", this->DomainName);    
    w.Write("Type = {0} ", FabricPlacementPolicyTypeToString());      

    w.Write("}");
}

FABRIC_PLACEMENT_POLICY_TYPE ServicePlacementPolicyDescription::GetType(std::wstring const & val)
{
    if (StringUtility::AreEqualCaseInsensitive(val, L"InvalidDomain"))
    {
        return FABRIC_PLACEMENT_POLICY_INVALID_DOMAIN;
    }
    else if (StringUtility::AreEqualCaseInsensitive(val, L"RequiredDomain"))
    {
        return FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN;
    }
    else if (StringUtility::AreEqualCaseInsensitive(val, L"PreferredPrimaryDomain"))
    {
        return FABRIC_PLACEMENT_POLICY_PREFERRED_PRIMARY_DOMAIN;
    }
    else if (StringUtility::AreEqualCaseInsensitive(val, L"RequiredDomainDistribution"))
    {
        return FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN_DISTRIBUTION;
    }
    else if (StringUtility::AreEqualCaseInsensitive(val, L"NonPartiallyPlaceService"))
    {
        return FABRIC_PLACEMENT_POLICY_NONPARTIALLY_PLACE_SERVICE;
    }
    else
    {
        return FABRIC_PLACEMENT_POLICY_INVALID;
    }
}

void ServicePlacementPolicyDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    xmlReader->StartElement(
        *SchemaNames::Element_ServicePlacementPolicy,
        *SchemaNames::Namespace);

    this->domainName_ = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_DomainName);
    this->type_ = GetType(xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Type));

    xmlReader->ReadElement();
}

ErrorCode ServicePlacementPolicyDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	//<ServicePlacementPolicy>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ServicePlacementPolicy, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	//Attributes
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_DomainName, this->domainName_);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Type, FabricPlacementPolicyTypeToString());
	if (!er.IsSuccess())
	{
		return er;
	}
	//</ServicePlacementPolicy>
	return xmlWriter->WriteEndElement();
}

void ServicePlacementPolicyDescription::clear()
{
    domainName_.clear();    
}

std::wstring ServicePlacementPolicyDescription::ToString() const
{
    return wformatString("Type = '{0}'; DomainName = '{1}';", FabricPlacementPolicyTypeToString(), DomainName); 
}

void ServicePlacementPolicyDescription::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_SERVICE_PLACEMENT_POLICY_DESCRIPTION & policyDesc) const
{
    policyDesc.Type = type_;

    switch (type_)
    {
    case FABRIC_PLACEMENT_POLICY_INVALID_DOMAIN:
        {
            auto domainValue = heap.AddItem<FABRIC_PLACEMENT_POLICY_INVALID_DOMAIN_DESCRIPTION>();
            domainValue->InvalidFaultDomain = heap.AddString(DomainName);
            policyDesc.Value = domainValue.GetRawPointer();
            break;
        }
    case FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN:
        {
            auto domainValue = heap.AddItem<FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN_DESCRIPTION>();
            domainValue->RequiredFaultDomain = heap.AddString(DomainName);
            policyDesc.Value = domainValue.GetRawPointer();
            break;
        }
    case FABRIC_PLACEMENT_POLICY_PREFERRED_PRIMARY_DOMAIN:
        {
            auto domainValue = heap.AddItem<FABRIC_PLACEMENT_POLICY_PREFERRED_PRIMARY_DOMAIN_DESCRIPTION>();
            domainValue->PreferredPrimaryFaultDomain = heap.AddString(DomainName);
            policyDesc.Value = domainValue.GetRawPointer();
            break;
        }
    case FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN_DISTRIBUTION:
        {
            auto domainValue = heap.AddItem<FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN_DISTRIBUTION_DESCRIPTION>();
            policyDesc.Value = domainValue.GetRawPointer();
            break;
        }
    case FABRIC_PLACEMENT_POLICY_NONPARTIALLY_PLACE_SERVICE:
        {
            auto domainValue = heap.AddItem<FABRIC_PLACEMENT_POLICY_NONPARTIALLY_PLACE_SERVICE_DESCRIPTION>();
            policyDesc.Value = domainValue.GetRawPointer();
            break;
        }

    default:
        policyDesc.Type = FABRIC_PLACEMENT_POLICY_INVALID;
    }

}

void ServicePlacementPolicyDescription::ToPlacementConstraints(__out std::wstring & placementConstraints) const
{
    Common::ServicePlacementPolicyHelper::PolicyDescriptionToPlacementConstraints(type_, domainName_, placementConstraints);
}

std::wstring ServicePlacementPolicyDescription::FabricPlacementPolicyTypeToString() const
{
    switch (type_)
    {
    case FABRIC_PLACEMENT_POLICY_INVALID_DOMAIN:
        return L"InvalidDomain";
    case FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN:
        return L"RequiredDomain";
    case FABRIC_PLACEMENT_POLICY_PREFERRED_PRIMARY_DOMAIN:
        return L"PreferredPrimaryDomain";
    case FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN_DISTRIBUTION:
        return L"RequiredDomainDistribution";
    case FABRIC_PLACEMENT_POLICY_NONPARTIALLY_PLACE_SERVICE:
        return L"NonPartiallyPlaceService";
    default:
        return L"Invalid";
    }
}
