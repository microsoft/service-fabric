// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace HttpGateway;
using namespace Naming;
using namespace Management::FaultAnalysisService;
using namespace ServiceModel;

UriArgumentParser::UriArgumentParser(GatewayUri const & uri)
: uri_(uri)
{
}

Common::ErrorCode UriArgumentParser::TryGetRequiredPartitionId(__inout Common::Guid & partitionId)
{
    return TryGetPartitionId(true, partitionId);
}

Common::ErrorCode UriArgumentParser::TryGetOptionalPartitionId(__inout Common::Guid & partitionId)
{
    return TryGetPartitionId(false, partitionId);
}

ErrorCode UriArgumentParser::TryGetPartitionId(bool isRequired, __inout Guid & partitionId)
{
    wstring value;
    if (!uri_.GetItem(Constants::PartitionIdString, value))
    {
        if (isRequired)
        {
            return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_HTTP_GATEWAY_RC(Missing_Required_Parameter), Constants::PartitionIdString));
        }

        return ErrorCodeValue::NameNotFound;
    }

    if (!Guid::TryParse(value, Constants::HttpGatewayTraceId, partitionId))
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_HTTP_GATEWAY_RC(Invalid_PartitionId_Parameter), value));
    }

    return ErrorCodeValue::Success;
}

ErrorCode UriArgumentParser::TryGetServicePackageActivationId(__out wstring & servicePackageActivationId)
{
    if (!uri_.GetItem(Constants::ServicePackageActivationIdString, servicePackageActivationId))
    {
        return ErrorCodeValue::NameNotFound;
    }

    return ErrorCodeValue::Success;
}

ErrorCode UriArgumentParser::TryGetOperationId(__out Guid & operationId)
{
    wstring value;
    if (!uri_.GetItem(Constants::OperationIdString, value))
    {
        return ErrorCodeValue::NameNotFound;
    }

    if (!Guid::TryParse(value, Constants::HttpGatewayTraceId, operationId))
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_COMMON_RC(Invalid_Guid), value));
    }

    return ErrorCodeValue::Success;
}

ErrorCode UriArgumentParser::TryGetReplicaId(__inout int64 & replicaId)
{
    wstring value;
    if (!uri_.GetItem(Constants::ReplicaIdString, value))
    {
        return ErrorCode(ErrorCodeValue::NameNotFound, wformatString(GET_HTTP_GATEWAY_RC(Missing_Parameter), Constants::ReplicaIdString));
    }

    if (!TryParseInt64(value, replicaId))
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_HTTP_GATEWAY_RC(Invalid_ReplicaId_Parameter), value));
    }

    return ErrorCodeValue::Success;
}

ErrorCode UriArgumentParser::TryGetRequiredReplicaId(__inout int64 & replicaId)
{
    wstring value;
    if (!uri_.GetItem(Constants::ReplicaIdString, value))
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_HTTP_GATEWAY_RC(Missing_Required_Parameter), Constants::ReplicaIdString));
    }

    if (!TryParseInt64(value, replicaId))
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_HTTP_GATEWAY_RC(Invalid_ReplicaId_Parameter), value));
    }

    return ErrorCodeValue::Success;
}

Common::ErrorCode UriArgumentParser::TryGetNodeInstanceId(__out uint64 & nodeInstanceId)
{
    wstring value;
    if (!uri_.GetItem(Constants::NodeInstanceIdString, value))
    {
        return ErrorCodeValue::NameNotFound;
    }

    if (!TryParseUInt64(value, nodeInstanceId))
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_COMMON_RC(Invalid_Node_Instance), value));
    }

    return ErrorCodeValue::Success;
}

// TODO: Make these error messages more detailed by adding an actual error message
ErrorCode UriArgumentParser::TryGetNodeName(__out wstring & nodeName)
{
    if (!uri_.GetItem(Constants::NodeIdString, nodeName))
    {
        return ErrorCodeValue::NameNotFound;
    }

    return Utility::ValidateString(nodeName);
}

