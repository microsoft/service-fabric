// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ServiceGroupTypeMemberDescription::ServiceGroupTypeMemberDescription() :
    ServiceTypeName(),
    LoadMetrics()
{
}

ServiceGroupTypeMemberDescription::ServiceGroupTypeMemberDescription(ServiceGroupTypeMemberDescription const & other) :
    ServiceTypeName(other.ServiceTypeName),
    LoadMetrics(other.LoadMetrics)
{
}

ServiceGroupTypeMemberDescription::ServiceGroupTypeMemberDescription(ServiceGroupTypeMemberDescription && other) :
    ServiceTypeName(move(other.ServiceTypeName)),
    LoadMetrics(move(other.LoadMetrics))
{
}

ServiceGroupTypeMemberDescription const & ServiceGroupTypeMemberDescription::operator = (ServiceGroupTypeMemberDescription const & other)
{
    if (this != & other)
    {
        this->ServiceTypeName = other.ServiceTypeName;
        this->LoadMetrics = other.LoadMetrics;
    }

    return *this;
}

ServiceGroupTypeMemberDescription const & ServiceGroupTypeMemberDescription::operator = (ServiceGroupTypeMemberDescription && other)
{
    if (this != & other)
    {
        this->ServiceTypeName = move(other.ServiceTypeName);
        this->LoadMetrics = move(other.LoadMetrics);
    }

    return *this;
}

bool ServiceGroupTypeMemberDescription::operator == (ServiceGroupTypeMemberDescription const & other) const
{
    bool equals = true;

    equals = this->ServiceTypeName == other.ServiceTypeName;
    if (!equals) { return equals; }

    for (auto it = begin(this->LoadMetrics); it != end(this->LoadMetrics); ++it)
    {
        auto metric = find(begin(other.LoadMetrics), end(other.LoadMetrics), *it);
        equals = metric != end(other.LoadMetrics);
        if (!equals) { return equals; }
    }

    return equals;
}

bool ServiceGroupTypeMemberDescription::operator != (ServiceGroupTypeMemberDescription const & other) const
{
    return !(*this == other);
}

void ServiceGroupTypeMemberDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ServiceGroupTypeDescription { ");
    w.Write("ServiceTypeName = {0}", ServiceTypeName);
    w.Write("LoadMetrics {");

    for (auto it = begin(LoadMetrics); it != end(LoadMetrics); ++it)
    {
        w.Write("{0}", *it);
    }

    w.Write("}");
    w.Write("}");
}

void ServiceGroupTypeMemberDescription::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_SERVICE_GROUP_TYPE_MEMBER_DESCRIPTION & publicDescription) const
{
    auto name = heap.AddString(ServiceTypeName);

    auto loadMetricsList = heap.AddItem<FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION_LIST>();
    ULONG const loadMetricsSize = static_cast<const ULONG>(LoadMetrics.size());

    loadMetricsList->Count = loadMetricsSize;

    if (loadMetricsSize > 0)
    {
        auto loadMetrics = heap.AddArray<FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION>(loadMetricsSize);
        for (size_t i = 0; i < LoadMetrics.size(); i++)
        {
            LoadMetrics[i].ToPublicApi(heap, loadMetrics[i]);
        }

        loadMetricsList->Items = loadMetrics.GetRawArray();
    }
    else
    {
        loadMetricsList->Items = NULL;
    }

    publicDescription.ServiceTypeName = name;
    publicDescription.LoadMetrics = loadMetricsList.GetRawPointer();
}

void ServiceGroupTypeMemberDescription::ReadFromXml(Common::XmlReaderUPtr const & xmlReader)
{
    this->clear();

    xmlReader->StartElement(
        *SchemaNames::Element_ServiceGroupTypeMember,
        *SchemaNames::Namespace);

    this->ServiceTypeName = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ServiceTypeName);

    if (xmlReader->IsEmptyElement())
    {
        xmlReader->ReadElement();
    }
    else
    {
        xmlReader->ReadStartElement();
        
        ParseLoadBalancingMetrics(xmlReader);

        xmlReader->ReadEndElement();
    }
}

void ServiceGroupTypeMemberDescription::ParseLoadBalancingMetrics(Common::XmlReaderUPtr const & xmlReader)
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
ErrorCode ServiceGroupTypeMemberDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	//<ServiceGroupTypeMember>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ServiceGroupTypeMember, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_ServiceTypeName, this->ServiceTypeName);
	if (!er.IsSuccess())
	{
		return er;
	}
	//Load Metrics
	er = WriteLoadBalancingMetrics(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	//</ServiceGroupTypeMember>
	return xmlWriter->WriteEndElement();
}

