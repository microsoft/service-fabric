// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ApplicationServiceDescription::ApplicationServiceDescription()
    :
    ServiceTypeName(),
    IsStateful(),
    IsServiceGroup(),
    Partition(),
    InstanceCount(1),
    TargetReplicaSetSize(1),
    MinReplicaSetSize(1),
    ReplicaRestartWaitDuration(TimeSpan::MinValue),
    QuorumLossWaitDuration(TimeSpan::MinValue),
    StandByReplicaKeepDuration(TimeSpan::MinValue),
    PlacementConstraints(),
    IsPlacementConstraintsSpecified(false),
    LoadMetrics(),
    ServiceCorrelations(),
    ServiceGroupMembers(),
    ServicePlacementPolicies(),
    DefaultMoveCost(FABRIC_MOVE_COST_LOW),
    IsDefaultMoveCostSpecified(false)
{
}

bool ApplicationServiceDescription::operator == (ApplicationServiceDescription const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->ServiceTypeName, other.ServiceTypeName);
    if (!equals) { return equals; }

    equals = (this->IsStateful == other.IsStateful);
    if (!equals) { return equals; }

    equals = (this->IsServiceGroup == other.IsServiceGroup);
    if (!equals) { return equals; }

    equals = (this->Partition == other.Partition);
    if (!equals) { return equals; }

    equals = (this->InstanceCount == other.InstanceCount);
    if (!equals) { return equals; }

    equals = (this->TargetReplicaSetSize == other.TargetReplicaSetSize);
    if (!equals) { return equals; }

    equals = (this->MinReplicaSetSize == other.MinReplicaSetSize);
    if (!equals) { return equals; }

    equals = (this->ReplicaRestartWaitDuration == other.ReplicaRestartWaitDuration);
    if (!equals) { return equals; }

    equals = (this->QuorumLossWaitDuration == other.QuorumLossWaitDuration);
    if (!equals) { return equals; }

    equals = (this->StandByReplicaKeepDuration == other.StandByReplicaKeepDuration);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->PlacementConstraints, other.PlacementConstraints);
    if (!equals) { return equals; }

    equals = this->IsPlacementConstraintsSpecified == other.IsPlacementConstraintsSpecified;
    if (!equals) { return equals; }

    equals = (this->DefaultMoveCost == other.DefaultMoveCost);
    if (!equals) { return equals; }

    equals = (this->IsDefaultMoveCostSpecified == other.IsDefaultMoveCostSpecified);
    if (!equals) { return equals; }

    for (size_t ix = 0; ix < this->LoadMetrics.size(); ++ix)
    {
        auto it = find(other.LoadMetrics.begin(), other.LoadMetrics.end(), this->LoadMetrics[ix]);
        equals = it != other.LoadMetrics.end();
        if (!equals) { return equals; }
    }

    for (size_t ix = 0; ix < this->ServiceCorrelations.size(); ++ix)
    {
        auto it = find(other.ServiceCorrelations.begin(), other.ServiceCorrelations.end(), this->ServiceCorrelations[ix]);
        equals = it != other.ServiceCorrelations.end();
        if (!equals) { return equals; }
    }

    for (size_t ix = 0; ix < this->ServiceGroupMembers.size(); ++ix)
    {
        auto it = find(other.ServiceGroupMembers.begin(), other.ServiceGroupMembers.end(), this->ServiceGroupMembers[ix]);
        equals = it != other.ServiceGroupMembers.end();
        if (!equals) { return equals; }
    }

    for (size_t ix = 0; ix < this->ServicePlacementPolicies.size(); ++ix)
    {
        auto it = find(other.ServicePlacementPolicies.begin(), other.ServicePlacementPolicies.end(), this->ServicePlacementPolicies[ix]);
        equals = it != other.ServicePlacementPolicies.end();
        if (!equals) { return equals; }
    }

    if (equals)
    {
        equals = (ScalingPolicies.size() == other.ScalingPolicies.size());
        if (equals)
        {
            for (size_t index = 0; index < ScalingPolicies.size(); ++index)
            {
                auto error = ScalingPolicies[index].Equals(other.ScalingPolicies[index], true);
                if (!error.IsSuccess())
                {
                    return false;
                }
            }
        }
    }

    if (!equals) { return equals; }


    return equals;
}