ErrorCode UriArgumentParser::TryGetNodeStatusFilter(
    __out DWORD &nodeStatus)
{
    wstring nodeStatusString;
    nodeStatus = FABRIC_QUERY_NODE_STATUS_FILTER_DEFAULT;

    if (uri_.GetItem(Constants::NodeStatusFilterString, nodeStatusString))
    {
        wstring nodeStatusFilter;
        if (StringUtility::AreEqualCaseInsensitive(nodeStatusString, Constants::NodeStatusFilterAllString))
        {
            nodeStatusFilter = *Constants::NodeStatusFilterAll;
        }
        else if (StringUtility::AreEqualCaseInsensitive(nodeStatusString, Constants::NodeStatusFilterUpString))
        {
            nodeStatusFilter = *Constants::NodeStatusFilterUp;
        }
        else if (StringUtility::AreEqualCaseInsensitive(nodeStatusString, Constants::NodeStatusFilterDownString))
        {
            nodeStatusFilter = *Constants::NodeStatusFilterDown;
        }
        else if (StringUtility::AreEqualCaseInsensitive(nodeStatusString, Constants::NodeStatusFilterEnablingString))
        {
            nodeStatusFilter = *Constants::NodeStatusFilterEnabling;
        }
        else if (StringUtility::AreEqualCaseInsensitive(nodeStatusString, Constants::NodeStatusFilterDisablingString))
        {
            nodeStatusFilter = *Constants::NodeStatusFilterDisabling;
        }
        else if (StringUtility::AreEqualCaseInsensitive(nodeStatusString, Constants::NodeStatusFilterDisabledString))
        {
            nodeStatusFilter = *Constants::NodeStatusFilterDisabled;
        }
        else if (StringUtility::AreEqualCaseInsensitive(nodeStatusString, Constants::NodeStatusFilterUnknownString))
        {
            nodeStatusFilter = *Constants::NodeStatusFilterUnknown;
        }
        else if (StringUtility::AreEqualCaseInsensitive(nodeStatusString, Constants::NodeStatusFilterRemovedString))
        {
            nodeStatusFilter = *Constants::NodeStatusFilterRemoved;
        }
        else if (StringUtility::AreEqualCaseInsensitive(nodeStatusString, Constants::NodeStatusFilterDefaultString))
        {
            nodeStatusFilter = *Constants::NodeStatusFilterDefault;
        }

        return Utility::TryParseQueryFilter(nodeStatusFilter, FABRIC_QUERY_NODE_STATUS_FILTER_DEFAULT, nodeStatus);
    }
    else
    {
        return ErrorCodeValue::NameNotFound;
    }
}

ErrorCode UriArgumentParser::TryGetContinuationToken(__out wstring & continuationToken)
{
    if (!uri_.GetItem(Constants::ContinuationTokenString, continuationToken))
    {
        return ErrorCodeValue::NameNotFound;
    }

    return Utility::ValidateString(continuationToken);
}

ErrorCode UriArgumentParser::TryGetCodePackageName(
    __out wstring & codePackageName)
{
    wstring escapedCodePackageName;
    if (!uri_.GetItem(Constants::CodePackageNameIdString, escapedCodePackageName))
    {
        return ErrorCodeValue::NameNotFound;
    }

    return NamingUri::UnescapeString(escapedCodePackageName, codePackageName);
}

// MaxResults will return InvalidArgument if it's a negative number.
ErrorCode UriArgumentParser::TryGetMaxResults(__inout int64 & maxResults)
{
    wstring intValueString;

    // Try to read the MaxResults value into intValueString
    // If it doesn't exist, then return success. The max results has already been set to the default value above
    if (!uri_.GetItem(Constants::MaxResultsString, intValueString))
    {
        // Optional parameter, so return success if not found (along with default value)
        return ErrorCodeValue::NameNotFound;
    }

    // Try to parse the intValueString pulled from the REST request as an int
    // If it fails, then return invalid argument
    // If it succeeds then it sets the __out value to the retrieved integer value
    if (!Config::TryParse<int64>(maxResults, intValueString) || maxResults < 0)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(GET_COMMON_RC(Invalid_Max_Results), intValueString));
    }

    return ErrorCodeValue::Success;
}

ErrorCode UriArgumentParser::TryGetPagingDescription(__out ServiceModel::QueryPagingDescription & pagingDescription)
{
    int64 maxResults;
    wstring continuationToken;

    auto error = TryGetMaxResults(maxResults);
    if (error.IsSuccess())
    {
        pagingDescription.MaxResults = maxResults;
    }
    else if (!error.IsError(ErrorCodeValue::NameNotFound))
    {
        return error;
    }

    error = TryGetContinuationToken(continuationToken);
    if (error.IsSuccess())
    {
        pagingDescription.ContinuationToken = move(continuationToken);
    }
    else if (!error.IsError(ErrorCodeValue::NameNotFound))
    {
        return error;
    }

    return ErrorCodeValue::Success;
}

