// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ServiceTypeDescription::ServiceTypeDescription()
    : IsStateful(false),
    ServiceTypeName(),
    PlacementConstraints(),
    HasPersistedState(false),
    UseImplicitHost(false),
    Extensions(),
    LoadMetrics(),
    ServicePlacementPolicies()
{
}

ServiceTypeDescription::ServiceTypeDescription(ServiceTypeDescription const & other)
    : IsStateful(other.IsStateful),
    ServiceTypeName(other.ServiceTypeName),
    PlacementConstraints(other.PlacementConstraints),
    HasPersistedState(other.HasPersistedState),
    UseImplicitHost(other.UseImplicitHost),
    Extensions(other.Extensions),
    LoadMetrics(other.LoadMetrics),
    ServicePlacementPolicies(other.ServicePlacementPolicies)
{
}

ServiceTypeDescription::ServiceTypeDescription(ServiceTypeDescription && other)
    : IsStateful(other.IsStateful),
    ServiceTypeName(move(other.ServiceTypeName)),
    PlacementConstraints(move(other.PlacementConstraints)),
    HasPersistedState(other.HasPersistedState),
    UseImplicitHost(other.UseImplicitHost),
    Extensions(move(other.Extensions)),
    LoadMetrics(move(other.LoadMetrics)),
    ServicePlacementPolicies(other.ServicePlacementPolicies)
{
}

ServiceTypeDescription const & ServiceTypeDescription::operator = (ServiceTypeDescription const & other)
{
    if (this != & other)
    {
        this->IsStateful = other.IsStateful;
        this->ServiceTypeName = other.ServiceTypeName;
        this->PlacementConstraints = other.PlacementConstraints;
        this->HasPersistedState = other.HasPersistedState;
        this->UseImplicitHost = other.UseImplicitHost;
        this->Extensions = other.Extensions;
        this->LoadMetrics = other.LoadMetrics;
        this->ServicePlacementPolicies = other.ServicePlacementPolicies;
    }

    return *this;
}

ServiceTypeDescription const & ServiceTypeDescription::operator = (ServiceTypeDescription && other)
{
    if (this != & other)
    {
        this->IsStateful = other.IsStateful;
        this->ServiceTypeName = move(other.ServiceTypeName);
        this->PlacementConstraints = move(other.PlacementConstraints);
        this->HasPersistedState = other.HasPersistedState;
        this->UseImplicitHost = other.UseImplicitHost;
        this->Extensions = move(other.Extensions);
        this->LoadMetrics = move(other.LoadMetrics);
        this->ServicePlacementPolicies = move(other.ServicePlacementPolicies);
    }

    return *this;
}

bool ServiceTypeDescription::operator == (ServiceTypeDescription const & other) const
{
    bool equals = true;

    equals = this->IsStateful == other.IsStateful;
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->ServiceTypeName, other.ServiceTypeName);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->PlacementConstraints, other.PlacementConstraints);
    if (!equals) { return equals; }

    if(this->IsStateful)
    {
        equals = this->HasPersistedState == other.HasPersistedState;
        if (!equals) { return equals; }
    }

    equals = this->UseImplicitHost == other.UseImplicitHost;
    if (!equals) { return equals; }

    equals = this->LoadMetrics.size() == other.LoadMetrics.size();
    if (!equals) { return equals; }

    equals = this->Extensions.size() == other.Extensions.size();
    if (!equals) { return equals; }

    for (size_t ix = 0; ix < this->Extensions.size(); ++ix)
    {
        auto it = find(other.Extensions.begin(), other.Extensions.end(), this->Extensions[ix]);
        equals = it != other.Extensions.end();
        if (!equals) { return equals; }
    }

    for (size_t ix = 0; ix < this->LoadMetrics.size(); ++ix)
    {
        auto it = find(other.LoadMetrics.begin(), other.LoadMetrics.end(), this->LoadMetrics[ix]);
        equals = it != other.LoadMetrics.end();
        if (!equals) { return equals; }
    }

    for (size_t ix = 0; ix < this->ServicePlacementPolicies.size(); ++ix)
    {
        auto it = find(other.ServicePlacementPolicies.begin(), other.ServicePlacementPolicies.end(), this->ServicePlacementPolicies[ix]);
        equals = it != other.ServicePlacementPolicies.end();
        if (!equals) { return equals; }
    }

    return equals;
}