bool ApplicationServiceDescription::operator != (ApplicationServiceDescription const & other) const
{
    return !(*this == other);
}

void ApplicationServiceDescription::WriteTo(__in TextWriter & w, FormatOptions const &) const
{
    w.Write("ApplicationServiceDescription { ");
    w.Write("ServiceTypeName = {0}, ", ServiceTypeName);
    w.Write("IsStateful = {0}, ", IsStateful);
    w.Write("IsServiceGroup = {0}, ", IsServiceGroup);
    w.Write("Partition = {0}, ", Partition);
    w.Write("InstanceCount = {0}, ", InstanceCount);   
    w.Write("TargetReplicaSetSize = {0}, ", TargetReplicaSetSize);   
    w.Write("MinReplicaSetSize = {0}", MinReplicaSetSize);   
    w.Write("ReplicaRestartWaitDuration = {0}", ReplicaRestartWaitDuration);
    w.Write("QuorumLossWaitDuration = {0}", QuorumLossWaitDuration);
    w.Write("StandByReplicaKeepDuration = {0}", StandByReplicaKeepDuration);
    w.Write("PlacementConstraints = {0}", PlacementConstraints);
    w.Write("IsPlacementConstraintsSpecified = {0}", IsPlacementConstraintsSpecified);
    w.Write("DefaultMoveCost = {0}", DefaultMoveCost);
    w.Write("IsDefaultMoveCostSpecified = {0}", IsDefaultMoveCostSpecified);
    
    w.Write("LoadMetrics = {");
    for(auto iter = LoadMetrics.cbegin(); iter != LoadMetrics.cend(); ++ iter)
    {
        w.Write("{0},",*iter);
    }
    w.Write("}");

    w.Write("ServiceCorrelations = {");
    for(auto iter = ServiceCorrelations.cbegin(); iter != ServiceCorrelations.cend(); ++ iter)
    {
        w.Write("{0},",*iter);
    }
    w.Write("}");

    if (IsServiceGroup)
    {
        w.Write("ServiceGroupMembers = {");
        for(auto iter = ServiceGroupMembers.cbegin(); iter != ServiceGroupMembers.cend(); ++ iter)
        {
            w.Write("{0},",*iter);
        }
        w.Write("}");
    }

    for(auto iter = ServicePlacementPolicies.cbegin(); iter != ServicePlacementPolicies.cend(); ++ iter)
    {
        w.Write("{0},",*iter);
    }
    w.Write("}");
    w.Write("ServiceScalingPolicies = {");
    for (auto & scalingPolicy : ScalingPolicies)
    {
        w.Write("{0},", scalingPolicy);
    }
    w.Write("}");

    w.Write("}");
}