ErrorCode UriArgumentParser::TryGetPathId(
    std::wstring const & paramName,
    __out NamingUri & name) const
{
    std::wstring nameId;
    std::wstring nameString;
    if (!uri_.GetItem(paramName, nameId))
    {
        return ErrorCodeValue::NameNotFound;
    }

    auto error = NamingUri::IdToFabricName(NamingUri::RootNamingUriString, nameId, nameString);
    if (!error.IsSuccess())
    {
        return error;
    }

    error = NamingUri::TryParse(nameString, Constants::HttpGatewayTraceId, name);
    if (!error.IsSuccess())
    {
        return error;
    }

    return ErrorCodeValue::Success;
}

ErrorCode UriArgumentParser::TryGetName(
    __out NamingUri & name)
{
    return TryGetPathId(Constants::NameString, name);
}

ErrorCode UriArgumentParser::TryGetApplicationName(
    __out NamingUri &applicationName)
{
    return TryGetPathId(Constants::ApplicationIdString, applicationName);
}

ErrorCode UriArgumentParser::TryGetAbsoluteServiceName(
    __out NamingUri & name)
{
    wstring serviceName;
    if (!uri_.GetItem(Constants::ServiceIdString, serviceName))
    {
        return ErrorCodeValue::NameNotFound;
    }

    wstring applicationName;
    if (!uri_.GetItem(Constants::ApplicationIdString, applicationName))
    {
        return ErrorCodeValue::NameNotFound;
    }

    wstring nameString;
    auto error = NamingUri::IdToFabricName(NamingUri::RootNamingUriString, wformatString("{0}/{1}", applicationName, serviceName), nameString);
    if (!error.IsSuccess())
    {
        return error;
    }

    return NamingUri::TryParse(nameString, Constants::HttpGatewayTraceId, name);
}

ErrorCode UriArgumentParser::TryGetDeploymentName(
    __out wstring & deploymentName)
{
    if (!uri_.GetItem(Constants::DeploymentNameString, deploymentName))
    {
        return ErrorCodeValue::NameNotFound;
    }
    else
    {
        return Utility::ValidateString(deploymentName);
    }
}

ErrorCode UriArgumentParser::TryGetContainerName(
    __out wstring & containerName)
{
    if (!uri_.GetItem(Constants::ContainerNameString, containerName))
    {
        return ErrorCodeValue::NameNotFound;
    }
    else
    {
        return Utility::ValidateString(containerName);
    }
}

ErrorCode UriArgumentParser::TryGetApplicationNameInQueryParam(
    __out NamingUri & applicationName)
{
    wstring appNameString;
    if (!uri_.GetItem(Constants::ApplicationNameString, appNameString))
    {
        return ErrorCodeValue::NameNotFound;
    }

    HRESULT hr = NamingUri::TryParse(appNameString.c_str(), Constants::HttpGatewayTraceId, applicationName);
    if (FAILED(hr))
    {
        return ErrorCode::FromHResult(hr);
    }

    return ErrorCodeValue::Success;
}

ErrorCode UriArgumentParser::TryGetImageStoreRelativePath(
    __out std::wstring & relativePath)
{
    wstring unescapedRelativePath;
    if (!uri_.GetItem(Constants::NativeImageStoreRelativePathString, unescapedRelativePath))
    {
        return ErrorCodeValue::NameNotFound;
    }

    return NamingUri::UnescapeString(unescapedRelativePath, relativePath);
}

ErrorCode UriArgumentParser::TryGetInstanceId(
    __out wstring & instanceId) const
{
    if (!uri_.GetItem(Constants::InstanceIdString, instanceId))
    {
        return ErrorCodeValue::NameNotFound;
    }

    return Utility::ValidateString(instanceId);
}

ErrorCode UriArgumentParser::TryGetServiceName(
    __out NamingUri &serviceName)
{
    return TryGetPathId(Constants::ServiceIdString, serviceName);
}

