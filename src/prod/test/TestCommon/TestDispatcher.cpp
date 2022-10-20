// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace TestCommon
{
    using namespace std;
    using namespace Common;

    map<wstring, ErrorCodeValue::Enum> TestDispatcher::testErrorCodeMap;
    typedef pair <wstring, ErrorCodeValue::Enum> Type_Pair;

    ErrorCodeValue::Enum TestDispatcher::ParseErrorCode(wstring const & text)
    {
        ErrorCodeValue::Enum errorValue;
        if (TryParseErrorCode(text, errorValue))
        {
            return errorValue;
        }
        else
        {
            TestSession::FailTest(
                "Unrecognized/Unsupported (by TestDispatcher) ErrorCode: {0}.",
                text);
        }
    }

    bool TestDispatcher::TryParseErrorCode(wstring const & text, __out ErrorCodeValue::Enum & error)
    {
        if (testErrorCodeMap.size()==0)
        {
            AcquireExclusiveLock lock(mapLock_);             //The lock is intended to be put inside the check
            if (testErrorCodeMap.size()==0)                  //the duplicated checking is by design to eliminate race condition with low cost.
            {
                testErrorCodeMap.insert( Type_Pair(L"Success",                                  ErrorCodeValue::Success) );
                testErrorCodeMap.insert( Type_Pair(L"NameAlreadyExists",                        ErrorCodeValue::NameAlreadyExists) );
                testErrorCodeMap.insert( Type_Pair(L"NameNotEmpty",                             ErrorCodeValue::NameNotEmpty) );
                testErrorCodeMap.insert( Type_Pair(L"NameNotFound",                             ErrorCodeValue::NameNotFound) );
                testErrorCodeMap.insert( Type_Pair(L"PropertyNotFound",                         ErrorCodeValue::PropertyNotFound) );
                testErrorCodeMap.insert( Type_Pair(L"PropertyTooLarge",                         ErrorCodeValue::PropertyTooLarge) );
                testErrorCodeMap.insert( Type_Pair(L"UnsupportedNamingOperation",               ErrorCodeValue::UnsupportedNamingOperation) );
                testErrorCodeMap.insert( Type_Pair(L"UserServiceAlreadyExists",                 ErrorCodeValue::UserServiceAlreadyExists) );
                testErrorCodeMap.insert( Type_Pair(L"UserServiceGroupAlreadyExists",            ErrorCodeValue::UserServiceGroupAlreadyExists) );
                testErrorCodeMap.insert( Type_Pair(L"UserServiceNotFound",                      ErrorCodeValue::UserServiceNotFound) );
                testErrorCodeMap.insert( Type_Pair(L"UserServiceGroupNotFound",                 ErrorCodeValue::UserServiceGroupNotFound) );
                testErrorCodeMap.insert( Type_Pair(L"AccessDenied",                             ErrorCodeValue::AccessDenied) );
                testErrorCodeMap.insert( Type_Pair(L"InsufficientClusterCapacity",              ErrorCodeValue::InsufficientClusterCapacity) );
                testErrorCodeMap.insert( Type_Pair(L"ConstraintKeyUndefined",                   ErrorCodeValue::ConstraintKeyUndefined));
                testErrorCodeMap.insert( Type_Pair(L"ConstraintNotSatisfied",                   ErrorCodeValue::ConstraintNotSatisfied));
                testErrorCodeMap.insert( Type_Pair(L"InvalidArgument",                          ErrorCodeValue::InvalidArgument) );
                testErrorCodeMap.insert( Type_Pair(L"InvalidNameUri",                           ErrorCodeValue::InvalidNameUri) );
                testErrorCodeMap.insert( Type_Pair(L"InvalidServicePartition",                  ErrorCodeValue::InvalidServicePartition) );
                testErrorCodeMap.insert( Type_Pair(L"InvalidOperation",                         ErrorCodeValue::InvalidOperation) );
                testErrorCodeMap.insert( Type_Pair(L"ServiceOffline",                           ErrorCodeValue::ServiceOffline) );
                testErrorCodeMap.insert( Type_Pair(L"PartitionNotFound",                        ErrorCodeValue::PartitionNotFound) );
                testErrorCodeMap.insert( Type_Pair(L"Timeout",                                  ErrorCodeValue::Timeout) );
                testErrorCodeMap.insert( Type_Pair(L"FMNotReadyForUse",                         ErrorCodeValue::FMNotReadyForUse) );
                testErrorCodeMap.insert( Type_Pair(L"NoWriteQuorum",                            ErrorCodeValue::NoWriteQuorum) );
                testErrorCodeMap.insert( Type_Pair(L"OperationCanceled",                        ErrorCodeValue::OperationCanceled) );
                testErrorCodeMap.insert( Type_Pair(L"GatewayUnreachable",                       ErrorCodeValue::GatewayUnreachable) );
                testErrorCodeMap.insert( Type_Pair(L"OperationFailed",                          ErrorCodeValue::OperationFailed) );
                testErrorCodeMap.insert( Type_Pair(L"ImageBuilderValidationError",              ErrorCodeValue::ImageBuilderValidationError) );
                testErrorCodeMap.insert( Type_Pair(L"ImageBuilderUnexpectedError",              ErrorCodeValue::ImageBuilderUnexpectedError));
                testErrorCodeMap.insert( Type_Pair(L"ApplicationTypeNotFound",                  ErrorCodeValue::ApplicationTypeNotFound) );
                testErrorCodeMap.insert( Type_Pair(L"ApplicationTypeAlreadyExists",             ErrorCodeValue::ApplicationTypeAlreadyExists) );
                testErrorCodeMap.insert( Type_Pair(L"ApplicationTypeInUse",                     ErrorCodeValue::ApplicationTypeInUse) );
                testErrorCodeMap.insert( Type_Pair(L"ServiceTypeNotFound",                      ErrorCodeValue::ServiceTypeNotFound) );
                testErrorCodeMap.insert( Type_Pair(L"ServiceTypeMismatch",                      ErrorCodeValue::ServiceTypeMismatch) );
                testErrorCodeMap.insert( Type_Pair(L"FileNotFound",                             ErrorCodeValue::FileNotFound) );
                testErrorCodeMap.insert( Type_Pair(L"ServiceTypeTemplateNotFound",              ErrorCodeValue::ServiceTypeTemplateNotFound) );
                testErrorCodeMap.insert( Type_Pair(L"ApplicationNotFound",                      ErrorCodeValue::ApplicationNotFound) );
                testErrorCodeMap.insert( Type_Pair(L"ApplicationAlreadyExists",                 ErrorCodeValue::ApplicationAlreadyExists) );
                testErrorCodeMap.insert( Type_Pair(L"ApplicationUpgradeInProgress",             ErrorCodeValue::ApplicationUpgradeInProgress) );
                testErrorCodeMap.insert( Type_Pair(L"ApplicationNotUpgrading",                  ErrorCodeValue::ApplicationNotUpgrading) );
                testErrorCodeMap.insert( Type_Pair(L"ApplicationAlreadyInTargetVersion",        ErrorCodeValue::ApplicationAlreadyInTargetVersion) );
                testErrorCodeMap.insert( Type_Pair(L"ApplicationUpgradeValidationError",        ErrorCodeValue::ApplicationUpgradeValidationError) );
                testErrorCodeMap.insert( Type_Pair(L"PropertyCheckFailed",                      ErrorCodeValue::PropertyCheckFailed) );
                testErrorCodeMap.insert( Type_Pair(L"StoreWriteConflict",                       ErrorCodeValue::StoreWriteConflict) );
                testErrorCodeMap.insert( Type_Pair(L"ServiceMetadataMismatch",                  ErrorCodeValue::ServiceMetadataMismatch) );
                testErrorCodeMap.insert( Type_Pair(L"ServiceAffinityChainNotSupported",         ErrorCodeValue::ServiceAffinityChainNotSupported) );
                testErrorCodeMap.insert( Type_Pair(L"CommunicationError",                       ErrorCodeValue::GatewayUnreachable) );
                testErrorCodeMap.insert( Type_Pair(L"AddressAlreadyInUse",                      ErrorCodeValue::AddressAlreadyInUse) );
                testErrorCodeMap.insert( Type_Pair(L"RoutingNodeDoesNotMatchFault",             ErrorCodeValue::RoutingNodeDoesNotMatchFault));
                testErrorCodeMap.insert( Type_Pair(L"InvalidCredentials",                       ErrorCodeValue::InvalidCredentials) );
                testErrorCodeMap.insert( Type_Pair(L"InvalidCredentialType",                    ErrorCodeValue::InvalidCredentialType) );
                testErrorCodeMap.insert( Type_Pair(L"InvalidProtectionLevel",                   ErrorCodeValue::InvalidProtectionLevel) );
                testErrorCodeMap.insert( Type_Pair(L"InvalidAllowedCommonNameList",             ErrorCodeValue::InvalidAllowedCommonNameList) );
                testErrorCodeMap.insert( Type_Pair(L"FabricVersionNotFound",                    ErrorCodeValue::FabricVersionNotFound) );
                testErrorCodeMap.insert( Type_Pair(L"FabricVersionInUse",                       ErrorCodeValue::FabricVersionInUse) );
                testErrorCodeMap.insert( Type_Pair(L"FabricVersionAlreadyExists",               ErrorCodeValue::FabricVersionAlreadyExists) );
                testErrorCodeMap.insert( Type_Pair(L"FabricAlreadyInTargetVersion",             ErrorCodeValue::FabricAlreadyInTargetVersion) );
                testErrorCodeMap.insert( Type_Pair(L"FabricNotUpgrading",                       ErrorCodeValue::FabricNotUpgrading) );
                testErrorCodeMap.insert( Type_Pair(L"FabricUpgradeValidationError",             ErrorCodeValue::FabricUpgradeValidationError) );
                testErrorCodeMap.insert( Type_Pair(L"UpgradeDomainAlreadyCompleted",            ErrorCodeValue::UpgradeDomainAlreadyCompleted) );
                testErrorCodeMap.insert( Type_Pair(L"NodeNotFound",                             ErrorCodeValue::NodeNotFound) );
                testErrorCodeMap.insert( Type_Pair(L"HealthMaxReportsReached",                  ErrorCodeValue::HealthMaxReportsReached) );
                testErrorCodeMap.insert( Type_Pair(L"HealthStaleReport",                        ErrorCodeValue::HealthStaleReport) );
                testErrorCodeMap.insert( Type_Pair(L"InvalidState",                             ErrorCodeValue::InvalidState) );
                testErrorCodeMap.insert( Type_Pair(L"StaleRequest",                             ErrorCodeValue::StaleRequest) );
                testErrorCodeMap.insert( Type_Pair(L"HealthEntityDeleted",                      ErrorCodeValue::HealthEntityDeleted) );
                testErrorCodeMap.insert( Type_Pair(L"ServiceNotFound",                          ErrorCodeValue::ServiceNotFound) );
                testErrorCodeMap.insert( Type_Pair(L"ServiceTooBusy",                           ErrorCodeValue::ServiceTooBusy) );
                testErrorCodeMap.insert( Type_Pair(L"NotPrimary",                               ErrorCodeValue::NotPrimary) );
                testErrorCodeMap.insert( Type_Pair(L"NotReady",                                 ErrorCodeValue::NotReady) );
                testErrorCodeMap.insert( Type_Pair(L"REReplicaDoesNotExist",                    ErrorCodeValue::REReplicaDoesNotExist) );
                testErrorCodeMap.insert( Type_Pair(L"RepairTaskAlreadyExists",                  ErrorCodeValue::RepairTaskAlreadyExists) );
                testErrorCodeMap.insert( Type_Pair(L"RepairTaskNotFound",                       ErrorCodeValue::RepairTaskNotFound) );
                testErrorCodeMap.insert( Type_Pair(L"StoreSequenceNumberCheckFailed",           ErrorCodeValue::StoreSequenceNumberCheckFailed) );
                testErrorCodeMap.insert( Type_Pair(L"EntryTooLarge",                            ErrorCodeValue::EntryTooLarge));
                testErrorCodeMap.insert( Type_Pair(L"CertificateNotFound",                      ErrorCodeValue::CertificateNotFound));
                testErrorCodeMap.insert( Type_Pair(L"InvalidBackupSetting",                     ErrorCodeValue::InvalidBackupSetting));
                testErrorCodeMap.insert( Type_Pair(L"MissingFullBackup",                        ErrorCodeValue::MissingFullBackup));
                testErrorCodeMap.insert( Type_Pair(L"InvalidReplicaOperation",                  ErrorCodeValue::InvalidReplicaOperation));
                testErrorCodeMap.insert( Type_Pair(L"InvalidReplicaStateForReplicaOperation",   ErrorCodeValue::InvalidReplicaStateForReplicaOperation));
                testErrorCodeMap.insert( Type_Pair(L"InvalidPartitionOperation",                ErrorCodeValue::InvalidPartitionOperation));
                testErrorCodeMap.insert( Type_Pair(L"AlreadyPrimaryReplica",                    ErrorCodeValue::AlreadyPrimaryReplica));
                testErrorCodeMap.insert( Type_Pair(L"AlreadySecondaryReplica",                  ErrorCodeValue::AlreadySecondaryReplica));
                testErrorCodeMap.insert( Type_Pair(L"ForceNotSupportedForReplicaOperation",     ErrorCodeValue::ForceNotSupportedForReplicaOperation));
                testErrorCodeMap.insert( Type_Pair(L"InvalidCredentials",                       ErrorCodeValue::InvalidCredentials));
                testErrorCodeMap.insert( Type_Pair(L"ConnectionDenied",                         ErrorCodeValue::ConnectionDenied));
                testErrorCodeMap.insert( Type_Pair(L"ServerAuthenticationFailed",               ErrorCodeValue::ServerAuthenticationFailed));
                testErrorCodeMap.insert( Type_Pair(L"NodeIsUp",                                 ErrorCodeValue::NodeIsUp));
                testErrorCodeMap.insert( Type_Pair(L"DatabaseFilesCorrupted",                   ErrorCodeValue::DatabaseFilesCorrupted));
                testErrorCodeMap.insert( Type_Pair(L"CMOperationFailed",                        ErrorCodeValue::CMOperationFailed));
                testErrorCodeMap.insert( Type_Pair(L"REQueueFull",                              ErrorCodeValue::REQueueFull));
                testErrorCodeMap.insert( Type_Pair(L"DnsNameInUse",                             ErrorCodeValue::DnsNameInUse));
                testErrorCodeMap.insert( Type_Pair(L"InvalidDnsName",                           ErrorCodeValue::InvalidDnsName));
                testErrorCodeMap.insert( Type_Pair(L"DirectoryNotFound",                        ErrorCodeValue::DirectoryNotFound));
                testErrorCodeMap.insert( Type_Pair(L"ComposeDeploymentAlreadyExists",           ErrorCodeValue::ComposeDeploymentAlreadyExists));
                testErrorCodeMap.insert( Type_Pair(L"ComposeDeploymentNotFound",                ErrorCodeValue::ComposeDeploymentNotFound));
                testErrorCodeMap.insert(Type_Pair(L"ComposeDeploymentNotUpgrading",             ErrorCodeValue::ComposeDeploymentNotUpgrading));
                testErrorCodeMap.insert( Type_Pair(L"InvalidAddress",                           ErrorCodeValue::InvalidAddress));
                testErrorCodeMap.insert( Type_Pair(L"InvalidServiceScalingPolicy",              ErrorCodeValue::InvalidServiceScalingPolicy));
                testErrorCodeMap.insert( Type_Pair(L"NetworkNotFound",                          ErrorCodeValue::NetworkNotFound));
                testErrorCodeMap.insert( Type_Pair(L"NetworkInUse",                             ErrorCodeValue::NetworkInUse));
            }
        }

        auto itor = testErrorCodeMap.find(text);
        if( itor != testErrorCodeMap.end() )
        {
            error = itor->second;
            return true;
        }
        else
        {
            TestSession::WriteError(
                "dispatch",
                "Unrecognized/Unsupported (by TestDispatcher) ErrorCode: {0}.",
                text);
            return false;
        }
    }

    StringCollection TestDispatcher::CompactParameters(StringCollection const & params)
    {
        StringCollection compactedParams;
        bool foundStartingQuote = false;
        wstring currentParam;
        for (wstring const & param : params)
        {
            if (foundStartingQuote)
            {
                if (param[param.size() - 1] == L'"')
                {
                    foundStartingQuote = false;
                    currentParam += L" " + param.substr(0, param.size() - 1);
                    compactedParams.push_back(currentParam);
                }
                else
                {
                    currentParam += L" " + param;
                }
            }
            else if (param[0] == L'"')
            {
                if (param[param.size() - 1] == L'"')
                {
                    compactedParams.push_back(param.substr(1, param.size() - 2));
                }
                else
                {
                    foundStartingQuote = true;
                    currentParam = param.substr(1, param.size() - 1);
                }
            }
            else
            {
                compactedParams.push_back(param);
            }
        }
        return compactedParams;
    }
}
