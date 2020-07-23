// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Reliability;
using namespace Federation;
using namespace ServiceModel;
using namespace SystemServices;
using namespace std;
using namespace Transport;
using namespace ClientServerTransport;

namespace Naming
{
#if !defined(PLATFORM_UNIX)
    Common::Global<locale> NamingUtility::USLocale = Common::make_global<locale>("us");
#else
    Common::Global<locale> NamingUtility::USLocale = Common::make_global<locale>("en_US.UTF-8");
#endif

    bool NamingUtility::ParseServiceGroupAddress(
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
        size_t endPos = serviceGroupAddress.find(*Constants::ServiceAddressDoubleDelimiter, startPos);
        memberServiceAddress = serviceGroupAddress.substr(startPos, std::wstring::npos == endPos ? endPos : endPos - startPos);
        Common::StringUtility::Replace<std::wstring>(memberServiceAddress, Constants::ServiceAddressEscapedDelimiter, Constants::ServiceAddressDelimiter);

        return true;
    }

    bool NamingUtility::ExtractMemberServiceLocation(
        ResolvedServicePartitionSPtr const & serviceGroupLocation,
        wstring const & serviceName,
        __out ResolvedServicePartitionSPtr & serviceMemberLocation)
    {
        if (!serviceGroupLocation->IsServiceGroup)
        {
            return false;
        }

        std::wstring patternInAddress = Constants::ServiceAddressDoubleDelimiter;
        patternInAddress += serviceName;
        patternInAddress += (*Constants::MemberServiceNameDelimiter);
        
        std::wstring primaryLocation;
        if (serviceGroupLocation->Locations.ServiceReplicaSet.IsStateful && serviceGroupLocation->Locations.ServiceReplicaSet.IsPrimaryLocationValid)
        {
            if (!ParseServiceGroupAddress(serviceGroupLocation->Locations.ServiceReplicaSet.PrimaryLocation, patternInAddress, primaryLocation))
            {
                Trace.WriteError(
                    Constants::NamingServiceSource,
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
                    Constants::NamingServiceSource,
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
            false, // IsServiceGroup
            memberServiceTableEntry,
            serviceGroupLocation->PartitionData,
            serviceGroupLocation->Generation,
            serviceGroupLocation->StoreVersion,
            serviceGroupLocation->Psd);
            
        serviceMemberLocation = std::move(memberResolvedServicePartition);

        return true;
    }

    void NamingUtility::SetCommonTransportProperties(Transport::IDatagramTransport & transport)
    {
        transport.SetMaxIncomingFrameSize(ServiceModelConfig::GetConfig().MaxMessageSize);

        //Set outgoing message size limit to avoid sending oversized messages to old clients.
        //New clients will indicate whether message to them have size limit in SecurityNegotiationHeader
        transport.SetMaxOutgoingFrameSize(ServiceModelConfig::GetConfig().MaxMessageSize);

        //
        // Disable the connection idle timeout logic in transport. Fabricclient automatically reconnects
        // when there is a disconnect.
        //
        transport.SetConnectionIdleTimeout(TimeSpan::Zero); 
    }
    
    size_t NamingUtility::GetMessageContentThreshold()
    {
        return static_cast<size_t>(
            static_cast<double>(ServiceModelConfig::GetConfig().MaxMessageSize) * ServiceModelConfig::GetConfig().MessageContentBufferRatio);
    }

    ErrorCode NamingUtility::CheckEstimatedSize(size_t estimatedSize)
    {
        return CheckEstimatedSize(estimatedSize, GetMessageContentThreshold());
    }

    ErrorCode NamingUtility::CheckEstimatedSize(
        size_t estimatedSize,
        size_t maxAllowedSize)
    {
        // The entry must be less than the max allowed size.
        if (estimatedSize >= maxAllowedSize)
        {
            Trace.WriteInfo(
                Constants::CommonSource,
                "Entry size {0} is larger than allowed size {1}", 
                estimatedSize, 
                maxAllowedSize);

            return ErrorCode(ErrorCodeValue::EntryTooLarge);
        }

        return ErrorCode(ErrorCodeValue::Success);
    }

    size_t NamingUtility::GetHash(std::wstring const & value)
    {
        __int64 key = use_facet< collate<wchar_t> >(USLocale).hash(&value[0], &value[value.size() - 1]);
        return static_cast<size_t>(key);
    }

    bool NamingUtility::ValidateClientMessage(MessageUPtr const &message, StringLiteral const &traceComponent, wstring const &instanceString)
    {
        bool success = true;
        {
            FabricActivityHeader header;
            success = message->Headers.TryReadFirst(header);
        }

        if (success)
        {
            TimeoutHeader header;
            success = message->Headers.TryReadFirst(header);
        }

        if (success)
        {
            success = (message->Actor == Actor::NamingGateway);
        }

        if (success)
        {
            ClientProtocolVersionHeader versionHeader;
            success = message->Headers.TryReadFirst(versionHeader);

            if (success)
            {
                if (!versionHeader.IsCompatibleVersion())
                {
                    Trace.WriteWarning(
                        traceComponent,
                        "{0}: unsupported protocol version: expected = '{1}' received = '{2}'",
                        instanceString,
                        ClientProtocolVersionHeader::CurrentMajorVersion,
                        versionHeader);

                    success = false;
                }
            }
            else
            {
                Trace.WriteWarning(
                    traceComponent,
                    "{0}: message missing protocol version header: expected = '{1}'",
                    instanceString,
                    ClientProtocolVersionHeader::CurrentVersionHeader);
            }
        }

        return success;
    }

    ErrorCode NamingUtility::CheckClientTimeout(TimeSpan const &clientTimeout, StringLiteral const &traceComponent, wstring const &instanceString)
    {
        if (clientTimeout > TimeSpan::Zero)
        {
            return ErrorCodeValue::Success;
        }

        auto msg = GET_NS_RC(Invalid_Client_Timeout);

        Trace.WriteWarning(traceComponent, "{0}: {1}", instanceString, msg);
        return ErrorCode(ErrorCodeValue::InvalidArgument, move(msg));
    }

    wstring NamingUtility::GetFabricGatewayServiceName(wstring const &nodeName)
    {
        return L"HostedService/" + nodeName + L"_FabricGateway";
    }

    wstring NamingUtility::GetFabricApplicationGatewayServiceName(wstring const &nodeName)
    {
        return L"HostedService/" + nodeName + L"_FabricApplicationGateway";
    }

    bool NamingUtility::ExtractServiceRoutingAgentHeader(MessageUPtr const & receivedMessage,
        Common::StringLiteral const &traceComponent,
        __out SystemServices::ServiceRoutingAgentHeader & routingAgentHeader)
    {
        Transport::ForwardMessageHeader forwardMessageHeader;
        if (!receivedMessage->Headers.TryReadFirst(forwardMessageHeader))
        {
            Trace.WriteWarning(
                traceComponent,
                "{0} ForwardMessageHeader missing: {1}",
                FabricActivityHeader::FromMessage(*receivedMessage).ActivityId,
                *receivedMessage);

            return false;
        }

        if (forwardMessageHeader.Actor == Actor::ServiceRoutingAgent
            && forwardMessageHeader.Action == ServiceRoutingAgentMessage::ServiceRouteRequestAction)
        {
            if (!receivedMessage->Headers.TryReadFirst(routingAgentHeader))
            {
                Trace.WriteWarning(
                    traceComponent,
                    "{0} ServiceRouteRequestAction message is missing ServiceRoutingAgentHeader during access check",
                    FabricActivityHeader::FromMessage(*receivedMessage).ActivityId);

                return false;
            }
        }

        return true;
    }

}
