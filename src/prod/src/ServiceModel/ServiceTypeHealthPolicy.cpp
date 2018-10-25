// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ServiceTypeHealthPolicy::ServiceTypeHealthPolicy()
    : MaxPercentUnhealthyServices(0)
    , MaxPercentUnhealthyPartitionsPerService(0)
    , MaxPercentUnhealthyReplicasPerPartition(0)
{
}

ServiceTypeHealthPolicy::ServiceTypeHealthPolicy(
    BYTE maxPercentUnhealthyServices,
    BYTE maxPercentUnhealthyPartitionsPerService,
    BYTE maxPercentUnhealthyReplicasPerPartition)
    : MaxPercentUnhealthyServices(maxPercentUnhealthyServices)
    , MaxPercentUnhealthyPartitionsPerService(maxPercentUnhealthyPartitionsPerService)
    , MaxPercentUnhealthyReplicasPerPartition(maxPercentUnhealthyReplicasPerPartition)
{
}

bool ServiceTypeHealthPolicy::operator == (ServiceTypeHealthPolicy const & other) const
{
    bool equals = (this->MaxPercentUnhealthyServices == other.MaxPercentUnhealthyServices);
    if (!equals) return equals;

    equals = (this->MaxPercentUnhealthyPartitionsPerService == other.MaxPercentUnhealthyPartitionsPerService);
    if (!equals) return equals;

    equals = (this->MaxPercentUnhealthyReplicasPerPartition == other.MaxPercentUnhealthyReplicasPerPartition);
    if (!equals) return equals;

    return equals;
}

bool ServiceTypeHealthPolicy::operator != (ServiceTypeHealthPolicy const & other) const
{
    return !(*this == other);
}


void ServiceTypeHealthPolicy::WriteTo(TextWriter & w, FormatOptions const &) const
{
   w.Write("ServiceTypeHealthPolicy = {0}", ToString());
}

wstring ServiceTypeHealthPolicy::ToString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<ServiceTypeHealthPolicy&>(*this), objectString);
    if (!error.IsSuccess())
    {
        return wstring();
    }

    return objectString;
}

void ServiceTypeHealthPolicy::ReadFromXml(XmlReaderUPtr const & xmlReader)
{
    this->clear();
    
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_MaxPercentUnhealthyServices))
    {
        Parser::Utility::ReadPercentageAttribute(
            xmlReader, 
            *SchemaNames::Attribute_MaxPercentUnhealthyServices, 
            this->MaxPercentUnhealthyServices);
    }
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_MaxPercentUnhealthyPartitionsPerService))
    {
        Parser::Utility::ReadPercentageAttribute(
            xmlReader, 
            *SchemaNames::Attribute_MaxPercentUnhealthyPartitionsPerService, 
            this->MaxPercentUnhealthyPartitionsPerService);
    }
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_MaxPercentUnhealthyReplicasPerPartition))
    {
        Parser::Utility::ReadPercentageAttribute(
            xmlReader, 
            *SchemaNames::Attribute_MaxPercentUnhealthyReplicasPerPartition, 
            this->MaxPercentUnhealthyReplicasPerPartition);
    }
}

ErrorCode ServiceTypeHealthPolicy::WriteToXml(XmlWriterUPtr const & xmlWriter, bool isDefault, wstring const & name)
{
	//<HealthPolicy>
	ErrorCode er;
	if (isDefault)
	{
		er = xmlWriter->WriteStartElement(*SchemaNames::Element_DefaultServiceTypeHealthPolicy, L"", *SchemaNames::Namespace);

	}
	else
	{
		er = xmlWriter->WriteStartElement(*SchemaNames::Element_ServiceTypeHealthPolicy, L"", *SchemaNames::Namespace);
		if (!er.IsSuccess())
		{
			return er;
		}
		er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_ServiceTypeName, name);
	}
	if (!er.IsSuccess())
	{
		return er;
	}
	
	er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_MaxPercentUnhealthyServices,
		this->MaxPercentUnhealthyServices);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_MaxPercentUnhealthyPartitionsPerService,
		this->MaxPercentUnhealthyPartitionsPerService);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_MaxPercentUnhealthyReplicasPerPartition,
		this->MaxPercentUnhealthyReplicasPerPartition);

	if (!er.IsSuccess())
	{
		return er;
	}
	//</HealthPolicy>
	return xmlWriter->WriteEndElement();
}

void ServiceTypeHealthPolicy::clear()
{
    this->MaxPercentUnhealthyServices = 0;
    this->MaxPercentUnhealthyPartitionsPerService = 0;
    this->MaxPercentUnhealthyReplicasPerPartition = 0;
}

ErrorCode ServiceTypeHealthPolicy::FromString(wstring const & serviceTypeHealthPolicyStr, __out ServiceTypeHealthPolicy & serviceTypeHealthPolicy)
{
    return JsonHelper::Deserialize(serviceTypeHealthPolicy, serviceTypeHealthPolicyStr);
}