ErrorCode ServiceGroupTypeMemberDescription::WriteLoadBalancingMetrics(XmlWriterUPtr const & xmlWriter)
{
	if (this->LoadMetrics.size() == 0)
	{
		return ErrorCode::Success();
	}
	//<Loadbalancing metrics
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_LoadMetrics, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	for (vector<ServiceLoadMetricDescription>::iterator it = this->LoadMetrics.begin(); it != this->LoadMetrics.end(); ++it){
		er = (*it).WriteToXml(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	//</Loadbalancingmetrics>
	return xmlWriter->WriteEndElement();
}
void ServiceGroupTypeMemberDescription::clear()
{
    this->ServiceTypeName.clear();
    this->LoadMetrics.clear();
}

ServiceGroupTypeDescription::ServiceGroupTypeDescription() :
    Description(),
    Members(),
    UseImplicitFactory(false)
{
}

ServiceGroupTypeDescription::ServiceGroupTypeDescription(ServiceGroupTypeDescription const & other) :
    Description(other.Description),
    Members(other.Members),
    UseImplicitFactory(other.UseImplicitFactory)
{
}

ServiceGroupTypeDescription::ServiceGroupTypeDescription(ServiceGroupTypeDescription && other) :
    Description(move(other.Description)),
    Members(move(other.Members)),
    UseImplicitFactory(other.UseImplicitFactory)
{
}

ServiceGroupTypeDescription const & ServiceGroupTypeDescription::operator = (ServiceGroupTypeDescription const & other)
{
    if (this != & other)
    {
        this->Description = other.Description;
        this->Members = other.Members;
        this->UseImplicitFactory = other.UseImplicitFactory;
    }

    return *this;
}

ServiceGroupTypeDescription const & ServiceGroupTypeDescription::operator = (ServiceGroupTypeDescription && other)
{
    if (this != & other)
    {
        this->Description = move(other.Description);
        this->Members = move(other.Members);
        this->UseImplicitFactory = other.UseImplicitFactory;
    }

    return *this;
}

bool ServiceGroupTypeDescription::operator == (ServiceGroupTypeDescription const & other) const
{
    bool equals = true;

    equals = this->Description == other.Description;
    if (!equals) { return equals; }

    equals = this->UseImplicitFactory == other.UseImplicitFactory;
    if (!equals) { return equals; }

    equals = this->Members.size() == other.Members.size();
    if (!equals) { return equals; }

    for (auto it = begin(this->Members); it != end(this->Members); ++it)
    {
        auto member = find(begin(other.Members), end(other.Members), *it);
        equals = member != end(other.Members);
        if (!equals) { return equals; }
    }

    return equals;
}

bool ServiceGroupTypeDescription::operator != (ServiceGroupTypeDescription const & other) const
{
    return !(*this == other);
}

void ServiceGroupTypeDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ServiceGroupTypeDescription { ");
    w.Write("Description = {0}", Description);
    w.Write("Members = {");
    for(auto it = begin(Members); it != end(Members); ++it)
    {
        w.Write("{0},",*it);
    }
    w.Write("}");
    w.Write("}");
}

void ServiceGroupTypeDescription::ReadFromXml(Common::XmlReaderUPtr const & xmlReader, bool isStateful)
{
    this->clear();

    if(isStateful)
    {
        // ensure that we are on <StatefulServiceGroupType
        xmlReader->StartElement(
            *SchemaNames::Element_StatefulServiceGroupType,
            *SchemaNames::Namespace);
        if (xmlReader->HasAttribute(*SchemaNames::Attribute_HasPersistedState))
        {
            wstring attrValue = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_HasPersistedState);
            if (!StringUtility::TryFromWString<bool>(
                attrValue, 
                this->Description.HasPersistedState))
            {
                Parser::ThrowInvalidContent(xmlReader, L"true/false", attrValue);
            }
        }
    }
    else
    {
        // ensure that we are on <StatelessServiceGroupType
        xmlReader->StartElement(
            *SchemaNames::Element_StatelessServiceGroupType,
            *SchemaNames::Namespace);
    }

    this->Description.ServiceTypeName = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ServiceGroupTypeName);
    this->Description.IsStateful = isStateful;

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_UseImplicitFactory))
    {
        wstring attrValue = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_UseImplicitFactory);
        if (!StringUtility::TryFromWString<bool>(
            attrValue, 
            this->UseImplicitFactory))
        {
            Parser::ThrowInvalidContent(xmlReader, L"true/false", attrValue);
        }
    }

    if (xmlReader->IsEmptyElement())
    {
        // <Stateful/StatelessServiceType .. />
        xmlReader->ReadElement();
    }
    else
    {
        xmlReader->ReadStartElement();

        // <LoadMetrics ..>
        ParseLoadBalancingMetrics(xmlReader);

        // <PlacementConstraints ..>
        ParsePlacementConstraints(xmlReader);

        // <ServiceGroupMembers ..>
        ParseServiceGroupMembers(xmlReader);

        // <Extensions ..>
        ParseExtensions(xmlReader);

        xmlReader->ReadEndElement();
    }

    CombineLoadBalancingMetrics(xmlReader);

    //TODO: Reading the description extensions from the xml is pending
}