void ApplicationServiceDescription::ReadFromXml(XmlReaderUPtr const & xmlReader, bool isStateful, bool isServiceGroup)
{
    clear();    
    this->IsStateful = isStateful;
	this->IsServiceGroup = isServiceGroup;
    if(isStateful)
    {
        if (isServiceGroup)
        {
            xmlReader->StartElement(
                *SchemaNames::Element_StatefulServiceGroup,
                *SchemaNames::Namespace);
        }
        else
        {
            xmlReader->StartElement(
                *SchemaNames::Element_StatefulService,
                *SchemaNames::Namespace);
        }

        if(xmlReader->HasAttribute(*SchemaNames::Attribute_TargetReplicaSetSize))
        {
            wstring targetReplicaSetSizeCount = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_TargetReplicaSetSize);
            if (!StringUtility::TryFromWString<uint>(
                targetReplicaSetSizeCount, 
                this->TargetReplicaSetSize) || this->TargetReplicaSetSize < 1)
            {
                Parser::ThrowInvalidContent(xmlReader, L"positive integer", targetReplicaSetSizeCount);
            }
        }   
        else
        {
            // default described in XSD
            this->TargetReplicaSetSize = 1;
        }

        if(xmlReader->HasAttribute(*SchemaNames::Attribute_MinReplicaSetSize))
        {
            wstring minReplicaSetSizeCount = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_MinReplicaSetSize);
            if (!StringUtility::TryFromWString<uint>(
                minReplicaSetSizeCount, 
                this->MinReplicaSetSize) || this->MinReplicaSetSize < 1)
            {
                Parser::ThrowInvalidContent(xmlReader, L"positive integer", minReplicaSetSizeCount);
            }
        }           
        else
        {
            // default described in XSD
            this->MinReplicaSetSize = 1;
        }

        if (xmlReader->HasAttribute(*SchemaNames::Attribute_ReplicaRestartWaitDurationSeconds))
        {
            double replicaRestartWaitDurationSeconds;
            wstring attrValue = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ReplicaRestartWaitDurationSeconds);
            if (StringUtility::TryFromWString<double>(attrValue, replicaRestartWaitDurationSeconds))
            {
				if (replicaRestartWaitDurationSeconds < 0)
				{
					replicaRestartWaitDurationSeconds = 0;
				}
                this->ReplicaRestartWaitDuration = TimeSpan::FromSeconds(replicaRestartWaitDurationSeconds);
            }
            else
            {
                Parser::ThrowInvalidContent(xmlReader, L"int", attrValue);
            }
        }

        if (xmlReader->HasAttribute(*SchemaNames::Attribute_QuorumLossWaitDurationSeconds))
        {
            double quorumLossWaitDurationSeconds;
            wstring attrValue = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_QuorumLossWaitDurationSeconds);
            if (StringUtility::TryFromWString<double>(attrValue, quorumLossWaitDurationSeconds))
            {
				if (quorumLossWaitDurationSeconds < 0)
				{
					quorumLossWaitDurationSeconds = 0;
				}
                this->QuorumLossWaitDuration = TimeSpan::FromSeconds(quorumLossWaitDurationSeconds);
            }
            else
            {
                Parser::ThrowInvalidContent(xmlReader, L"int", attrValue);
            }
        }

        if (xmlReader->HasAttribute(*SchemaNames::Attribute_StandByReplicaKeepDurationSeconds))
        {
            double standByReplicaKeepDurationSeconds;
            wstring attrValue = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_StandByReplicaKeepDurationSeconds);
            if (StringUtility::TryFromWString<double>(attrValue, standByReplicaKeepDurationSeconds))
            {
				if (standByReplicaKeepDurationSeconds < 0)
				{
					standByReplicaKeepDurationSeconds = 0;
				}
                this->StandByReplicaKeepDuration = TimeSpan::FromSeconds(standByReplicaKeepDurationSeconds);
            }
            else
            {
                Parser::ThrowInvalidContent(xmlReader, L"int", attrValue);
            }
        }
    }
    else
    {
        if (isServiceGroup)
        {
            xmlReader->StartElement(
                *SchemaNames::Element_StatelessServiceGroup,
                *SchemaNames::Namespace);   
        }
        else
        {
            xmlReader->StartElement(
                *SchemaNames::Element_StatelessService,
                *SchemaNames::Namespace);   
        }

        if(xmlReader->HasAttribute(*SchemaNames::Attribute_InstanceCount))
        {
            wstring instanceCountValue = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_InstanceCount);
            if (!StringUtility::TryFromWString<int>(
                instanceCountValue, 
                this->InstanceCount) || this->InstanceCount == 0 || this->InstanceCount < -1)
            {
                Parser::ThrowInvalidContent(xmlReader, L"positive integer or -1", instanceCountValue);
            }
        }     
        else
        {
            // default described in XSD
            this->InstanceCount = 1;
        }
    }     

    this->ServiceTypeName = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ServiceTypeName);

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_DefaultMoveCost))
    {
        IsDefaultMoveCostSpecified = true;
        wstring defaultMoveCostStr = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_DefaultMoveCost);
        if (StringUtility::AreEqualCaseInsensitive(defaultMoveCostStr, L"Zero"))
        {
            DefaultMoveCost = FABRIC_MOVE_COST_ZERO;
        }
        else if (StringUtility::AreEqualCaseInsensitive(defaultMoveCostStr, L"Low"))
        {
            DefaultMoveCost = FABRIC_MOVE_COST_LOW;
        }
        else if (StringUtility::AreEqualCaseInsensitive(defaultMoveCostStr, L"Medium"))
        {
            DefaultMoveCost = FABRIC_MOVE_COST_MEDIUM;
        }
        else if (StringUtility::AreEqualCaseInsensitive(defaultMoveCostStr, L"High"))
        {
            DefaultMoveCost = FABRIC_MOVE_COST_HIGH;
        }
        else
        {
            Parser::ThrowInvalidContent(xmlReader, L"Zero/Low/Medium/High", defaultMoveCostStr);
        }
    }
    else
    {
        IsDefaultMoveCostSpecified = false;
    }
    xmlReader->ReadStartElement();

    ParsePartitionDescription(xmlReader);
    ParseLoadBalancingMetics(xmlReader);
    ParsePlacementConstraints(xmlReader);
    ParseServiceCorrelations(xmlReader);
    ParseServicePlacementPolicies(xmlReader);
    ParseScalingPolicy(xmlReader);

    if (isServiceGroup)
    {
        ParseServiceGroupMembers(xmlReader);
    }

    xmlReader->ReadEndElement();
}

