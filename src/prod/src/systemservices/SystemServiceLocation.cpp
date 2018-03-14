// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Transport;
using namespace SystemServices;

FABRIC_REPLICA_ID const SystemServiceLocation::ReplicaId_AnyReplica = 0;
__int64 const SystemServiceLocation::ReplicaInstance_AnyReplica = 0;

StringLiteral const TraceComponent("SystemServiceLocation");
StringLiteral const LocationFormat("{0}+{1}+{2}+{3}+{4}");

GlobalWString SystemServiceLocation::TokenDelimiter = make_global<wstring>(L"+");

SystemServiceLocation::SystemServiceLocation()
    : nodeInstance_()
    , partitionId_()
    , replicaId_(0)
    , replicaInstance_(0)
    , location_()
    , hostAddress_()
{
}

SystemServiceLocation::SystemServiceLocation(
    Federation::NodeInstance const & nodeInstance, 
    Common::Guid const & partitionId, 
    ::FABRIC_REPLICA_ID replicaId,
    __int64 replicaInstance)
    : nodeInstance_(nodeInstance)
    , partitionId_(partitionId)
    , replicaId_(replicaId)
    , replicaInstance_(replicaInstance)
    , location_()
    , hostAddress_()
{
    this->InitializeLocation();
}

SystemServiceLocation::SystemServiceLocation(
    Federation::NodeInstance const & nodeInstance, 
    Common::Guid const & partitionId, 
    ::FABRIC_REPLICA_ID replicaId,
    __int64 replicaInstance,
    wstring const & hostAddress)
    : nodeInstance_(nodeInstance)
    , partitionId_(partitionId)
    , replicaId_(replicaId)
    , replicaInstance_(replicaInstance)
    , location_()
    , hostAddress_(hostAddress)
{
    this->InitializeLocation();
}

void SystemServiceLocation::InitializeLocation()
{
    StringWriter writer(location_);

    // NodeInstance is used for routing
    // Partition/Replica IDs are used to uniquely identify service instance
    //
    writer.Write(
        LocationFormat,
        nodeInstance_,
        partitionId_,
        static_cast<__int64>(replicaId_),
        replicaInstance_,
        hostAddress_);
}

ErrorCode SystemServiceLocation::Create(
    Federation::NodeInstance const & nodeInstance, 
    Common::Guid const & partitionId, 
    ::FABRIC_REPLICA_ID replicaId,
    __int64 replicaInstance,
    std::wstring const & hostAddress,
    __out SystemServiceLocation & result)
{
    if (StringUtility::Contains<wstring>(hostAddress, TokenDelimiter))
    {
        WriteWarning(
            TraceComponent,
            "Host address cannot contain '{0}': address='{1}'",
            TokenDelimiter,
            hostAddress);

        return ErrorCodeValue::InvalidAddress;
    }

    result = SystemServiceLocation(nodeInstance, partitionId, replicaId, replicaInstance, hostAddress);

    return ErrorCodeValue::Success;
}

SystemServiceLocation & SystemServiceLocation::operator = (SystemServiceLocation && other)
{
    nodeInstance_ = move(other.nodeInstance_);
    partitionId_ = move(other.partitionId_);
    replicaId_ = move(other.replicaId_);
    replicaInstance_ = move(other.replicaInstance_);
    hostAddress_ = move(other.hostAddress_);
    location_ = move(other.location_);
    return *this;
}

bool SystemServiceLocation::operator == (SystemServiceLocation const & other)
{
    return (nodeInstance_ == other.nodeInstance_
        && partitionId_ == other.partitionId_
        && replicaId_ == other.replicaId_
        && replicaInstance_ == other.replicaInstance_
        && hostAddress_ == other.hostAddress_);
}

bool SystemServiceLocation::EqualsIgnoreInstances(SystemServiceLocation const & other) const
{
    return (partitionId_ == other.partitionId_ && replicaId_ == other.replicaId_);
}

SystemServiceFilterHeader SystemServiceLocation::CreateFilterHeader() const
{
    return SystemServiceFilterHeader(partitionId_, replicaId_, replicaInstance_);
}

bool SystemServiceLocation::TryParse(
    wstring const & serviceLocation,     
    __out SystemServiceLocation & result)
{
    return TryParse(serviceLocation, false, result);
}

bool SystemServiceLocation::TryParse(
    wstring const & serviceLocation, 
    bool isFabSrvService,
    __out SystemServiceLocation & result)
{
	wstring temp;
	if (isFabSrvService)
	{
		ExtractedEndpointList extractedEndpointList;
        if (!ExtractedEndpointList::FromString(serviceLocation, extractedEndpointList))
        {
            return false;
        }        

		if (!extractedEndpointList.GetFirstEndpoint(temp))
        {      
            return false;
        }
	}
	else
	{
		temp = serviceLocation;
	}
    
    vector<wstring> tokens;
	StringUtility::Split<wstring>(temp, tokens, TokenDelimiter);

    if (tokens.size() < 4)
    {
        Assert::TestAssert(
            "Could not parse service location '{0}'",
			temp);
        return false;
    }

    Federation::NodeInstance nodeInstance;
    if (!Federation::NodeInstance::TryParse(tokens[0], nodeInstance))
    {
        Assert::TestAssert(
            "Could not parse '{0}' as a node Instance",
            tokens[0]);
        return false;
    }

    Guid partitionId(tokens[1]);

    ::FABRIC_REPLICA_ID replicaId;
    if (!StringUtility::TryFromWString<__int64>(tokens[2], replicaId))
    {
        Assert::TestAssert(
            "Could not parse '{0}' as a replica Id",
            tokens[2]);
        return false;
    }

    __int64 replicaInstance;
    if (!StringUtility::TryFromWString<__int64>(tokens[3], replicaInstance))
    {
        Assert::TestAssert(
            "Could not parse '{0}' as a replica Instance",
            tokens[3]);
        return false;
    }

    wstring hostAddress;
    if (tokens.size() >= 5)
    {
        hostAddress = tokens[4];
    }

    result = SystemServiceLocation(nodeInstance, partitionId, replicaId, replicaInstance, hostAddress);

    return true;
}

void SystemServiceLocation::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w << location_;
}

string SystemServiceLocation::AddField(Common::TraceEvent & traceEvent, string const & name)
{   
    size_t index = 0;
    string locationFormat("{0}+{1}+{2}+{3}+{4}");

    traceEvent.AddEventField<Federation::NodeInstance>(locationFormat, name + ".node", index);
    traceEvent.AddEventField<Guid>(locationFormat, name + ".partitionId", index);
    traceEvent.AddEventField<__int64>(locationFormat, name + ".replicaId", index);
    traceEvent.AddEventField<__int64>(locationFormat, name + ".replicaInstance", index);
    traceEvent.AddEventField<wstring>(locationFormat, name + ".hostAddress", index);

    return locationFormat;
}

void SystemServiceLocation::FillEventData(Common::TraceEventContext & context) const
{
    context.Write<Federation::NodeInstance>(nodeInstance_);
    context.Write<Guid>(partitionId_);
    context.Write<__int64>(static_cast<__int64>(replicaId_));
    context.Write<__int64>(replicaInstance_);
    context.Write<wstring>(hostAddress_);
}
