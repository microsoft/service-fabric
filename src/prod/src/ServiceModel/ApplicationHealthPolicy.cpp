// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

Global<ApplicationHealthPolicy> ApplicationHealthPolicy::Default = make_global<ApplicationHealthPolicy>();

ApplicationHealthPolicy::ApplicationHealthPolicy()
    : ConsiderWarningAsError(false),
    MaxPercentUnhealthyDeployedApplications(0),
    DefaultServiceTypeHealthPolicy(),
    ServiceTypeHealthPolicies()
{
}

ApplicationHealthPolicy::ApplicationHealthPolicy(
    bool considerWarningAsError,
    BYTE maxPercentUnhealthyDeployedApplications,
    ServiceTypeHealthPolicy const & defaultServiceTypeHealthPolicy,
    ServiceTypeHealthPolicyMap const & serviceHealthTypePolicies)
    : ConsiderWarningAsError(considerWarningAsError),
    MaxPercentUnhealthyDeployedApplications(maxPercentUnhealthyDeployedApplications),
    DefaultServiceTypeHealthPolicy(defaultServiceTypeHealthPolicy),
    ServiceTypeHealthPolicies(serviceHealthTypePolicies)
{
}

bool ApplicationHealthPolicy::operator == (ApplicationHealthPolicy const & other) const
{
    bool equals = (this->ConsiderWarningAsError == other.ConsiderWarningAsError);
    if (!equals) return equals;

    equals = (this->MaxPercentUnhealthyDeployedApplications == other.MaxPercentUnhealthyDeployedApplications);
    if (!equals) return equals;

    equals = (this->DefaultServiceTypeHealthPolicy == other.DefaultServiceTypeHealthPolicy);
    if (!equals) return equals;

    equals = (this->ServiceTypeHealthPolicies == other.ServiceTypeHealthPolicies);
    if (!equals) return equals;

    return equals;
}

bool ApplicationHealthPolicy::operator != (ApplicationHealthPolicy const & other) const
{
    return !(*this == other);
}

void ApplicationHealthPolicy::ReadFromXml(XmlReaderUPtr const & xmlReader)
{
    this->clear();

    // ensure that we are on <HealthPolicy
    xmlReader->StartElement(
        *SchemaNames::Element_HealthPolicy,
        *SchemaNames::Namespace);

    {
        if (xmlReader->HasAttribute(*SchemaNames::Attribute_ConsiderWarningAsError))
        {
            wstring attrValue = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ConsiderWarningAsError);
            if (!StringUtility::TryFromWString<bool>(attrValue, this->ConsiderWarningAsError))
            {
                Parser::ThrowInvalidContent(xmlReader, L"true/false", attrValue);
            }
        }
        
        if (xmlReader->HasAttribute(*SchemaNames::Attribute_MaxPercentUnhealthyDeployedApplications))
        {
            Parser::Utility::ReadPercentageAttribute(
                xmlReader, 
                *SchemaNames::Attribute_MaxPercentUnhealthyDeployedApplications, 
                this->MaxPercentUnhealthyDeployedApplications);
        }
    }

    
    if (xmlReader->IsEmptyElement())
    {
        xmlReader->ReadElement();
    }
    else
    {
        // <HealthPolicy ..>
        xmlReader->ReadStartElement();
        
        this->ReadDefaultServiceTypeHealthPolicy(xmlReader);
        this->ReadServiceTypeHealthPolicyMap(xmlReader);
        
        xmlReader->ReadEndElement();
    }
}

void ApplicationHealthPolicy::ReadDefaultServiceTypeHealthPolicy(XmlReaderUPtr const & xmlReader)
{
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_DefaultServiceTypeHealthPolicy,
        *SchemaNames::Namespace))
    {         
        this->DefaultServiceTypeHealthPolicy.ReadFromXml(xmlReader);
        if (xmlReader->IsEmptyElement())
        {
            xmlReader->ReadElement();
        }
        else
        {
            xmlReader->ReadStartElement();
            xmlReader->ReadEndElement();
        }
    }
}