void ApplicationServiceDescription::ParsePartitionDescription(XmlReaderUPtr const & xmlReader)
{
    PartitionSchemeDescription::Enum scheme;

    if (xmlReader->IsStartElement(
        *SchemaNames::Element_SingletonPartition,
        *SchemaNames::Namespace,
        false))
    {
        scheme = PartitionSchemeDescription::Singleton;
    }
    else if (xmlReader->IsStartElement(
        *SchemaNames::Element_UniformInt64Partition,
        *SchemaNames::Namespace,
        false))
    {
        scheme = PartitionSchemeDescription::UniformInt64Range;
    }
    else if (xmlReader->IsStartElement(
        *SchemaNames::Element_NamedPartition,
        *SchemaNames::Namespace,
        false))
    {
        scheme = PartitionSchemeDescription::Named;
    }
    else
    {
        Parser::ThrowInvalidContent(xmlReader, L"SingletonPartition/UniformInt64Partition/NamedPartition", L"", 
            L"Only SingletonPartition, UniformInt64Partition or NamedPartition element is expected to specify partition schema");
        return; // ThrowInvalidContent is not declared as noreturn, and there will be an error if we don't return.
    }

    this->Partition.ReadFromXml(xmlReader, scheme);
}

void ApplicationServiceDescription::ParseLoadBalancingMetics(Common::XmlReaderUPtr const & xmlReader)
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

void ApplicationServiceDescription::ParsePlacementConstraints(Common::XmlReaderUPtr const & xmlReader)
{
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_PlacementConstraints,
        *SchemaNames::Namespace,
        false))
    {
        this->IsPlacementConstraintsSpecified = true;

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

void ApplicationServiceDescription::ParseServiceCorrelations(Common::XmlReaderUPtr const & xmlReader)
{
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_ServiceCorrelations,
        *SchemaNames::Namespace,
        false))
    {
        xmlReader->ReadStartElement();

        bool done = false;
        while(!done)
        {
            if (xmlReader->IsStartElement(
                *SchemaNames::Element_ServiceCorrelation,
                *SchemaNames::Namespace))
            {
                ServiceCorrelationDescription serviceCorrelationDescription;
                serviceCorrelationDescription.ReadFromXml(xmlReader);
                this->ServiceCorrelations.push_back(serviceCorrelationDescription);
            }
            else
            {
                done = true;
            }
        }

        xmlReader->ReadEndElement();
    }
}

void ApplicationServiceDescription::ParseServicePlacementPolicies(Common::XmlReaderUPtr const & xmlReader)
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

