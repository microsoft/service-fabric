// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Naming;
using namespace Reliability;
using namespace ServiceGroup;

bool Utility::ParseServiceGroupAddress(
    std::wstring const & serviceGroupAddress,
    std::wstring const & pattern,
    __out std::wstring & memberServiceAddress)
{
    size_t startPos = serviceGroupAddress.find(pattern);
    if (std::wstring::npos == startPos)
    {
        return false;
    }

    startPos += pattern.size();
    size_t endPos = serviceGroupAddress.find(*ServiceGroupConstants::ServiceAddressDoubleDelimiter, startPos);
    memberServiceAddress = serviceGroupAddress.substr(startPos, std::wstring::npos == endPos ? endPos : endPos - startPos);
    StringUtility::Replace<wstring>(memberServiceAddress, ServiceGroupConstants::ServiceAddressEscapedDelimiter, ServiceGroupConstants::ServiceAddressDelimiter);

    return true;
}

bool Utility::ExtractMemberServiceLocation(
    ResolvedServicePartitionSPtr const & serviceGroupLocation,
    wstring const & serviceName,
    __out ResolvedServicePartitionSPtr & serviceMemberLocation)
{
    if (!serviceGroupLocation->IsServiceGroup)
    {
        return false;
    }

    std::wstring patternInAddress = wformatString("{0}{1}{2}", ServiceGroupConstants::ServiceAddressDoubleDelimiter, serviceName, ServiceGroupConstants::MemberServiceNameDelimiter);
    
    std::wstring primaryLocation;
    if (serviceGroupLocation->Locations.ServiceReplicaSet.IsStateful && serviceGroupLocation->Locations.ServiceReplicaSet.IsPrimaryLocationValid)
    {
        if (!ParseServiceGroupAddress(serviceGroupLocation->Locations.ServiceReplicaSet.PrimaryLocation, patternInAddress, primaryLocation))
        {
            Trace.WriteError(
                ServiceGroupConstants::TraceSource,
                "Cannot find pattern {0} in service group primary location {1}",
                patternInAddress,
                serviceGroupLocation->Locations.ServiceReplicaSet.PrimaryLocation);
            return false;
        }
    }

    std::vector<std::wstring> const & groupSecondaryLocations = 
        serviceGroupLocation->Locations.ServiceReplicaSet.IsStateful ? 
        serviceGroupLocation->Locations.ServiceReplicaSet.SecondaryLocations :
        serviceGroupLocation->Locations.ServiceReplicaSet.ReplicaLocations;
    std::vector<std::wstring> replicaLocations(groupSecondaryLocations.size());
    for (size_t i =0 ; i < groupSecondaryLocations.size(); i++)
    {
        if (!ParseServiceGroupAddress(groupSecondaryLocations[i], patternInAddress, replicaLocations[i]))
        {
            Trace.WriteError(
                ServiceGroupConstants::TraceSource,
                "Cannot find pattern {0} in service group secondary or replica location {1}",
                patternInAddress,
                groupSecondaryLocations[i]);
            return false;
        }
    }

    ServiceReplicaSet memberServiceReplicaSet(
        serviceGroupLocation->Locations.ServiceReplicaSet.IsStateful,
        serviceGroupLocation->Locations.ServiceReplicaSet.IsPrimaryLocationValid,
        move(primaryLocation),
        move(replicaLocations),
        serviceGroupLocation->Locations.ServiceReplicaSet.LookupVersion);
    
    ServiceTableEntry memberServiceTableEntry(
        serviceGroupLocation->Locations.ConsistencyUnitId,
        serviceName,
        move(memberServiceReplicaSet));
        
    auto memberResolvedServicePartition = std::make_shared<ResolvedServicePartition>(
        false, // isServiceGroup
        memberServiceTableEntry,
        serviceGroupLocation->PartitionData,
        serviceGroupLocation->Generation,
        serviceGroupLocation->StoreVersion,
        nullptr); // psd
        
    serviceMemberLocation = std::move(memberResolvedServicePartition);

    return true;
}