void ApplicationHealthPolicy::ReadServiceTypeHealthPolicyMap(XmlReaderUPtr const & xmlReader)
{
    bool done = false;
    while(!done)
    {
        if (xmlReader->IsStartElement(
            *SchemaNames::Element_ServiceTypeHealthPolicy,
            *SchemaNames::Namespace))
        {
            wstring serviceTypeName = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ServiceTypeName);
            ServiceTypeHealthPolicy serviceTypeHealthPolicy;

            serviceTypeHealthPolicy.ReadFromXml(xmlReader);
            if (xmlReader->IsEmptyElement())
            {
                xmlReader->ReadElement();
            }
            else
            {
                xmlReader->ReadStartElement();
                xmlReader->ReadEndElement();
            }

            this->ServiceTypeHealthPolicies.insert(make_pair(serviceTypeName, move(serviceTypeHealthPolicy)));
        }
        else
        {
            done = true;
        }
    }
}

ServiceTypeHealthPolicy const & ApplicationHealthPolicy::GetServiceTypeHealthPolicy(wstring const & serviceTypeName) const 
{
    auto iter  = this->ServiceTypeHealthPolicies.find(serviceTypeName);
    if (iter == this->ServiceTypeHealthPolicies.end())
    {
        return this->DefaultServiceTypeHealthPolicy;
    }
    else
    {
        return iter->second;
    }
}

void ApplicationHealthPolicy::WriteTo(TextWriter & w, FormatOptions const &) const
{
   w.Write("ApplicationHealthPolicy = {0}", ToString());
}

wstring ApplicationHealthPolicy::ToString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<ApplicationHealthPolicy&>(*this), objectString);
    if (!error.IsSuccess())
    {
        return wstring();
    }

    return objectString;
}

ErrorCode ApplicationHealthPolicy::FromString(wstring const & applicationHealthPolicyStr, __out ApplicationHealthPolicy & applicationHealthPolicy)
{
    return JsonHelper::Deserialize(applicationHealthPolicy, applicationHealthPolicyStr);
}

ErrorCode ApplicationHealthPolicy::FromPublicApi(FABRIC_APPLICATION_HEALTH_POLICY const & publicAppHealthPolicy)
{
    if (publicAppHealthPolicy.DefaultServiceTypeHealthPolicy != NULL)
    {
        auto error = this->DefaultServiceTypeHealthPolicy.FromPublicApi(*publicAppHealthPolicy.DefaultServiceTypeHealthPolicy);
        if (!error.IsSuccess()) { return error; } 
    }

    if (publicAppHealthPolicy.ServiceTypeHealthPolicyMap != NULL)
    {
        auto serviceTypeHealthPolicyMap = publicAppHealthPolicy.ServiceTypeHealthPolicyMap;

        for (size_t i = 0; i < serviceTypeHealthPolicyMap->Count; ++i)
        {
            wstring serviceTypeName;
            ServiceTypeHealthPolicy item;

            auto error = item.FromPublicApi(serviceTypeHealthPolicyMap->Items[i], serviceTypeName);
            if (!error.IsSuccess()) return error;
        
            this->ServiceTypeHealthPolicies[serviceTypeName] = item;
        }
    }
    
    if (publicAppHealthPolicy.ConsiderWarningAsError)
    {
        this->ConsiderWarningAsError = true;
    }
    else
    {
        this->ConsiderWarningAsError = false;
    }

    auto error = ParameterValidator::ValidatePercentValue(publicAppHealthPolicy.MaxPercentUnhealthyDeployedApplications, Constants::MaxPercentUnhealthyDeployedApplications);
    if (!error.IsSuccess()) { return error; }
    this->MaxPercentUnhealthyDeployedApplications = publicAppHealthPolicy.MaxPercentUnhealthyDeployedApplications;

    return ErrorCode(ErrorCodeValue::Success);
}