void ApplicationServiceDescription::PlacementPoliciesToPlacementConstraints(
    __in const std::vector<ServicePlacementPolicyDescription> & policies,
    __out std::wstring & placementConstraints)
{
    std::map<FABRIC_PLACEMENT_POLICY_TYPE, vector<ServicePlacementPolicyDescription>> typePolicies;
    for (auto it = policies.begin(); it != policies.end(); it++)
    {
        typePolicies[it->Type].push_back(*it);
    }

    for (auto itMap = typePolicies.begin(); itMap != typePolicies.end(); itMap++)
    {
        if (!Common::ServicePlacementPolicyHelper::IsValidPlacementPolicyType(itMap->first))
        {
            Common::Assert::CodingError("Invalid placement policy type {0}", static_cast<ULONG>(itMap->first));
        }

        if (!placementConstraints.empty())
        {
            placementConstraints.append(L" && ");
        }

        placementConstraints.append(L"(");

        auto it = itMap->second.begin();
        while (it != itMap->second.end())
        {
            it->ToPlacementConstraints(placementConstraints);

            it++;
            if (it != itMap->second.end())
            {
                if (itMap->first == FABRIC_PLACEMENT_POLICY_INVALID_DOMAIN)
                {
                    placementConstraints.append(L" && ");
                }
                else
                {
                    placementConstraints.append(L" || ");
                }
            }
        }
        
        placementConstraints.append(L")");
    }

}

void ApplicationServiceDescription::ParseServiceGroupMembers(Common::XmlReaderUPtr const & xmlReader)
{
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_Members,
        *SchemaNames::Namespace,
        false))
    {
        xmlReader->ReadStartElement();

        bool done = false;
        while(!done)
        {
            if (xmlReader->IsStartElement(
                *SchemaNames::Element_Member,
                *SchemaNames::Namespace))
            {
                ServiceGroupMemberDescription serviceGroupMemberDescription;
                serviceGroupMemberDescription.ReadFromXml(xmlReader);
                this->ServiceGroupMembers.push_back(serviceGroupMemberDescription);
            }
            else
            {
                done = true;
            }
        }

        xmlReader->ReadEndElement();
    }
}

void ApplicationServiceDescription::ParseScalingPolicy(XmlReaderUPtr const & xmlReader)
{
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_ServiceScalingPolicies,
        *SchemaNames::Namespace,
        false))
    {
        xmlReader->ReadStartElement();

        bool done = false;
        while (!done)
        {
            if (xmlReader->IsStartElement(
                *SchemaNames::Element_ScalingPolicy,
                *SchemaNames::Namespace))
            {
                Reliability::ServiceScalingPolicyDescription scalingDescription;
                scalingDescription.ReadFromXml(xmlReader);
                this->ScalingPolicies.push_back(scalingDescription);
            }
            else
            {
                done = true;
            }
        }

        xmlReader->ReadEndElement();
    }
}

