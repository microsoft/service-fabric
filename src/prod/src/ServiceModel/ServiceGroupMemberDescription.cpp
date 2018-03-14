// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ServiceGroupMemberDescription::ServiceGroupMemberDescription()
{
}

ServiceGroupMemberDescription::ServiceGroupMemberDescription(ServiceGroupMemberDescription const & other) : 
    ServiceTypeName(other.ServiceTypeName), 
    MemberName(other.MemberName),
    LoadMetrics(other.LoadMetrics)
{
}

ServiceGroupMemberDescription::ServiceGroupMemberDescription(ServiceGroupMemberDescription && other) :
    ServiceTypeName(move(other.ServiceTypeName)), 
    MemberName(move(other.MemberName)),
    LoadMetrics(move(other.LoadMetrics))
{
}

ServiceGroupMemberDescription const & ServiceGroupMemberDescription::operator = (ServiceGroupMemberDescription const & other)
{
    if (this != &other)
    {
        this->ServiceTypeName = other.ServiceTypeName;
        this->MemberName = other.MemberName;
        this->LoadMetrics = other.LoadMetrics;
    }

    return *this;
}

ServiceGroupMemberDescription const & ServiceGroupMemberDescription::operator = (ServiceGroupMemberDescription && other)
{
    if (this != &other)
    {
        this->ServiceTypeName = move(other.ServiceTypeName);
        this->MemberName = move(other.MemberName);
        this->LoadMetrics = move(other.LoadMetrics);
    }

    return *this;
}

bool ServiceGroupMemberDescription::operator == (ServiceGroupMemberDescription const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->ServiceTypeName, other.ServiceTypeName);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->MemberName, other.MemberName);
    if (!equals) { return equals; }

    for (size_t ix = 0; ix < this->LoadMetrics.size(); ++ix)
    {
        auto it = find(other.LoadMetrics.begin(), other.LoadMetrics.end(), this->LoadMetrics[ix]);
        equals = it != other.LoadMetrics.end();
        if (!equals) { return equals; }
    }

    return equals;
}

bool ServiceGroupMemberDescription::operator != (ServiceGroupMemberDescription const & other) const
{
    return !(*this == other);
}

void ServiceGroupMemberDescription::WriteTo(__in TextWriter & w, FormatOptions const &) const
{
    w.Write("ServiceGroupMemberDescription { ");
    w.Write("ServiceTypeName = {0}, ", ServiceTypeName);
    w.Write("MemberName = {0}, ", MemberName);
    
    w.Write("LoadMetrics = {");
    for(auto iter = LoadMetrics.cbegin(); iter != LoadMetrics.cend(); ++ iter)
    {
        w.Write("{0},",*iter);
    }
    w.Write("}");

    w.Write("}");
}

void ServiceGroupMemberDescription::ReadFromXml(XmlReaderUPtr const & xmlReader)
{
    clear();    

    xmlReader->StartElement(
        *SchemaNames::Element_Member,
        *SchemaNames::Namespace);

    this->ServiceTypeName = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ServiceTypeName);
    this->MemberName = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Name);

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

void ServiceGroupMemberDescription::ParseLoadBalancingMetrics(Common::XmlReaderUPtr const & xmlReader)
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

Common::ErrorCode ServiceGroupMemberDescription::WriteToXml(Common::XmlWriterUPtr const & xmlWriter)
{
	//<Member>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_Member, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_ServiceTypeName, this->ServiceTypeName);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Name, this->MemberName);
	if (!er.IsSuccess())
	{
		return er;
	}
	if (!(this->LoadMetrics.empty()))
	{
		er = xmlWriter->WriteStartElement(*SchemaNames::Element_LoadMetrics, L"", *SchemaNames::Namespace);
		if (!er.IsSuccess())
		{
			return er;
		}
		for (auto i = 0; i < LoadMetrics.size(); i++)
		{
			er = LoadMetrics[i].WriteToXml(xmlWriter);
			if (!er.IsSuccess())
			{
				return er;
			}
		}

		er = xmlWriter->WriteEndElement();
		if (!er.IsSuccess())
		{
			return er;
		}
	}

	//</Member>
	return xmlWriter->WriteEndElement();
}

void ServiceGroupMemberDescription::clear()
{
    this->ServiceTypeName.clear();
    this->MemberName.clear();
    this->LoadMetrics.clear();
}