void ServiceGroupTypeDescription::ParseLoadBalancingMetrics(Common::XmlReaderUPtr const & xmlReader)
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
                this->Description.LoadMetrics.push_back(serviceLoadMetricDescription);
            }
            else
            {
                done = true;
            }
        }

        xmlReader->ReadEndElement();
    }
}

void ServiceGroupTypeDescription::ParsePlacementConstraints(Common::XmlReaderUPtr const & xmlReader)
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

            this->Description.PlacementConstraints = xmlReader->ReadElementValue(true);
            
            xmlReader->ReadEndElement();
        }
    }
}

void ServiceGroupTypeDescription::ParseExtensions(Common::XmlReaderUPtr const & xmlReader)
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
                    this->Description.Extensions.push_back(descriptionExtension);
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

void ServiceGroupTypeDescription::ParseServiceGroupMembers(Common::XmlReaderUPtr const & xmlReader)
{
    xmlReader->StartElement(
        *SchemaNames::Element_ServiceGroupMembers,
        *SchemaNames::Namespace,
        false);

    xmlReader->ReadStartElement();

    bool done = false;
    while(!done)
    {
        if (xmlReader->IsStartElement(
            *SchemaNames::Element_ServiceGroupTypeMember,
            *SchemaNames::Namespace,
            false))
        {
            ServiceGroupTypeMemberDescription member;
            member.ReadFromXml(xmlReader);
            this->Members.push_back(move(member));
        }
        else
        {
            done = true;
        }
    }

    xmlReader->ReadEndElement();
}