bool ServiceTypeDescription::operator != (ServiceTypeDescription const & other) const
{
    return !(*this == other);
}

void ServiceTypeDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ServiceTypeDescription { ");
    w.Write("IsStateful = {0}, ", IsStateful);
    w.Write("ServiceTypeName = {0}", ServiceTypeName);
    w.Write("PlacementConstraints = {0}", PlacementConstraints);
    w.Write("HasPersistedState = {0}", HasPersistedState);
    w.Write("UseImplicitHost = {0}", UseImplicitHost);

    w.Write("Extensions = {");
    for(auto iter = Extensions.cbegin(); iter != Extensions.cend(); ++ iter)
    {
        w.Write("{0},",*iter);
    }
    w.Write("}");

    w.Write("LoadMetrics = {");
    for(auto iter = LoadMetrics.cbegin(); iter != LoadMetrics.cend(); ++ iter)
    {
        w.Write("{0},",*iter);
    }
    w.Write("}");

    for(auto iter = ServicePlacementPolicies.cbegin(); iter != ServicePlacementPolicies.cend(); ++ iter)
    {
        w.Write("{0},",*iter);
    }
    w.Write("}");

    w.Write("}");
}

void ServiceTypeDescription::ReadFromXml(Common::XmlReaderUPtr const & xmlReader, bool isStateful)
{
    this->clear();

    this->IsStateful = isStateful;

    if(this->IsStateful)
    {
        // ensure that we are on <StatefulServiceType
        xmlReader->StartElement(
            *SchemaNames::Element_StatefulServiceType,
            *SchemaNames::Namespace);
        if (xmlReader->HasAttribute(*SchemaNames::Attribute_HasPersistedState))
        {
            wstring attrValue = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_HasPersistedState);
            if (!StringUtility::TryFromWString<bool>(
                attrValue,
                this->HasPersistedState))
            {
                Parser::ThrowInvalidContent(xmlReader, L"true/false", attrValue);
            }
        }

        if (xmlReader->HasAttribute(*SchemaNames::Attribute_UseImplicitHost))
        {
            wstring attrValue = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_UseImplicitHost);
            if (!StringUtility::TryFromWString<bool>(
                attrValue,
                this->UseImplicitHost))
            {
                Parser::ThrowInvalidContent(xmlReader, L"true/false", attrValue);
            }
        }

    }
    else
    {
        // ensure that we are on <StatelessServiceType
        xmlReader->StartElement(
            *SchemaNames::Element_StatelessServiceType,
            *SchemaNames::Namespace);

        if (xmlReader->HasAttribute(*SchemaNames::Attribute_UseImplicitHost))
        {
            wstring attrValue = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_UseImplicitHost);
            if (!StringUtility::TryFromWString<bool>(
                attrValue, 
                this->UseImplicitHost))
            {
                Parser::ThrowInvalidContent(xmlReader, L"true/false", attrValue);
            }
        }
    }

    this->ServiceTypeName = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ServiceTypeName);

    if (xmlReader->IsEmptyElement())
    {
        // <Stateful/StatelessServiceType .. />
        xmlReader->ReadElement();
    }
    else
    {
        xmlReader->ReadStartElement();

        // <LoadMetrics ..>
        ParseLoadBalancingMetics(xmlReader);

        // <PlacementConstraints ..>
        ParsePlacementConstraints(xmlReader);

        // <ServicePlacementPolicies ..>
        ParseServicePlacementPolicies(xmlReader);

        // <Extensions ..>
        ParseExtensions(xmlReader);

        // </Stateful/StatelessServiceType>
        xmlReader->ReadEndElement();
    }

    //TODO: Reading the description extensions from the xml is pending
}