ErrorCode UriArgumentParser::TryGetServiceTypeName(__out wstring & serviceTypeName)
{
    if (!uri_.GetItem(Constants::ServiceTypeNameString, serviceTypeName))
    {
        return ErrorCodeValue::ServiceTypeNotFound;
    }

    return Utility::ValidateString(serviceTypeName);
}

ErrorCode UriArgumentParser::TryGetServiceManifestName(__out wstring & serviceManifestName)
{
    wstring escapedServiceManifestName;
    if (!uri_.GetItem(Constants::ServiceManifestNameIdString, escapedServiceManifestName))
    {
        return ErrorCodeValue::NameNotFound;
    }

    return NamingUri::UnescapeString(escapedServiceManifestName, serviceManifestName);
}

ErrorCode UriArgumentParser::TryGetMode(
    __out wstring &marker)
{

//    wstring serviceNameString;
    if (!uri_.GetItem(Constants::Mode, marker))
    {
        return ErrorCodeValue::NameNotFound;
    }

    return ErrorCodeValue::Success;
}


ErrorCode UriArgumentParser::TryGetReplicaStatusFilter(
    __out DWORD &replicaStatus)
{
    wstring replicaStatusString;
    replicaStatus = FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER_DEFAULT;

    if (uri_.GetItem(Constants::ReplicaStatusFilterString, replicaStatusString))
    {
        return Utility::TryParseQueryFilter(replicaStatusString, FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER_DEFAULT, replicaStatus);
    }
    else
    {
        return ErrorCodeValue::NameNotFound;
    }
}

ErrorCode UriArgumentParser::TryGetPartitionKey(
    __out PartitionKey& partitionKey)
{
    wstring partitionKeyType;
    if (!uri_.GetItem(Constants::PartitionKeyType, partitionKeyType))
    {
        // Not sending any partition key is treated as FABRIC_PARTITION_KEY_TYPE_NONE
        return ErrorCodeValue::Success;
    }

    uint parsedKeyType;
    if (!StringUtility::TryFromWString<uint>(partitionKeyType, parsedKeyType))
    {
        return ErrorCodeValue::InvalidArgument;
    }

    if (parsedKeyType == FABRIC_PARTITION_KEY_TYPE_INT64)
    {
        wstring partitionKeyValue;
        __int64 int64Key;
        if (!uri_.GetItem(Constants::PartitionKeyValue, partitionKeyValue) ||
            !TryParseInt64(partitionKeyValue, int64Key))
        {
            return ErrorCodeValue::InvalidArgument;
        }

        PartitionKey key(int64Key);

        partitionKey = move(key);
        return ErrorCodeValue::Success;
    }
    else if (parsedKeyType == FABRIC_PARTITION_KEY_TYPE_STRING)
    {
        wstring partitionKeyValue;
        if (!uri_.GetItem(Constants::PartitionKeyValue, partitionKeyValue))
        {
            return ErrorCodeValue::InvalidArgument;
        }

        PartitionKey key(partitionKeyValue);

        partitionKey = move(key);
        return ErrorCodeValue::Success;
    }
    else if (parsedKeyType == FABRIC_PARTITION_KEY_TYPE_NONE)
    {
        return ErrorCodeValue::Success;
    }

    return ErrorCodeValue::InvalidArgument;
}

ErrorCode UriArgumentParser::TryGetOptionalBool(
    std::wstring const & paramName,
    __out bool & value) const
{
    value = false;

    wstring boolValue;
    if (!uri_.GetItem(paramName, boolValue))
    {
        // Optional parameter
        return ErrorCodeValue::Success;
    }

    auto boolParser = StringUtility::ParseBool();
    if (!boolParser.Try(boolValue, value))
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(GET_COMMON_RC(Invalid_String_Boolean_Value), paramName, boolValue));
    }

    return ErrorCodeValue::Success;
}

ErrorCode UriArgumentParser::TryGetBoolOrError(
    std::wstring const & paramName,
    __inout bool & value) const
{
    value = false;

    wstring boolValue;
    if (!uri_.GetItem(paramName, boolValue))
    {
        return ErrorCodeValue::NotFound;
    }

    if (!Config::TryParse<bool>(value, boolValue))
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(GET_COMMON_RC(Invalid_String_Boolean_Value), paramName, boolValue));
    }

    return ErrorCodeValue::Success;
}