bool Utility::TryConvertToServiceGroupMembers(
    ServiceTableEntry const & ste,
    __out vector<ServiceTableEntrySPtr> & results)
{
    results.clear();

    auto const & replicaSet = ste.ServiceReplicaSet;

    // map of: <member name, pair<primary location, vector<secondary location>>>
    //
    map<wstring, pair<wstring, vector<wstring>>> memberLocations;
    if (replicaSet.IsStateful && replicaSet.IsPrimaryLocationValid)
    {
        map<wstring, wstring> memberPrimaries;
        if (!TryExtractServiceGroupMemberAddresses(replicaSet.PrimaryLocation, memberPrimaries))
        {
            return false;
        }

        for (auto const & primary : memberPrimaries)
        {
            auto const & memberName = primary.first;
            auto const & memberLocation = primary.second;

            memberLocations.insert(make_pair<wstring, pair<wstring, vector<wstring>>>(
                wstring(memberName),
                make_pair<wstring, vector<wstring>>(wstring(memberLocation), vector<wstring>())));
        }
    }

    for (auto const & locations : 
        (replicaSet.IsStateful ? replicaSet.SecondaryLocations : replicaSet.ReplicaLocations))
    {
        map<wstring, wstring> memberReplicas;
        if (!TryExtractServiceGroupMemberAddresses(locations, memberReplicas))
        {
            return false;
        }

        for (auto const & replica : memberReplicas)
        {
            auto const & memberName = replica.first;
            auto const & memberLocation = replica.second;

            auto findIt = memberLocations.find(memberName);
            if (findIt == memberLocations.end())
            {
                findIt = memberLocations.insert(make_pair<wstring, pair<wstring, vector<wstring>>>(
                    wstring(memberName),
                    make_pair<wstring, vector<wstring>>(L"", vector<wstring>()))).first;
            }

            findIt->second.second.push_back(memberLocation);
        }
    }

    for (auto const & member : memberLocations)
    {
        auto const & memberName = member.first;
        auto primaryLocation = member.second.first;
        auto replicaLocations = member.second.second;

        auto entry = make_shared<ServiceTableEntry>(
            ste.ConsistencyUnitId,
            wformatString("{0}{1}{2}", ste.ServiceName, ServiceGroupConstants::MemberServiceNameDelimiter, memberName),
            ServiceReplicaSet(
                replicaSet.IsStateful,
                !primaryLocation.empty(),
                move(primaryLocation),
                move(replicaLocations),
                replicaSet.LookupVersion));

        results.push_back(move(entry));
    }

    return true;
}

bool Utility::TryExtractServiceGroupMemberAddresses(
    wstring const & serviceGroupAddress,
    __out map<wstring, wstring> & memberServiceAddresses)
{
    memberServiceAddresses.clear();

    vector<wstring> tokens;
    StringUtility::Split<wstring>(serviceGroupAddress, tokens, ServiceGroupConstants::ServiceAddressDoubleDelimiter);

    if (tokens.empty())
    {
        return false;
    }

    for (auto const & token : tokens)
    {
        // Don't skip empty segments - the member may intentionally have an empty address
        //
        vector<wstring> segments;
        StringUtility::Split<wstring>(token, segments, ServiceGroupConstants::MemberServiceNameDelimiter, false); 

        if (segments.size() != 2 && segments.size() != 3)
        {
            // Expected SG address format:
            //
            // %%fabric:/service#member1#member1_address%%fabric:/service#member2#member2_address ...
            //
            return false;
        }

        auto const & serviceName = segments[0];
        if (!StringUtility::StartsWith<wstring>(serviceName, L"fabric:/"))
        {
            return false;
        }

        auto memberName = segments[1];
        auto memberAddress = segments.size() == 3 ? segments[2] : L"";

        StringUtility::Replace<wstring>(
            memberAddress, 
            ServiceGroupConstants::ServiceAddressEscapedDelimiter, 
            ServiceGroupConstants::ServiceAddressDelimiter);

        memberServiceAddresses.insert(make_pair<wstring, wstring>(std::move(memberName), std::move(memberAddress)));
    }

    return true;
}