void ServiceTypeDescription::ParsePlacementConstraints(Common::XmlReaderUPtr const & xmlReader)
{
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_PlacementConstraints,
        *SchemaNames::Namespace,
        false))
    {
        if (xmlReader->IsEmptyElement())
        {
            xmlReader->ReadElement();
        }
        else
        {
            xmlReader->ReadStartElement();
            this->PlacementConstraints = xmlReader->ReadElementValue(true);
            xmlReader->ReadEndElement();
        }
    }
}

void ServiceTypeDescription::ParseExtensions(Common::XmlReaderUPtr const & xmlReader)
{
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_Extensions,
        *SchemaNames::Namespace,
        false))
    {
        if (xmlReader->IsEmptyElement())
        {
            // <Extensions />
            xmlReader->ReadElement();
        }
        else
        {
            xmlReader->ReadStartElement();

            bool done = false;
            while(!done)
            {
                if (xmlReader->IsStartElement(
                    *SchemaNames::Element_Extension,
                    *SchemaNames::Namespace))
                {
                    DescriptionExtension descriptionExtension;
                    descriptionExtension.ReadFromXml(xmlReader);
                    this->Extensions.push_back(descriptionExtension);
                }
                else
                {
                    done = true;
                }
            }

            xmlReader->ReadEndElement();
        }
    }
}

void ServiceTypeDescription::ParseLoadBalancingMetics(Common::XmlReaderUPtr const & xmlReader)
{
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_LoadMetrics,
        *SchemaNames::Namespace,
        false))
    {
        xmlReader->ReadStartElement();

        bool done = false;
        while(!done)
        {
            if (xmlReader->IsStartElement(
                *SchemaNames::Element_LoadMetric,
                *SchemaNames::Namespace))
            {
                ServiceLoadMetricDescription serviceLoadMetricDescription;
                serviceLoadMetricDescription.ReadFromXml(xmlReader);
                this->LoadMetrics.push_back(serviceLoadMetricDescription);
            }
            else
            {
                done = true;
            }
        }

        xmlReader->ReadEndElement();
    }
}

void ServiceTypeDescription::ParseServicePlacementPolicies(Common::XmlReaderUPtr const & xmlReader)
{
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_ServicePlacementPolicies,
        *SchemaNames::Namespace,
        false))
    {
        xmlReader->ReadStartElement();

        bool done = false;
        while(!done)
        {
            if (xmlReader->IsStartElement(
                *SchemaNames::Element_ServicePlacementPolicy,
                *SchemaNames::Namespace))
            {
                ServicePlacementPolicyDescription placementPolicyDescription;
                placementPolicyDescription.ReadFromXml(xmlReader);
                this->ServicePlacementPolicies.push_back(placementPolicyDescription);
            }
            else
            {
                done = true;
            }
        }

        xmlReader->ReadEndElement();
    }
}