ErrorCode UriArgumentParser::TryGetForceRemove(
    __out bool & forceRemove)
{
    return TryGetOptionalBool(Constants::ForceRemove, forceRemove);
}

ErrorCode UriArgumentParser::TryGetCreateFabricDump(
    __out bool & createFabricDump)
{
    return TryGetOptionalBool(Constants::CreateFabricDump, createFabricDump);
}

ErrorCode UriArgumentParser::TryGetForce(
    __out bool & force)
{
    return TryGetOptionalBool(Constants::Force, force);
}

ErrorCode UriArgumentParser::TryGetImmediateOption(
    __inout bool& immediate)
{
    return TryGetOptionalBool(Constants::Immediate, immediate);
}

ErrorCode UriArgumentParser::TryGetExcludeApplicationParameters(
    __out bool & excludeApplicationParameters) const
{
    return TryGetOptionalBool(Constants::ExcludeApplicationParametersString, excludeApplicationParameters);
}

ErrorCode UriArgumentParser::GetExcludeApplicationParameters(
    __out bool & excludeApplicationParameters) const
{
    return TryGetBoolOrError(Constants::ExcludeApplicationParametersString, excludeApplicationParameters);
}

ErrorCode HttpGateway::UriArgumentParser::TryGetIncludeHealthState(
    __out bool & includeHealthState) const
{
    return TryGetOptionalBool(Constants::IncludeHealthStateString, includeHealthState);
}

ErrorCode HttpGateway::UriArgumentParser::GetIncludeHealthState(
    __out bool & includeHealthState) const
{
    return TryGetBoolOrError(Constants::IncludeHealthStateString, includeHealthState);
}

ErrorCode UriArgumentParser::TryGetRecursive(
    __out bool & recursive) const
{
    return TryGetOptionalBool(Constants::RecursiveString, recursive);
}

ErrorCode UriArgumentParser::TryGetIncludeValues(
    __out bool & includeValues) const
{
    return TryGetOptionalBool(Constants::IncludeValuesString, includeValues);
}

ErrorCode UriArgumentParser::TryGetServiceKind(
    bool isRequired,
    __out FABRIC_SERVICE_KIND & serviceKind) const
{
    wstring serviceKindString;
    if (!uri_.GetItem(Constants::ServiceKindString, serviceKindString))
    {
        if (isRequired)
        {
            return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_HTTP_GATEWAY_RC(Missing_Required_Parameter), Constants::ServiceKindString));
        }
        else
        {
            serviceKind = FABRIC_SERVICE_KIND_STATEFUL;
            return ErrorCode::Success();
        }
    }

    int parsedServiceKind;
    if (!StringUtility::TryFromWString<int>(serviceKindString, parsedServiceKind))
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString("{0} {1}.", GET_NAMING_RC(Invalid_Service_Kind), serviceKindString));
    }

    if (parsedServiceKind == FABRIC_SERVICE_KIND_STATEFUL)
    {
        serviceKind = FABRIC_SERVICE_KIND_STATEFUL;
        return ErrorCodeValue::Success;
    }
    else if (parsedServiceKind == FABRIC_SERVICE_KIND_STATELESS)
    {
        serviceKind = FABRIC_SERVICE_KIND_STATELESS;
        return ErrorCodeValue::Success;
    }
    else
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString("{0} {1}.", GET_NAMING_RC(Invalid_Service_Kind), serviceKindString));
    }
}

ErrorCode UriArgumentParser::TryGetApplicationDefinitionKindFilter(
    __out DWORD & applicationDefinitionKindFilter) const
{
    wstring definitionKindFilterString;
    applicationDefinitionKindFilter = FABRIC_APPLICATION_DEFINITION_KIND_FILTER_DEFAULT;

    if (uri_.GetItem(Constants::ApplicationDefinitionKindFilterString, definitionKindFilterString))
    {
        return Utility::TryParseQueryFilter(definitionKindFilterString, FABRIC_APPLICATION_DEFINITION_KIND_FILTER_DEFAULT, applicationDefinitionKindFilter);
    }
    else
    {
        return ErrorCodeValue::NameNotFound;
    }
}