ErrorCode ApplicationServiceDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	ErrorCode er;
	if (this->IsServiceGroup)
	{
		if (this->IsStateful)
		{
			er = xmlWriter->WriteStartElement(*SchemaNames::Element_StatefulServiceGroup, L"", *SchemaNames::Namespace);
		}
		else
		{
			er = xmlWriter->WriteStartElement(*SchemaNames::Element_StatelessServiceGroup, L"", *SchemaNames::Namespace);

		}
	} 
	else
	{
		if (this->IsStateful)
		{
			er = xmlWriter->WriteStartElement(*SchemaNames::Element_StatefulService, L"", *SchemaNames::Namespace);	
		}
		else
		{
			er = xmlWriter->WriteStartElement(*SchemaNames::Element_StatelessService, L"", *SchemaNames::Namespace);
			
		}
	}
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_ServiceTypeName, this->ServiceTypeName);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_DefaultMoveCost, this->DefaultMoveCostToString());
	if (!er.IsSuccess())
	{
		return er;
	}
	if (this->IsStateful)
	{

		er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_TargetReplicaSetSize, this->TargetReplicaSetSize);
		if (!er.IsSuccess())
		{
			return er;
		}
		er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_MinReplicaSetSize, this->MinReplicaSetSize);
		if (!er.IsSuccess())
		{
			return er;
		}
		int64 replicaRestart = this->ReplicaRestartWaitDuration.TotalSeconds();
		if (replicaRestart >= 0)
		{
			er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_ReplicaRestartWaitDurationSeconds, replicaRestart);
			if (!er.IsSuccess())
			{
				return er;
			}
		}
		
		int64 quorum = this->QuorumLossWaitDuration.TotalSeconds();
		if (quorum >= 0)
		{
			er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_QuorumLossWaitDurationSeconds, quorum);
			if (!er.IsSuccess())
			{
				return er;
			}
		}
		int64 standby = this->StandByReplicaKeepDuration.TotalSeconds();
		if (standby >= 0)
		{
			er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_StandByReplicaKeepDurationSeconds, standby);
			if (!er.IsSuccess())
			{
				return er;
			}
		}
		
	}
	else
	{
		er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_InstanceCount, this->InstanceCount);
		if (!er.IsSuccess())
		{
			return er;
		}
	}

	er = this->Partition.WriteToXml(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = WriteLoadMetrics(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = WritePlacementConstraints(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = WriteServiceCorrelations(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = WriteServicePlacementPolicies(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}

    er = WriteScalingPolicy(xmlWriter);
    if (!er.IsSuccess())
    {
        return er;
    }

	if (this->IsServiceGroup)
	{
		er = WriteMembers(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}
	}

	//<End Service>
	return xmlWriter->WriteEndElement();
}

ErrorCode ApplicationServiceDescription::WriteLoadMetrics(XmlWriterUPtr const & xmlWriter)
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

ErrorCode ApplicationServiceDescription::WritePlacementConstraints(XmlWriterUPtr const & xmlWriter)
{
	if (this->PlacementConstraints.empty())
	{
		return ErrorCodeValue::Success;
	}
	return xmlWriter->WriteElementWithContent(*SchemaNames::Element_PlacementConstraints,
		this->PlacementConstraints, L"", *SchemaNames::Namespace);
}

ErrorCode ApplicationServiceDescription::WriteServiceCorrelations(XmlWriterUPtr const & xmlWriter)
{
	if (this->ServiceCorrelations.empty())
	{
		return ErrorCodeValue::Success;
	}
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ServiceCorrelations, L"", *SchemaNames::Namespace);

	for (auto i = 0; i < ServiceCorrelations.size(); i++)
	{
		er = ServiceCorrelations[i].WriteToXml(xmlWriter);
	}
	return xmlWriter->WriteEndElement();
}

ErrorCode ApplicationServiceDescription::WriteServicePlacementPolicies(XmlWriterUPtr const & xmlWriter)
{
	if (this->ServicePlacementPolicies.empty())
	{
		return ErrorCodeValue::Success;
	}
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ServicePlacementPolicies, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	for (auto i = 0; i < ServicePlacementPolicies.size(); i++)
	{
		er = ServicePlacementPolicies[i].WriteToXml(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	return xmlWriter->WriteEndElement();
}

ErrorCode ApplicationServiceDescription::WriteScalingPolicy(XmlWriterUPtr const & xmlWriter)
{
    if (this->ScalingPolicies.size() == 0)
    {
        return ErrorCode::Success();
    }
    ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ServiceScalingPolicies, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }
    for (auto & scalingPolicy : ScalingPolicies)
    {
        er = scalingPolicy.WriteToXml(xmlWriter);
        if (!er.IsSuccess())
        {
            return er;
        }
    }
    return xmlWriter->WriteEndElement();
}

ErrorCode ApplicationServiceDescription::WriteMembers(XmlWriterUPtr const & xmlWriter)
{
	//Open <Members>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_Members, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	for (int i = 0; i < this->ServiceGroupMembers.size(); i++)
	{
		er = ServiceGroupMembers[i].WriteToXml(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}
	}

	//Close <Members>
	return xmlWriter->WriteEndElement();
}
wstring ApplicationServiceDescription::DefaultMoveCostToString()
{

	switch (this->DefaultMoveCost)
	{
	case FABRIC_MOVE_COST_ZERO:
		return L"Zero";
	case FABRIC_MOVE_COST_MEDIUM:
		return L"Medium";
	case FABRIC_MOVE_COST_HIGH:
		return L"High";
	case FABRIC_MOVE_COST_LOW:
		return L"Low";
	default:
		Assert::CodingError("Invalid Move Cost");
	}
}

void ApplicationServiceDescription::clear()
{
    this->ServiceTypeName.clear();
    this->Partition.clear();
    this->ServiceGroupMembers.clear();
    this->ScalingPolicies.clear();
}