ErrorCode ServiceTypeDescription::WriteToXml(XmlWriterUPtr const & xmlWriter){
	wstring type = *(this->IsStateful ? SchemaNames::Element_StatefulServiceType : SchemaNames::Element_StatelessServiceType);
	// <StatefulServiceType or stateless>
	ErrorCode er = xmlWriter->WriteStartElement(type, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_ServiceTypeName, this->ServiceTypeName);
	if (!er.IsSuccess())
	{
		return er;
	}
	if (this->IsStateful){
		er = xmlWriter->WriteBooleanAttribute(*SchemaNames::Attribute_HasPersistedState,
			this->HasPersistedState);
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	else 
	{
		er = xmlWriter->WriteBooleanAttribute(*SchemaNames::Attribute_UseImplicitHost,
			this->UseImplicitHost);
		if (!er.IsSuccess())
		{
			return er;
		}
	}

	er = WriteLoadBalancingMetics(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = WritePlacementConstraints(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
  	er = WriteServicePlacementPolicies(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}

	er = WriteExtensions(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	//</Stateful/lessServiceType>
	return xmlWriter->WriteEndElement();
}

Common::ErrorCode ServiceTypeDescription::WriteLoadBalancingMetics(Common::XmlWriterUPtr const & xmlWriter)
{
	if (LoadMetrics.size() == 0)
	{
		return ErrorCode::Success();
	}
	//<Loadbalancing metrics
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_LoadMetrics, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	for (vector<ServiceLoadMetricDescription>::iterator it = this->LoadMetrics.begin(); it != LoadMetrics.end(); ++it){
		er = (*it).WriteToXml(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	//</Loadbalancingmetrics>
	return xmlWriter->WriteEndElement();
}

Common::ErrorCode ServiceTypeDescription::WritePlacementConstraints(Common::XmlWriterUPtr const & xmlWriter)
{
	if (this->PlacementConstraints.empty())
	{
		return ErrorCodeValue::Success;
	}
	return xmlWriter->WriteElementWithContent(*SchemaNames::Element_PlacementConstraints, PlacementConstraints, L"", *SchemaNames::Namespace);
}

Common::ErrorCode ServiceTypeDescription::WriteExtensions(Common::XmlWriterUPtr const & xmlWriter)
{
	if (Extensions.size() == 0)
	{
		return ErrorCode::Success();
	}
	//<Extensions>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_Extensions, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	for (vector<DescriptionExtension>::iterator it = this->Extensions.begin(); it != Extensions.end(); ++it){
		er = (*it).WriteToXml(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	//</Extensions>
	return xmlWriter->WriteEndElement();

}

Common::ErrorCode ServiceTypeDescription::WriteServicePlacementPolicies(Common::XmlWriterUPtr const & xmlWriter)
{
	if (ServicePlacementPolicies.empty())
	{
		return ErrorCode::Success();
	}
	//<ServicePlacementPolicies>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ServicePlacementPolicies, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	for (vector<ServicePlacementPolicyDescription>::iterator it = this->ServicePlacementPolicies.begin(); it != ServicePlacementPolicies.end(); ++it)
	{
		er = it->WriteToXml(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	//</ServicePlacementPolicies>
	return xmlWriter->WriteEndElement();
}



void ServiceTypeDescription::ToPublicApi(
    __in ScopedHeap & heap, 
    __in vector<ServiceTypeDescription> const & serviceTypes,
    __out FABRIC_SERVICE_TYPE_DESCRIPTION_LIST & publicServiceTypes)
{
    auto serviceTypeNamesCount = serviceTypes.size();
    publicServiceTypes.Count = static_cast<ULONG>(serviceTypeNamesCount);

    if (serviceTypeNamesCount > 0)
    {
        auto items = heap.AddArray<FABRIC_SERVICE_TYPE_DESCRIPTION>(serviceTypeNamesCount);

        for (size_t i = 0; i < items.GetCount(); ++i)
        {
            serviceTypes[i].ToPublicApi(heap, items[i]);
        }

        publicServiceTypes.Items = items.GetRawArray();
    }
    else
    {
        publicServiceTypes.Items = NULL;
    }
}

void ServiceTypeDescription::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_SERVICE_TYPE_DESCRIPTION & publicDescription) const
{
    auto loadMetricsList = heap.AddItem<FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION_LIST>();
    ULONG const loadMetricsSize = static_cast<const ULONG>(LoadMetrics.size());
    auto loadMetrics = heap.AddArray<FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION>(loadMetricsSize);

    for (size_t i = 0; i < LoadMetrics.size(); i++)
    {
        LoadMetrics[i].ToPublicApi(heap, loadMetrics[i]);
    }

    loadMetricsList->Count = loadMetricsSize;
    loadMetricsList->Items = loadMetrics.GetRawArray();

    auto extensionsList = heap.AddItem<FABRIC_SERVICE_TYPE_DESCRIPTION_EXTENSION_LIST>();
    ULONG const extensionsListSize = static_cast<const ULONG>(Extensions.size());
    auto extensions = heap.AddArray<FABRIC_SERVICE_TYPE_DESCRIPTION_EXTENSION>(extensionsListSize);

    for (size_t i = 0; i < Extensions.size(); i++)
    {
        Extensions[i].ToPublicApi(heap, extensions[i]);
    }

    extensionsList->Count = extensionsListSize;
    extensionsList->Items = extensions.GetRawArray();

    size_t policyCount = ServicePlacementPolicies.size();
    Common::ReferencePointer<FABRIC_SERVICE_PLACEMENT_POLICY_LIST> policyDescription;
    if (policyCount > 0)
    {
        policyDescription = heap.AddItem<FABRIC_SERVICE_PLACEMENT_POLICY_LIST>();
        policyDescription->PolicyCount = static_cast<ULONG>(policyCount);

        auto policies = heap.AddArray<FABRIC_SERVICE_PLACEMENT_POLICY_DESCRIPTION>(policyCount);
        policyDescription->Policies = policies.GetRawArray();
        for (size_t policyIndex = 0; policyIndex < policyCount; ++policyIndex)
        {
            ServicePlacementPolicies[policyIndex].ToPublicApi(heap, policies[policyIndex]);
        }
    }

    if (IsStateful)
    {
        auto statefulDescription = heap.AddItem<FABRIC_STATEFUL_SERVICE_TYPE_DESCRIPTION>();
        publicDescription.Kind = FABRIC_SERVICE_KIND_STATEFUL;
        publicDescription.Value = statefulDescription.GetRawPointer();
        statefulDescription->ServiceTypeName = heap.AddString(this->ServiceTypeName);
        statefulDescription->PlacementConstraints = heap.AddString(this->PlacementConstraints);
        statefulDescription->HasPersistedState = HasPersistedState;
        statefulDescription->LoadMetrics = loadMetricsList.GetRawPointer();
        statefulDescription->Extensions = extensionsList.GetRawPointer();

	    auto statefulDescriptionEx1 = heap.AddItem<FABRIC_STATEFUL_SERVICE_TYPE_DESCRIPTION_EX1>();
  	    statefulDescription->Reserved = statefulDescriptionEx1.GetRawPointer();
        if (policyCount > 0)
        {
            statefulDescriptionEx1->PolicyList = policyDescription.GetRawPointer();
        }
        else
        {
            statefulDescriptionEx1->PolicyList = nullptr;
        }
    }
    else
    {
        auto statelessDescription = heap.AddItem<FABRIC_STATELESS_SERVICE_TYPE_DESCRIPTION>();
        publicDescription.Kind = FABRIC_SERVICE_KIND_STATELESS;
        publicDescription.Value = statelessDescription.GetRawPointer();
        statelessDescription->ServiceTypeName = heap.AddString(this->ServiceTypeName);
        statelessDescription->PlacementConstraints = heap.AddString(this->PlacementConstraints);
        statelessDescription->LoadMetrics = loadMetricsList.GetRawPointer();
        statelessDescription->Extensions = extensionsList.GetRawPointer();
        statelessDescription->UseImplicitHost = UseImplicitHost;

	    auto statelessDescriptionEx1 = heap.AddItem<FABRIC_STATELESS_SERVICE_TYPE_DESCRIPTION_EX1>();
  	    statelessDescription->Reserved = statelessDescriptionEx1.GetRawPointer();
        if (policyCount > 0)
        {
            statelessDescriptionEx1->PolicyList = policyDescription.GetRawPointer();
        }
        else
        {
            statelessDescriptionEx1->PolicyList = nullptr;
        }
    }
}

Common::ErrorCode ServiceTypeDescription::FromPublicApi(__in FABRIC_SERVICE_TYPE_DESCRIPTION const & publicDescription)
{
    ErrorCode error(ErrorCodeValue::Success);
    IsStateful = (publicDescription.Kind == FABRIC_SERVICE_KIND_STATEFUL);

    if(IsStateful)
    {
        auto publicStatefulServiceTypeDescription = reinterpret_cast<FABRIC_STATEFUL_SERVICE_TYPE_DESCRIPTION *>(publicDescription.Value);
        ServiceTypeName = publicStatefulServiceTypeDescription->ServiceTypeName;
        PlacementConstraints = publicStatefulServiceTypeDescription->PlacementConstraints;
        HasPersistedState = publicStatefulServiceTypeDescription->HasPersistedState ? true : false;

        this->Extensions = vector<DescriptionExtension>(publicStatefulServiceTypeDescription->Extensions->Count);
        for (ULONG i = 0; i < publicStatefulServiceTypeDescription->Extensions->Count; ++i)
        {
            FABRIC_SERVICE_TYPE_DESCRIPTION_EXTENSION *publicServiceTypeDescriptionExtension =
                const_cast<FABRIC_SERVICE_TYPE_DESCRIPTION_EXTENSION *>(&publicStatefulServiceTypeDescription->Extensions->Items[i]);

            error = this->Extensions[i].FromPublicApi(*publicServiceTypeDescriptionExtension);
            if(!error.IsSuccess())
            {
                return error;
            }
        }

        this->LoadMetrics = vector<ServiceLoadMetricDescription>(publicStatefulServiceTypeDescription->LoadMetrics->Count);
        for (ULONG i = 0; i < publicStatefulServiceTypeDescription->LoadMetrics->Count; ++i)
        {
            FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION *publicServiceTypeDescriptionLoadMetric =
                const_cast<FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION *>(&publicStatefulServiceTypeDescription->LoadMetrics->Items[i]);

            error = this->LoadMetrics[i].FromPublicApi(*publicServiceTypeDescriptionLoadMetric);
            if(!error.IsSuccess())
            {
                return error;
            }
        }

        if (publicStatefulServiceTypeDescription->Reserved != NULL)
        {
            auto statefulEx1 = reinterpret_cast<FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX1*>(publicStatefulServiceTypeDescription->Reserved);
            if (statefulEx1->PolicyList != NULL)
            {
                auto pList = statefulEx1->PolicyList;
                for(ULONG i = 0; i < pList->PolicyCount; i++)
                {
                    std::wstring domainName;
                    FABRIC_SERVICE_PLACEMENT_POLICY_DESCRIPTION & policyDesc = pList->Policies[i];
                    ServicePlacementPolicyHelper::PolicyDescriptionToDomainName(policyDesc, domainName);
                    ServicePlacementPolicies.push_back(ServiceModel::ServicePlacementPolicyDescription(move(domainName), policyDesc.Type));
                }
            }
        }
    }
    else
    {
        auto publicStatelessServiceTypeDescription = reinterpret_cast<FABRIC_STATELESS_SERVICE_TYPE_DESCRIPTION *>(publicDescription.Value);
        this->ServiceTypeName = publicStatelessServiceTypeDescription->ServiceTypeName;
        this->PlacementConstraints = publicStatelessServiceTypeDescription->PlacementConstraints;
        this->UseImplicitHost = publicStatelessServiceTypeDescription->UseImplicitHost ? true : false;

        this->Extensions = vector<DescriptionExtension>(publicStatelessServiceTypeDescription->Extensions->Count);
        for (ULONG i = 0; i < publicStatelessServiceTypeDescription->Extensions->Count; ++i)
        {
            FABRIC_SERVICE_TYPE_DESCRIPTION_EXTENSION *publicServiceTypeDescriptionExtension =
                const_cast<FABRIC_SERVICE_TYPE_DESCRIPTION_EXTENSION *>(&publicStatelessServiceTypeDescription->Extensions->Items[i]);

            error = this->Extensions[i].FromPublicApi(*publicServiceTypeDescriptionExtension);
            if(!error.IsSuccess())
            {
                return error;
            }
        }

        this->LoadMetrics = vector<ServiceLoadMetricDescription>(publicStatelessServiceTypeDescription->LoadMetrics->Count);
        for (ULONG i = 0; i < publicStatelessServiceTypeDescription->LoadMetrics->Count; ++i)
        {
            FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION *publicServiceTypeDescriptionLoadMetric =
                const_cast<FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION *>(&publicStatelessServiceTypeDescription->LoadMetrics->Items[i]);

            error = this->LoadMetrics[i].FromPublicApi(*publicServiceTypeDescriptionLoadMetric);
            if(!error.IsSuccess())
            {
                return error;
            }
        }

        if (publicStatelessServiceTypeDescription->Reserved != NULL)
        {
            auto statelessEx1 = reinterpret_cast<FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX1*>(publicStatelessServiceTypeDescription->Reserved);
            if (statelessEx1->PolicyList != NULL)
            {
                auto pList = statelessEx1->PolicyList;
                for(ULONG i = 0; i < pList->PolicyCount; i++)
                {
                    std::wstring domainName;
                    FABRIC_SERVICE_PLACEMENT_POLICY_DESCRIPTION & policyDesc = pList->Policies[i];
                    ServicePlacementPolicyHelper::PolicyDescriptionToDomainName(policyDesc, domainName);
                    ServicePlacementPolicies.push_back(ServiceModel::ServicePlacementPolicyDescription(move(domainName), policyDesc.Type));
                }
            }
        }

    }

    return error;
}

void ServiceTypeDescription::clear()
{
    this->ServiceTypeName.clear();
    this->PlacementConstraints.clear();
    this->ServicePlacementPolicies.clear();
}

wstring ServiceTypeDescription::ToString() const
{
    wstring serviceKind;
    serviceKind = (this->IsStateful ? L"Stateful" : L"Stateless");

    wstring extensions = L"";
    for (auto it = this->Extensions.begin();
        it != this->Extensions.end();
        ++it)
    {
        extensions.append(it->ToString());
        extensions.append(L":");
    }

    wstring loadMetrics = L"";
    for (auto it = this->LoadMetrics.begin();
        it != this->LoadMetrics.end();
        ++it)
    {
        loadMetrics.append(it->ToString());
        loadMetrics.append(L":");
    }

    wstring policies = L"";
    for (auto it = this->ServicePlacementPolicies.begin();
        it != this->ServicePlacementPolicies.end();
        ++it)
    {
        policies.append(it->ToString());
        policies.append(L":");
    }

   
    if(IsStateful)
    {
        return wformatString("ServiceKind = '{0}'; ServiceTypeName = '{1}'; PlacementConstraints = '{2}'; HasPeristedState = '{3}'; Extensions = '{4}'; LoadMetrics = '{5}'; PlacementPolicies = '{6}'", 
            serviceKind, 
            this->ServiceTypeName, 
            this->PlacementConstraints,
            this->HasPersistedState ? "true" : "false",
            extensions,
            loadMetrics,
            policies);
    }
    else
    {
        return wformatString("ServiceKind = '{0}'; ServiceTypeName = '{1}'; PlacementConstraints = '{2}'; UseImplicitHost = '{3}'; Extensions = '{4}'; LoadMetrics = '{5}'; PlacementPolicies = '{6}'", 
            serviceKind, 
            this->ServiceTypeName, 
            this->PlacementConstraints,
            this->UseImplicitHost ? "true" : "false",
            extensions,
            loadMetrics,
            policies);
    }
}