ErrorCode UriArgumentParser::TryGetApplicationTypeDefinitionKindFilter(
    __out DWORD & definitionKindFilter) const
{
    wstring definitionKindFilterString;
    definitionKindFilter = FABRIC_APPLICATION_TYPE_DEFINITION_KIND_FILTER_DEFAULT;

    if (uri_.GetItem(Constants::ApplicationTypeDefinitionKindFilterString, definitionKindFilterString))
    {
        return Utility::TryParseQueryFilter(definitionKindFilterString, FABRIC_APPLICATION_TYPE_DEFINITION_KIND_FILTER_DEFAULT, definitionKindFilter);
    }
    else
    {
        return ErrorCodeValue::NameNotFound;
    }
}

ErrorCode UriArgumentParser::TryGetApplicationTypeVersion(
    __out wstring &appTypeVersion) const
{
    wstring temp;
    if (!uri_.GetItem(Constants::ApplicationTypeVersionString, temp))
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_HTTP_GATEWAY_RC(Missing_Required_Parameter), Constants::ApplicationTypeVersionString));
    }

    return NamingUri::UnescapeString(temp, appTypeVersion);
}

ErrorCode UriArgumentParser::TryGetApplicationTypeName(
    __out wstring & applicationTypeName) const
{
    if (!uri_.GetItem(Constants::ApplicationTypeNameString, applicationTypeName))
    {
        return ErrorCodeValue::ApplicationTypeNotFound;
    }

    return Utility::ValidateString(applicationTypeName);
}

ErrorCode UriArgumentParser::TryGetClusterManifestVersion(
    __out wstring &clusterManifestVersion) const
{
    wstring temp;
    if (!uri_.GetItem(Constants::ClusterManifestVersionString, temp))
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_HTTP_GATEWAY_RC(Missing_Required_Parameter), Constants::ClusterManifestVersionString));
    }

    clusterManifestVersion = move(temp);

    return ErrorCodeValue::Success;
}

ErrorCode UriArgumentParser::TryGetClusterConfigurationApiVersion(
    __out wstring &apiVersion) const
{
    wstring temp;
    if (!uri_.GetItem(Constants::ConfigurationApiVersionString, temp))
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_HTTP_GATEWAY_RC(Missing_Required_Parameter), Constants::ConfigurationApiVersionString));
    }

    apiVersion = move(temp);

    return ErrorCodeValue::Success;
}

ErrorCode UriArgumentParser::TryGetTestCommandProgressType(
    __out DWORD &progressType)
{
    wstring progressTypeString;
    progressType = FABRIC_QUERY_NODE_STATUS_FILTER_DEFAULT;

    if (uri_.GetItem(Constants::CommandType, progressTypeString))
    {
        wstring progressTypeIntString;
        if (progressTypeString.compare(Constants::TestCommandTypeDataLossString) == 0)
        {
            progressTypeIntString = *Constants::TestCommandTypeDataLoss;
        }
        else if (progressTypeString.compare(Constants::TestCommandTypeQuorumLossString) == 0)
        {
            progressTypeIntString = *Constants::TestCommandTypeQuorumLoss;
        }
        else if (progressTypeString.compare(Constants::TestCommandTypeRestartPartitionString) == 0)
        {
            progressTypeIntString = *Constants::TestCommandTypeRestartPartition;
        }
        else
        {
            return ErrorCodeValue::NameNotFound;
        }

        return Utility::TryParseQueryFilter(progressTypeIntString, FABRIC_TEST_COMMAND_TYPE_DEFAULT, progressType);
    }
    else
    {
        return ErrorCodeValue::NameNotFound;
    }
}

ErrorCode UriArgumentParser::TryGetStartTimeUtc(DateTime & startTimeUtc) const
{
    wstring value;
    if (!uri_.GetItem(Constants::StartTimeUtcString, value))
    {
        return ErrorCodeValue::NameNotFound;
    }

    int64 tickCount;
    if (!TryParseInt64(value, tickCount))
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_COMMON_RC(Invalid_Int64), value));
    }

    startTimeUtc = DateTime(tickCount);

    return ErrorCodeValue::Success;
}

ErrorCode UriArgumentParser::TryGetEndTimeUtc(DateTime & endTimeUtc) const
{
    wstring value;
    if (!uri_.GetItem(Constants::EndTimeUtcString, value))
    {
        return ErrorCodeValue::NameNotFound;
    }

    int64 tickCount;
    if (!TryParseInt64(value, tickCount))
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_COMMON_RC(Invalid_Int64), value));
    }

    endTimeUtc = DateTime(tickCount);

    return ErrorCodeValue::Success;
}