void ApplicationHealthPolicy::ToPublicApi(
    __in ScopedHeap & heap, 
    __out FABRIC_APPLICATION_HEALTH_POLICY & publicAppHealthPolicy) const
{
    publicAppHealthPolicy.ConsiderWarningAsError = this->ConsiderWarningAsError ? TRUE : FALSE;
    publicAppHealthPolicy.MaxPercentUnhealthyDeployedApplications = this->MaxPercentUnhealthyDeployedApplications;

    // DefaultServiceHealthPolicy
    auto defaultServiceTypeHealthPolicy = heap.AddItem<FABRIC_SERVICE_TYPE_HEALTH_POLICY>();
    publicAppHealthPolicy.DefaultServiceTypeHealthPolicy = defaultServiceTypeHealthPolicy.GetRawPointer();
    this->DefaultServiceTypeHealthPolicy.ToPublicApi(heap, *defaultServiceTypeHealthPolicy);

    // ServiceHealthPolicy Map
    auto publicServiceTypeHealthPolicies = heap.AddItem<FABRIC_SERVICE_TYPE_HEALTH_POLICY_MAP>();
    publicAppHealthPolicy.ServiceTypeHealthPolicyMap = publicServiceTypeHealthPolicies.GetRawPointer();
    publicServiceTypeHealthPolicies->Count = static_cast<ULONG>(this->ServiceTypeHealthPolicies.size());
    
    if (publicServiceTypeHealthPolicies->Count > 0)
    {
        auto mapItems = heap.AddArray<FABRIC_SERVICE_TYPE_HEALTH_POLICY_MAP_ITEM>(publicServiceTypeHealthPolicies->Count);
        publicServiceTypeHealthPolicies->Items = mapItems.GetRawArray();

        int index = 0;
        for (auto const & entry : this->ServiceTypeHealthPolicies)
        {
            entry.second.ToPublicApiMapItem(heap, entry.first, mapItems[index++]);
        }
    }
    else
    {
        publicServiceTypeHealthPolicies->Items = NULL;
    }
}

void ApplicationHealthPolicy::ToPublicApiMapItem(
    __in ScopedHeap & heap,
    wstring const & applicationName,
    __out FABRIC_APPLICATION_HEALTH_POLICY_MAP_ITEM & publicApplicationHealthPolicyItem) const
{
    auto applicationHealthPolicy = heap.AddItem<FABRIC_APPLICATION_HEALTH_POLICY>();
    this->ToPublicApi(heap, *applicationHealthPolicy);
    publicApplicationHealthPolicyItem.HealthPolicy = applicationHealthPolicy.GetRawPointer();
    publicApplicationHealthPolicyItem.ApplicationName = heap.AddString(applicationName);
}

bool ApplicationHealthPolicy::TryValidate(__out wstring & validationErrorMessage) const
{
    auto error = ParameterValidator::ValidatePercentValue(MaxPercentUnhealthyDeployedApplications, Constants::MaxPercentUnhealthyDeployedApplications);
    if (!error.IsSuccess())
    {
        validationErrorMessage = error.TakeMessage();
        return false;
    }

    wstring tempMessage;
    if (!DefaultServiceTypeHealthPolicy.TryValidate(tempMessage))
    {
        validationErrorMessage = wformatString("{0} (Default)", tempMessage);
        return false;
    }

    for (auto const & it : ServiceTypeHealthPolicies)
    {
        if (!it.second.TryValidate(tempMessage))
        {
            validationErrorMessage = wformatString("{0} ({1})", it.first);
            return false;
        }
    }

    return true;
}

void ApplicationHealthPolicy::clear()
{
    this->ConsiderWarningAsError = false;
    this->MaxPercentUnhealthyDeployedApplications = 0;
    this->DefaultServiceTypeHealthPolicy.clear();
    this->ServiceTypeHealthPolicies.clear();
}

ErrorCode ApplicationHealthPolicy::WriteToXml(XmlWriterUPtr const & xmlWriter)
{	//<HealthPolicy>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_HealthPolicy, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteBooleanAttribute(*SchemaNames::Attribute_ConsiderWarningAsError,
		this->ConsiderWarningAsError);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_MaxPercentUnhealthyDeployedApplications, this->MaxPercentUnhealthyDeployedApplications);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = this->DefaultServiceTypeHealthPolicy.WriteToXml(xmlWriter, true);
	if (!er.IsSuccess())
	{
		return er;
	}
	for (ServiceTypeHealthPolicyMap::iterator it = ServiceTypeHealthPolicies.begin(); it != ServiceTypeHealthPolicies.end(); ++it)
	{
		er = (*it).second.WriteToXml(xmlWriter, false, (*it).first);
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	//</HealthPolicy>
	return xmlWriter->WriteEndElement();
}