ErrorCode ServiceGroupTypeDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{

	ErrorCode er;
	if (this->Description.IsStateful)
	{
		er = xmlWriter->WriteStartElement(*SchemaNames::Element_StatefulServiceGroupType, L"", *SchemaNames::Namespace);

		if (!er.IsSuccess())
		{
			return er;
		}
		er = xmlWriter->WriteBooleanAttribute(*SchemaNames::Attribute_HasPersistedState,
			this->Description.HasPersistedState);
	}
	else
	{
		er = xmlWriter->WriteStartElement(*SchemaNames::Element_StatelessServiceGroupType, L"", *SchemaNames::Namespace);
	}
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_ServiceGroupTypeName, this->Description.ServiceTypeName);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteBooleanAttribute(*SchemaNames::Attribute_UseImplicitFactory,
		this->UseImplicitFactory);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = WriteLoadBalancingMetrics(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = WritePlacementConstraints(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = WriteServiceGroupMembers(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = WriteExtensions(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	//Close <StatefulServiceGroupType>
	return xmlWriter->WriteEndElement();
}

Common::ErrorCode ServiceGroupTypeDescription::WriteLoadBalancingMetrics(Common::XmlWriterUPtr const & xmlWriter)
{
	if (this->Description.LoadMetrics.size() == 0)
	{
		return ErrorCode::Success();
	}
	//<Loadbalancing metrics
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_LoadMetrics, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	for (vector<ServiceLoadMetricDescription>::iterator it = this->Description.LoadMetrics.begin(); it != this->Description.LoadMetrics.end(); ++it){
		er = (*it).WriteToXml(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	//</Loadbalancingmetrics>
	return xmlWriter->WriteEndElement();
}

Common::ErrorCode ServiceGroupTypeDescription::WritePlacementConstraints(Common::XmlWriterUPtr const & xmlWriter)
{
	if (this->Description.PlacementConstraints.empty()){
		return ErrorCodeValue::Success;
	}
	return xmlWriter->WriteElementWithContent(*SchemaNames::Element_PlacementConstraints, 
		this->Description.PlacementConstraints, L"", *SchemaNames::Namespace);
}

Common::ErrorCode ServiceGroupTypeDescription::WriteExtensions(Common::XmlWriterUPtr const & xmlWriter)
{
	if (this->Description.Extensions.size() == 0)
	{
		return ErrorCode::Success();
	}
	//<Extensions>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_Extensions, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	for (vector<DescriptionExtension>::iterator it = this->Description.Extensions.begin(); it != this->Description.Extensions.end(); ++it){
		er = (*it).WriteToXml(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	//</Extensions>
	return xmlWriter->WriteEndElement();

}

Common::ErrorCode ServiceGroupTypeDescription::WriteServiceGroupMembers(Common::XmlWriterUPtr const & xmlWriter)
{
	if (this->Members.size() == 0)
	{
		return ErrorCode::Success();
	}
	//<ServiceGroupMembers>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ServiceGroupMembers, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	for (vector<ServiceGroupTypeMemberDescription>::iterator it = this->Members.begin(); it != Members.end(); ++it)
	{
		er = (*it).WriteToXml(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	//</ServiceGroupMembers>
	return xmlWriter->WriteEndElement();
}


void ServiceGroupTypeDescription::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __in std::vector<ServiceGroupTypeDescription> const & serviceGroupTypes,
    __out FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION_LIST & publicServiceGroupTypes)
{
    auto serviceGroupCount = serviceGroupTypes.size();
    publicServiceGroupTypes.Count = static_cast<ULONG>(serviceGroupCount);

    if (serviceGroupCount > 0)
    {
        auto items = heap.AddArray<FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION>(serviceGroupCount);

        for (size_t i = 0; i < items.GetCount(); ++i)
        {
            serviceGroupTypes[i].ToPublicApi(heap, items[i]);
        }

        publicServiceGroupTypes.Items = items.GetRawArray();
    }
    else
    {
        publicServiceGroupTypes.Items = nullptr;
    }
}

void ServiceGroupTypeDescription::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION & publicDescription) const
{
    auto description = heap.AddItem<FABRIC_SERVICE_TYPE_DESCRIPTION>();

    Description.ToPublicApi(heap, *description);

    auto memberList = heap.AddItem<FABRIC_SERVICE_GROUP_TYPE_MEMBER_DESCRIPTION_LIST>();
    ULONG const memberListSize = static_cast<const ULONG>(Members.size());
    memberList->Count = memberListSize;

    if (memberListSize > 0)
    {
        auto members = heap.AddArray<FABRIC_SERVICE_GROUP_TYPE_MEMBER_DESCRIPTION>(memberListSize);
        for (size_t i = 0; i < Members.size(); i++)
        {
            Members[i].ToPublicApi(heap, members[i]);
        }

        memberList->Items = members.GetRawArray();
    }
    else
    {
        memberList->Items = NULL;
    }

    publicDescription.UseImplicitFactory = this->UseImplicitFactory;

    publicDescription.Description = description.GetRawPointer();
    publicDescription.Members = memberList.GetRawPointer();
}

void ServiceGroupTypeDescription::CombineLoadBalancingMetrics(Common::XmlReaderUPtr const & xmlReader)
{
    if (this->Description.LoadMetrics.empty())
    {
        map<wstring, pair<uint, uint> > metrics;

        for (auto member = begin(Members); member != end(Members); ++member)
        {
            for (auto metric = begin(member->LoadMetrics); metric != end(member->LoadMetrics); ++metric)
            {
                auto current = metrics.find(metric->Name);
                if (current == end(metrics))
                {
                    metrics.insert(make_pair(metric->Name, make_pair(metric->PrimaryDefaultLoad, metric->SecondaryDefaultLoad)));
                }
                else
                {
                    current->second.first = CombineMetric(current->second.first, metric->PrimaryDefaultLoad, xmlReader);
                    current->second.second = CombineMetric(current->second.second, metric->SecondaryDefaultLoad, xmlReader);
                }
            }
        }

        for (auto it = begin(metrics); it != end(metrics); ++it)
        {
            ServiceLoadMetricDescription description;
            description.Name = it->first;
            description.PrimaryDefaultLoad = it->second.first;
            description.SecondaryDefaultLoad = it->second.second;

            this->Description.LoadMetrics.push_back(move(description));
        }
    }

}

uint ServiceGroupTypeDescription::CombineMetric(uint lhs, uint rhs, Common::XmlReaderUPtr const & xmlReader)
{
    if (lhs > ~rhs)
    {
        Parser::ThrowInvalidContent(
            xmlReader, 
            L"uint", 
            wformatString(">= {0}", static_cast<uint64>(lhs) + rhs), 
            L"aggregated default load or move cost would overflow");
    }

    return lhs + rhs;
}

void ServiceGroupTypeDescription::clear()
{
    this->Description.clear();
    this->Members.clear();
}