ErrorCode UriArgumentParser::TryGetUploadSessionId(
    __out Common::Guid & sessionId)
{
    wstring sessionIdString;
    if (!uri_.GetItem(Constants::SessionIdString, sessionIdString))
    {
        return ErrorCodeValue::Success;
    }

    return Common::Guid::TryParse(sessionIdString, sessionId) ? ErrorCodeValue::Success : ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_COMMON_RC(Invalid_Guid), sessionIdString));;
}

Common::ErrorCode UriArgumentParser::TryGetDataLossMode(
    __out DataLossMode::Enum& mode)
{
    wstring value;
    if (!uri_.GetItem(Constants::DataLossMode, value))
    {
        return ErrorCodeValue::NameNotFound;
    }

    if (value == *Constants::PartialDataLoss)
    {
        mode = DataLossMode::Enum::Partial;
    }
    else if (value == *Constants::FullDataLoss)
    {
        mode = DataLossMode::Enum::Full;
    }
    else
    {
        return ErrorCodeValue::InvalidArgument;
    }

    return ErrorCodeValue::Success;
}

Common::ErrorCode UriArgumentParser::TryGetQuorumLossMode(
  __out QuorumLossMode::Enum& mode)
{
    wstring value;
    if (!uri_.GetItem(Constants::QuorumLossMode, value))
    {
      return ErrorCodeValue::NameNotFound;
    }

    if (value == *Constants::QuorumLossReplicas)
    {
        mode = QuorumLossMode::Enum::QuorumReplicas;
    }
    else if (value == *Constants::AllReplicas)
    {
        mode = QuorumLossMode::Enum::AllReplicas;
    }
    else
    {
        return ErrorCodeValue::InvalidArgument;
    }

    return ErrorCodeValue::Success;
}

Common::ErrorCode UriArgumentParser::TryGetRestartPartitionMode(
  __out RestartPartitionMode::Enum& mode)
{
    wstring value;

    if (!uri_.GetItem(Constants::RestartPartitionMode, value))
    {
        return ErrorCodeValue::NameNotFound;
    }

    if (value == *Constants::AllReplicasOrInstances)
    {
        mode = RestartPartitionMode::Enum::AllReplicasOrInstances;
    }
    else if (value == *Constants::OnlyActiveSecondaries)
    {
        mode = RestartPartitionMode::Enum::OnlyActiveReplicas;
    }
    else
    {
        return ErrorCodeValue::InvalidArgument;
    }

    return ErrorCodeValue::Success;
}

ErrorCode UriArgumentParser::TryGetQuorumLossDuration(
    __out DWORD& duration)
{
      wstring value;
    if (!uri_.GetItem(Constants::QuorumLossDuration, value))
    {
        return ErrorCodeValue::NameNotFound;
    }

    if (!StringUtility::TryFromWString(value, duration))
    {
        return ErrorCodeValue::InvalidArgument;
    }

    return ErrorCodeValue::Success;
}

ErrorCode UriArgumentParser::TryGetNodeTransitionType(
    __out FABRIC_NODE_TRANSITION_TYPE &nodeTransitionType)
{
    wstring value;
    if (!uri_.GetItem(Constants::NodeTransitionType, value))
    {
        return ErrorCodeValue::NameNotFound;
    }

    if (value == *Constants::Start)
    {
        nodeTransitionType = FABRIC_NODE_TRANSITION_TYPE_START;
    }
    else if (value == *Constants::Stop)
    {
        nodeTransitionType = FABRIC_NODE_TRANSITION_TYPE_STOP;
    }
    else
    {
        return ErrorCodeValue::InvalidArgument;
    }

    return ErrorCodeValue::Success;
}

ErrorCode UriArgumentParser::TryGetStopDurationInSeconds(
    __out DWORD& duration)
{
    wstring value;
    if (!uri_.GetItem(Constants::StopDurationInSeconds, value))
    {
        return ErrorCodeValue::NameNotFound;
    }

    if (!StringUtility::TryFromWString(value, duration))
    {
        return ErrorCodeValue::InvalidArgument;
    }

    return ErrorCodeValue::Success;
}