ErrorCode ServiceTypeHealthPolicy::FromPublicApi(FABRIC_SERVICE_TYPE_HEALTH_POLICY const & publicServiceTypeHealthPolicy)
{
    auto error = ParameterValidator::ValidatePercentValue(publicServiceTypeHealthPolicy.MaxPercentUnhealthyServices, Constants::MaxPercentUnhealthyServices);
    if (!error.IsSuccess()) { return error; }

    error = ParameterValidator::ValidatePercentValue(publicServiceTypeHealthPolicy.MaxPercentUnhealthyPartitionsPerService, Constants::MaxPercentUnhealthyPartitionsPerService);
    if (!error.IsSuccess()) { return error; }

    error = ParameterValidator::ValidatePercentValue(publicServiceTypeHealthPolicy.MaxPercentUnhealthyReplicasPerPartition, Constants::MaxPercentUnhealthyPartitionsPerService);
    if (!error.IsSuccess()) { return error; }

    this->MaxPercentUnhealthyServices = publicServiceTypeHealthPolicy.MaxPercentUnhealthyServices;
    this->MaxPercentUnhealthyPartitionsPerService = publicServiceTypeHealthPolicy.MaxPercentUnhealthyPartitionsPerService;
    this->MaxPercentUnhealthyReplicasPerPartition = publicServiceTypeHealthPolicy.MaxPercentUnhealthyReplicasPerPartition;

    return ErrorCode(ErrorCodeValue::Success);
}

void ServiceTypeHealthPolicy::ToPublicApi(
    __in ScopedHeap & heap, 
    __out FABRIC_SERVICE_TYPE_HEALTH_POLICY & publicServiceTypeHealthPolicy) const
{
    UNREFERENCED_PARAMETER(heap);

    publicServiceTypeHealthPolicy.MaxPercentUnhealthyServices = this->MaxPercentUnhealthyServices;
    publicServiceTypeHealthPolicy.MaxPercentUnhealthyPartitionsPerService = this->MaxPercentUnhealthyPartitionsPerService;
    publicServiceTypeHealthPolicy.MaxPercentUnhealthyReplicasPerPartition = this->MaxPercentUnhealthyReplicasPerPartition;
}

ErrorCode ServiceTypeHealthPolicy::FromPublicApi(
    FABRIC_SERVICE_TYPE_HEALTH_POLICY_MAP_ITEM const & publicServiceTypeHealthPolicyItem,
    __out wstring & serviceTypeName)
{
    if ((publicServiceTypeHealthPolicyItem.ServiceTypeHealthPolicy == NULL) || (publicServiceTypeHealthPolicyItem.ServiceTypeName == NULL))
    {
        return ErrorCode(ErrorCodeValue::ArgumentNull);
    }

    auto error = ParameterValidator::IsValid(publicServiceTypeHealthPolicyItem.ServiceTypeName);
    if (!error.IsSuccess())
    {
        return error;
    }

    serviceTypeName = wstring(publicServiceTypeHealthPolicyItem.ServiceTypeName);
    return this->FromPublicApi(*publicServiceTypeHealthPolicyItem.ServiceTypeHealthPolicy);
}

void ServiceTypeHealthPolicy::ToPublicApiMapItem(
    __in ScopedHeap & heap,
    wstring const & serviceTypeName,
    __out FABRIC_SERVICE_TYPE_HEALTH_POLICY_MAP_ITEM & publicServiceTypeHealthPolicyItem) const
{
    auto serviceTypeHealthPolicy = heap.AddItem<FABRIC_SERVICE_TYPE_HEALTH_POLICY>();
    this->ToPublicApi(heap, *serviceTypeHealthPolicy);
    publicServiceTypeHealthPolicyItem.ServiceTypeHealthPolicy = serviceTypeHealthPolicy.GetRawPointer();
    publicServiceTypeHealthPolicyItem.ServiceTypeName = heap.AddString(serviceTypeName);
}

bool ServiceTypeHealthPolicy::TryValidate(__out wstring & validationErrorMessage) const
{
    auto error = ParameterValidator::ValidatePercentValue(MaxPercentUnhealthyServices, Constants::MaxPercentUnhealthyServices);
    if (error.IsSuccess())
    {
        error = ParameterValidator::ValidatePercentValue(MaxPercentUnhealthyPartitionsPerService, Constants::MaxPercentUnhealthyPartitionsPerService);
    }

    if (error.IsSuccess())
    {
        error = ParameterValidator::ValidatePercentValue(MaxPercentUnhealthyReplicasPerPartition, Constants::MaxPercentUnhealthyReplicasPerPartition);
    }

    if (!error.IsSuccess())
    {
        validationErrorMessage = error.TakeMessage();
        return false;
    }

    return true;
}