ErrorCode UriArgumentParser::TryGetPropertyName(
    __out std::wstring & propertyName)
{
    std::wstring escapedPropertyName;
    if (!uri_.GetItem(Constants::PropertyNameString, escapedPropertyName))
    {
        return ErrorCodeValue::NameNotFound;
    }

    return NamingUri::UnescapeString(escapedPropertyName, propertyName);
}

ErrorCode UriArgumentParser::TryGetNetworkName(
    __out wstring & networkName)
{
    wstring escapedNetworkName;
    if (!uri_.GetItem(Constants::NetworkNameString, escapedNetworkName))
    {
        return ErrorCodeValue::NameNotFound;
    }

    return NamingUri::UnescapeString(escapedNetworkName, networkName);
}

ErrorCode UriArgumentParser::TryGetNetworkStatusFilter(
    __out DWORD & networkStatusFilter)
{
    wstring networkStatusFilterString;
    networkStatusFilter = FABRIC_NETWORK_STATUS_FILTER_DEFAULT;

    if (uri_.GetItem(Constants::NetworkStatusFilterString, networkStatusFilterString))
    {
        wstring networkStatusFilterValue;
        if (StringUtility::AreEqualCaseInsensitive(networkStatusFilterString, Constants::NetworkStatusFilterDefaultString))
        {
            networkStatusFilterValue = *Constants::NetworkStatusFilterDefault;
        }
        else if (StringUtility::AreEqualCaseInsensitive(networkStatusFilterString, Constants::NetworkStatusFilterAllString))
        {
            networkStatusFilterValue = *Constants::NetworkStatusFilterAll;
        }
        else if (StringUtility::AreEqualCaseInsensitive(networkStatusFilterString, Constants::NetworkStatusFilterReadyString))
        {
            networkStatusFilterValue = *Constants::NetworkStatusFilterReady;
        }
        else if (StringUtility::AreEqualCaseInsensitive(networkStatusFilterString, Constants::NetworkStatusFilterCreatingString))
        {
            networkStatusFilterValue = *Constants::NetworkStatusFilterCreating;
        }
        else if (StringUtility::AreEqualCaseInsensitive(networkStatusFilterString, Constants::NetworkStatusFilterDeletingString))
        {
            networkStatusFilterValue = *Constants::NetworkStatusFilterDeleting;
        }
        else if (StringUtility::AreEqualCaseInsensitive(networkStatusFilterString, Constants::NetworkStatusFilterUpdatingString))
        {
            networkStatusFilterValue = *Constants::NetworkStatusFilterUpdating;
        }
        else if (StringUtility::AreEqualCaseInsensitive(networkStatusFilterString, Constants::NetworkStatusFilterFailedString))
        {
            networkStatusFilterValue = *Constants::NetworkStatusFilterFailed;
        }

        return Utility::TryParseQueryFilter(networkStatusFilterValue, FABRIC_NETWORK_STATUS_FILTER_DEFAULT, networkStatusFilter);
    }
    else
    {
        return ErrorCodeValue::NameNotFound;
    }
}

Common::ErrorCode UriArgumentParser::TryGetIncludeSystemApplicationHealthStatisticsFilter(
    __inout bool & filter)
{
    return TryGetBoolOrError(Constants::IncludeSystemApplicationHealthStatisticsString, filter);
}

Common::ErrorCode UriArgumentParser::TryGetExcludeStatisticsFilter(
    __inout bool & filter)
{
    return TryGetBoolOrError(Constants::ExcludeHealthStatisticsString, filter);
}

ErrorCode UriArgumentParser::TryGetVolumeName(
    __out std::wstring & volumeName)
{
    std::wstring escapedVolumeName;
    if (!uri_.GetItem(Constants::VolumeNameString, escapedVolumeName))
    {
        return ErrorCodeValue::NameNotFound;
    }

    return NamingUri::UnescapeString(escapedVolumeName, volumeName);
}

ErrorCode UriArgumentParser::TryGetGatewayName(
    __out wstring & gatewayName)
{
    if (!uri_.GetItem(Constants::GatewayNameString, gatewayName))
    {
        return ErrorCodeValue::NameNotFound;
    }
    else
    {
        return Utility::ValidateString(gatewayName);
    }
}
