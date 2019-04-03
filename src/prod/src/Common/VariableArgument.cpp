// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

StringLiteral TrueString("true");
StringLiteral FalseString("false");
StringLiteral NullPtrString("nullptr");

VariableArgument::VariableArgument()
    : type_(TypeInvalid)
{
}

VariableArgument::VariableArgument(bool value)
    : type_(TypeBool)
{
    value_.valueBool_ = value;
}

VariableArgument::VariableArgument(int64 value)
    : type_(TypeSignedNumber)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(int32 value)
    : type_(TypeSignedNumber)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(int16 value)
    : type_(TypeSignedNumber)
{
    value_.valueInt64_ = value;
}

#ifndef PLATFORM_UNIX
VariableArgument::VariableArgument(LONG value)
    : type_(TypeSignedNumber)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(uint32 value)
    : type_(TypeUnsignedNumber)
{
    value_.valueUInt64_ = value;
}
#endif

VariableArgument::VariableArgument(uint64 value)
    : type_(TypeUnsignedNumber)
{
    value_.valueUInt64_ = value;
}

VariableArgument::VariableArgument(uint16 value)
    : type_(TypeUnsignedNumber)
{
    value_.valueUInt64_ = value;
}

VariableArgument::VariableArgument(UCHAR value)
    : type_(TypeUnsignedNumber)
{
    value_.valueUInt64_ = value;
}

VariableArgument::VariableArgument(DWORD value)
    : type_(TypeUnsignedNumber)
{
    value_.valueUInt64_ = value;
}

VariableArgument::VariableArgument(LargeInteger const & value)
    : type_(TypeLargeInteger)
{
    value_.valueLargeInteger_ = &value;
}

VariableArgument::VariableArgument(Guid const & value)
    : type_(TypeGuid)
{
    value_.valueGuid_ = &value;
}

VariableArgument::VariableArgument(float value)
    : type_(TypeDouble)
{
    value_.valueDouble_ = value;
}

VariableArgument::VariableArgument(double value)
    : type_(TypeDouble)
{
    value_.valueDouble_ = value;
}

VariableArgument::VariableArgument(DateTime value)
    : type_(TypeDateTime)
{
    value_.valueInt64_ = value.Ticks;
}

VariableArgument::VariableArgument(StopwatchTime value)
    : type_(TypeStopwatchTime)
{
    value_.valueInt64_ = value.Ticks;
}

VariableArgument::VariableArgument(TimeSpan value)
    : type_(TypeTimeSpan)
{
    value_.valueInt64_ = value.Ticks;
}

VariableArgument::VariableArgument(char value)
    : type_(TypeChar)
{
    value_.valueChar_ = value;
}

VariableArgument::VariableArgument(wchar_t value)
    : type_(TypeWChar)
{
    value_.valueWChar_ = value;
}

VariableArgument::VariableArgument(char const* value)
    : type_(TypeString)
{
    value_.valueString_.size_ = strlen(value);
    value_.valueString_.buffer_ = value;
}

_Use_decl_annotations_
VariableArgument::VariableArgument(char * value)
    : type_(TypeString)
{
    value_.valueString_.size_ = strlen(value);
    value_.valueString_.buffer_ = value;
}

VariableArgument::VariableArgument(wchar_t const* value)
    : type_(TypeWString)
{
    value_.valueWString_.size_ = wcslen(value);
    value_.valueWString_.buffer_ = value;
}

_Use_decl_annotations_
VariableArgument::VariableArgument(wchar_t * value)
    : type_(TypeWString)
{
    value_.valueWString_.size_ = wcslen(value);
    value_.valueWString_.buffer_ = value;
}

VariableArgument::VariableArgument(string const & value)
    : type_(TypeString)
{
    value_.valueString_.size_ = value.size();
    value_.valueString_.buffer_ = value.c_str();
}

VariableArgument::VariableArgument(wstring const & value)
    : type_(TypeWString)
{
    value_.valueWString_.size_ = value.size();
    value_.valueWString_.buffer_ = value.c_str();
}

VariableArgument::VariableArgument(literal_holder<char> const & value)
    : type_(TypeString)
{
    value_.valueString_.size_ = value.size();
    value_.valueString_.buffer_ = value.begin();
}

VariableArgument::VariableArgument(literal_holder<wchar_t> const & value)
    : type_(TypeWString)
{
    value_.valueWString_.size_ = value.size();
    value_.valueWString_.buffer_ = value.begin();
}

VariableArgument::VariableArgument(GlobalWString const & value)
    : type_(TypeWString)
{
    value_.valueWString_.size_ = value->size();
    value_.valueWString_.buffer_ = value->c_str();
}

VariableArgument::VariableArgument(StringCollection const & value)
    : type_(TypeStringCollection)
{
    value_.valueStringCollection_ = &value;
}

VariableArgument::VariableArgument(FABRIC_EPOCH const & value)
    : type_(Type_FABRIC_EPOCH)
{
    value_.value_FABRIC_EPOCH_ = &value;
}

VariableArgument::VariableArgument(FABRIC_OPERATION_METADATA const & value)
    : type_(Type_FABRIC_OPERATION_METADATA)
{
    value_.value_FABRIC_OPERATION_METADATA_ = &value;
}

VariableArgument::VariableArgument(FABRIC_OPERATION_TYPE value)
    : type_(Type_FABRIC_OPERATION_TYPE)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_SERVICE_PARTITION_ACCESS_STATUS value)
    : type_(Type_FABRIC_SERVICE_PARTITION_ACCESS_STATUS)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_REPLICA_SET_QUORUM_MODE value)
    : type_(Type_FABRIC_REPLICA_SET_QUORUM_MODE)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_REPLICA_ROLE value)
    : type_(Type_FABRIC_REPLICA_ROLE)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_HEALTH_STATE value)
    : type_(Type_FABRIC_HEALTH_STATE)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_HEALTH_REPORT_KIND value)
    : type_(Type_FABRIC_HEALTH_REPORT_KIND)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_HEALTH_EVALUATION_KIND value)
    : type_(Type_FABRIC_HEALTH_EVALUATION_KIND)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_QUERY_SERVICE_PARTITION_STATUS value)
    : type_(Type_FABRIC_QUERY_SERVICE_PARTITION_STATUS)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_QUERY_NODE_STATUS value)
    : type_(Type_FABRIC_QUERY_NODE_STATUS)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_PARTITION_KEY_TYPE value)
    : type_(Type_FABRIC_PARTITION_KEY_TYPE)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_SERVICE_PARTITION_KIND value)
    : type_(Type_FABRIC_SERVICE_PARTITION_KIND)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_SERVICE_LOAD_METRIC_WEIGHT value)
    : type_(Type_FABRIC_SERVICE_LOAD_METRIC_WEIGHT)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_SERVICE_CORRELATION_SCHEME value)
    : type_(Type_FABRIC_SERVICE_CORRELATION_SCHEME)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_FAULT_TYPE value)
    : type_(Type_FABRIC_FAULT_TYPE)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_NODE_DEACTIVATION_INTENT value)
    : type_(Type_FABRIC_NODE_DEACTIVATION_INTENT)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_SERVICE_DESCRIPTION_KIND value)
    : type_(Type_FABRIC_SERVICE_DESCRIPTION_KIND)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_PARTITION_SCHEME value)
    : type_(Type_FABRIC_PARTITION_SCHEME)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_PROPERTY_TYPE_ID value)
    : type_(Type_FABRIC_PROPERTY_TYPE_ID)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_PROPERTY_BATCH_OPERATION_KIND value)
    : type_(Type_FABRIC_PROPERTY_BATCH_OPERATION_KIND)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_QUERY_SERVICE_STATUS value)
    : type_(Type_FABRIC_QUERY_SERVICE_STATUS)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_QUERY_SERVICE_OPERATION_NAME value)
    : type_(Type_FABRIC_QUERY_SERVICE_OPERATION_NAME)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_QUERY_REPLICATOR_OPERATION_NAME value)
    : type_(Type_FABRIC_QUERY_REPLICATOR_OPERATION_NAME)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_SERVICE_KIND value)
    : type_(Type_FABRIC_SERVICE_KIND)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_HEALTH_STATE_FILTER value)
    : type_(Type_FABRIC_HEALTH_STATE_FILTER)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_NODE_DEACTIVATION_TASK_TYPE value)
    : type_(Type_FABRIC_NODE_DEACTIVATION_TASK_TYPE)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_TEST_COMMAND_PROGRESS_STATE value)
    : type_(Type_FABRIC_TEST_COMMAND_PROGRESS_STATE)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_TEST_COMMAND_TYPE value)
    : type_(Type_FABRIC_TEST_COMMAND_TYPE)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_PACKAGE_SHARING_POLICY_SCOPE value)
: type_(Type_FABRIC_PACKAGE_SHARING_POLICY_SCOPE)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_SERVICE_ENDPOINT_ROLE value)
: type_(Type_FABRIC_SERVICE_ENDPOINT_ROLE)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_APPLICATION_UPGRADE_STATE value)
: type_(Type_FABRIC_APPLICATION_UPGRADE_STATE)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_ROLLING_UPGRADE_MODE value)
: type_(Type_FABRIC_ROLLING_UPGRADE_MODE)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_UPGRADE_FAILURE_REASON value)
: type_(Type_FABRIC_UPGRADE_FAILURE_REASON)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_UPGRADE_DOMAIN_STATE value)
: type_(Type_FABRIC_UPGRADE_DOMAIN_STATE)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_UPGRADE_STATE value)
: type_(Type_FABRIC_UPGRADE_STATE)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_SERVICE_TYPE_REGISTRATION_STATUS value)
: type_(Type_FABRIC_SERVICE_TYPE_REGISTRATION_STATUS)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_QUERY_SERVICE_REPLICA_STATUS value)
: type_(Type_FABRIC_QUERY_SERVICE_REPLICA_STATUS)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_UPGRADE_KIND value)
: type_(Type_FABRIC_UPGRADE_KIND)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_FLAGS value)
: type_(Type_FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_FLAGS)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_MOVE_COST value)
: type_(Type_FABRIC_MOVE_COST)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_NODE_DEACTIVATION_STATUS value)
: type_(Type_FABRIC_NODE_DEACTIVATION_STATUS)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_PLACEMENT_POLICY_TYPE value)
: type_(Type_FABRIC_PLACEMENT_POLICY_TYPE)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_SERVICE_REPLICA_KIND value)
: type_(Type_FABRIC_SERVICE_REPLICA_KIND)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_PARTITION_SELECTOR_TYPE value)
    : type_(Type_FABRIC_PARTITION_SELECTOR_TYPE)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_DATA_LOSS_MODE value)
    : type_(Type_FABRIC_DATA_LOSS_MODE)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_QUORUM_LOSS_MODE value)
    : type_(Type_FABRIC_QUORUM_LOSS_MODE)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_RESTART_PARTITION_MODE value)
    : type_(Type_FABRIC_RESTART_PARTITION_MODE)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_NODE_TRANSITION_TYPE value)
    : type_(Type_FABRIC_NODE_TRANSITION_TYPE)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_CHAOS_EVENT_KIND value)
    : type_(Type_FABRIC_CHAOS_EVENT_KIND)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_CHAOS_STATUS value)
: type_(Type_FABRIC_CHAOS_STATUS)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_RECONFIGURATION_PHASE value)
    : type_(Type_FABRIC_RECONFIGURATION_PHASE)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_RECONFIGURATION_TYPE value)
    : type_(Type_FABRIC_RECONFIGURATION_TYPE)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_PROVISION_APPLICATION_TYPE_KIND value)
    : type_(Type_FABRIC_PROVISION_APPLICATION_TYPE_KIND)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_DIAGNOSTICS_SINKS_KIND value)
    : type_(Type_FABRIC_DIAGNOSTICS_SINKS_KIND)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_NETWORK_TYPE value)
    : type_(Type_FABRIC_NETWORK_TYPE)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(FABRIC_NETWORK_STATUS value)
    : type_(Type_FABRIC_NETWORK_STATUS)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(XmlNodeType value)
    : type_(Type_XmlNodeType)
{
    value_.valueInt64_ = value;
}

VariableArgument::VariableArgument(ErrorCode const & value)
    : type_(TypeErrorCode)
{
    value_.valueInt64_ = static_cast<int64>(value.ReadValue());
}

VariableArgument::VariableArgument(std::exception const & value)
    : type_(TypeStdException)
{
    value_.valueStdException_ = &value;
}

VariableArgument::VariableArgument(std::error_category const & value)
    : type_(TypeStdErrorCategory)
{
    value_.valueStdErrorCategory_ = &value;
}

VariableArgument::VariableArgument(std::error_code const & value)
    : type_(TypeStdErrorCode)
{
    value_.valueStdErrorCode_ = &value;
}

VariableArgument::VariableArgument(std::system_error const & value)
    : type_(TypeStdSystemError)
{
    value_.valueStdSystemError_ = &value;
}

bool VariableArgument::IsValid() const
{
    return (type_ != TypeInvalid);
}

void VariableArgument::WriteTo(TextWriter& w, FormatOptions const & format) const
{
    switch (type_)
    {
    case TypeInplaceObject:
        {
            ITextWritable* p = (ITextWritable*)(&value_);
            p->WriteTo(w, format);
        }
        break;
    case TypeBool:
        {
        StringLiteral const & result = (value_.valueBool_ ? TrueString : FalseString);
        w.WriteBuffer(result.begin(), result.size());
        }
        break;
    case TypeSignedNumber:
        w.WriteNumber(value_.valueUInt64_, format, value_.valueInt64_ < 0);
        break;
    case TypeUnsignedNumber:
        w.WriteNumber(value_.valueUInt64_, format, false);
        break;
    case TypeLargeInteger:
        value_.valueLargeInteger_->WriteTo(w, format);
        break;
    case TypeGuid:
        value_.valueGuid_->WriteTo(w, format);
        break;
    case TypeDouble:
        w.WriteString(std::to_string(static_cast<long double>(value_.valueDouble_)));
        break;
    case TypeDateTime:
        DateTime(value_.valueInt64_).WriteTo(w, format);
        break;
    case TypeStopwatchTime:
        StopwatchTime(value_.valueInt64_).WriteTo(w, format);
        break;
    case TypeTimeSpan:
        TimeSpan::FromTicks(value_.valueInt64_).WriteTo(w, format);
        break;
    case TypeChar:
        w.WriteBuffer(&value_.valueChar_, 1);
        break;
    case TypeWChar:
        w.WriteBuffer(&value_.valueWChar_, 1);
        break;
    case TypeString:
        w.WriteBuffer(value_.valueString_.buffer_, value_.valueString_.size_);
        break;
    case TypeWString:
        w.WriteBuffer(value_.valueWString_.buffer_, value_.valueWString_.size_);
        break;
    case TypeStringCollection:
        w.Write(TextWritableCollection<StringCollection>(*value_.valueStringCollection_));
        break;
    case TypePointer:
        if (value_.valuePointer_)
        {
            w.WriteNumber((uint64)(value_.valuePointer_), FormatOptions(0,0,"x"), false);
        }
        else
        {
            w.Write(NullPtrString);
        }
        break;
    case Type_FABRIC_EPOCH:
        w.Write("{0}.{1:X}", value_.value_FABRIC_EPOCH_->DataLossNumber, value_.value_FABRIC_EPOCH_->ConfigurationNumber);
        break;
    case Type_FABRIC_OPERATION_METADATA:
        w << value_.value_FABRIC_OPERATION_METADATA_->Type << L"." << value_.value_FABRIC_OPERATION_METADATA_->SequenceNumber;
        break;
    case Type_FABRIC_OPERATION_TYPE:
        switch (value_.valueInt64_)
        {
            case FABRIC_OPERATION_TYPE_NORMAL:
                w << "FABRIC_OPERATION_TYPE_NORMAL";
                break;
            default:
                w << "UNDEFINED FABRIC_OPERATION_TYPE=" << value_.valueInt64_;
        }
        break;
    case Type_FABRIC_SERVICE_PARTITION_ACCESS_STATUS:
        switch (value_.valueInt64_)
        {
            case FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING:
                w << "FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING";
                break;
            case FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY:
                w << "FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY";
                break;
            case FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NO_WRITE_QUORUM:
                w << "FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NO_WRITE_QUORUM";
                break;
            case FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED:
                w << "FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED";
                break;
            default:
                w << "UNDEFINED FABRIC_SERVICE_PARTITION_ACCESS_STATUS=" << value_.valueInt64_;
        }
        break;
    case Type_FABRIC_REPLICA_SET_QUORUM_MODE:
        switch (value_.valueInt64_)
        {
            case FABRIC_REPLICA_SET_QUORUM_ALL:
                w << "FABRIC_REPLICA_SET_QUORUM_ALL";
                break;
            case FABRIC_REPLICA_SET_WRITE_QUORUM:
                w << "FABRIC_REPLICA_SET_WRITE_QUORUM";
                break;
            default:
                w << "UNDEFINED FABRIC_REPLICA_SET_QUORUM_MODE=" << value_.valueInt64_;
        }
        break;
    case Type_FABRIC_REPLICA_ROLE:
        switch (value_.valueInt64_)
        {
            case FABRIC_REPLICA_ROLE_UNKNOWN:
                w << "Unknown";
                break;
            case FABRIC_REPLICA_ROLE_NONE:
                w << "None";
                break;
            case FABRIC_REPLICA_ROLE_PRIMARY:
                w << "Primary";
                break;
            case FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY:
                w << "ActiveSecondary";
                break;
            case FABRIC_REPLICA_ROLE_IDLE_SECONDARY:
                w << "IdleSecondary";
                break;
            default:
                w << "UNDEFINED FABRIC_REPLICA_ROLE=" << value_.valueInt64_;
        }
        break;
    case Type_FABRIC_HEALTH_STATE:
        switch (value_.valueInt64_)
        {
            case FABRIC_HEALTH_STATE_INVALID:
                w << "Invalid";
                break;
            case FABRIC_HEALTH_STATE_OK:
                w << "OK";
                break;
            case FABRIC_HEALTH_STATE_WARNING:
                w << "Warning";
                break;
            case FABRIC_HEALTH_STATE_ERROR:
                w << "Error";
                break;
            case FABRIC_HEALTH_STATE_UNKNOWN:
                w << "Unknown";
                break;
            default:
                w << "UNDEFINED FABRIC_HEALTH_STATE=" << value_.valueInt64_;
        }
        break;
    case Type_FABRIC_HEALTH_REPORT_KIND:
        switch (value_.valueInt64_)
        {
            case FABRIC_HEALTH_REPORT_KIND_STATELESS_SERVICE_INSTANCE:
                w << "Instance";
                break;
            case FABRIC_HEALTH_REPORT_KIND_STATEFUL_SERVICE_REPLICA:
                w << "Replica";
                break;
            case FABRIC_HEALTH_REPORT_KIND_PARTITION:
                w << "Partition";
                break;
            case FABRIC_HEALTH_REPORT_KIND_NODE:
                w << "Node";
                break;
            case FABRIC_HEALTH_REPORT_KIND_SERVICE:
                w << "Service";
                break;
            case FABRIC_HEALTH_REPORT_KIND_APPLICATION:
                w << "Application";
                break;
            case FABRIC_HEALTH_REPORT_KIND_DEPLOYED_APPLICATION:
                w << "DeployedApplication";
                break;
            case FABRIC_HEALTH_REPORT_KIND_DEPLOYED_SERVICE_PACKAGE:
                w << "DeployedServicePackage";
                break;
            case FABRIC_HEALTH_REPORT_KIND_CLUSTER:
                w << "Cluster";
                break;
            default:
                w << "UNDEFINED FABRIC_HEALTH_REPORT_KIND=" << value_.valueInt64_;
        }
        break;
    case Type_FABRIC_HEALTH_EVALUATION_KIND:
        switch (value_.valueInt64_)
        {
            case FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DELTA_NODES_CHECK:
                w << "HealthEvaluationKindUpgradeDomainDeltaNodesCheck";
                break;
            case FABRIC_HEALTH_EVALUATION_KIND_DELTA_NODES_CHECK:
                w << "HealthEvaluationKindDeltaNodesCheck";
                break;
            case FABRIC_HEALTH_EVALUATION_KIND_EVENT:
                w << "EventHealthEvaluation";
                break;
            case FABRIC_HEALTH_EVALUATION_KIND_REPLICAS:
                w << "ReplicasHealthEvaluation";
                break;
            case FABRIC_HEALTH_EVALUATION_KIND_PARTITIONS:
                w << "PartitionsHealthEvaluation";
                break;
            case FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_SERVICE_PACKAGES:
                w << "DeployedServicePackagesHealthEvaluation";
                break;
            case FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_APPLICATIONS:
                w << "DeployedApplicationsHealthEvaluation";
                break;
            case FABRIC_HEALTH_EVALUATION_KIND_SERVICES:
                w << "ServicesHealthEvaluation";
                break;
            case FABRIC_HEALTH_EVALUATION_KIND_NODES:
                w << "NodesHealthEvaluation";
                break;
            case FABRIC_HEALTH_EVALUATION_KIND_APPLICATIONS:
                w << "ApplicationsHealthEvaluation";
                break;
            case FABRIC_HEALTH_EVALUATION_KIND_SYSTEM_APPLICATION:
                w << "SystemApplicationHealthEvaluation";
                break;
            case FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DEPLOYED_APPLICATIONS:
                w << "UDDeployedApplicationsHealthEvaluation";
                break;
            case FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_NODES:
                w << "UDNodesHealthEvaluation";
                break;
            case FABRIC_HEALTH_EVALUATION_KIND_REPLICA:
                w << "ReplicaHealthEvaluation";
                break;
            case FABRIC_HEALTH_EVALUATION_KIND_PARTITION:
                w << "PartitionHealthEvaluation";
                break;
            case FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_SERVICE_PACKAGE:
                w << "DeployedServicePackageHealthEvaluation";
                break;
            case FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_APPLICATION:
                w << "DeployedApplicationHealthEvaluation";
                break;
            case FABRIC_HEALTH_EVALUATION_KIND_SERVICE:
                w << "ServiceHealthEvaluation";
                break;
            case FABRIC_HEALTH_EVALUATION_KIND_NODE:
                w << "NodeHealthEvaluation";
                break;
            case FABRIC_HEALTH_EVALUATION_KIND_APPLICATION:
                w << "ApplicationHealthEvaluation";
                break;
            case FABRIC_HEALTH_EVALUATION_KIND_APPLICATION_TYPE_APPLICATIONS:
                w << "ApplicationTypeApplicationsHealthEvaluation";
                break;
            default:
                w << "UNDEFINED FABRIC_HEALTH_EVALUATION_KIND=" << value_.valueInt64_;
        }
        break;
    case Type_FABRIC_QUERY_SERVICE_PARTITION_STATUS:
        switch (value_.valueInt64_)
        {
            case FABRIC_QUERY_SERVICE_PARTITION_STATUS_READY:
                w << "Ready";
                break;
            case FABRIC_QUERY_SERVICE_PARTITION_STATUS_NOT_READY:
                w << "NotReady";
                break;
            case FABRIC_QUERY_SERVICE_PARTITION_STATUS_IN_QUORUM_LOSS:
                w << "InQuorumLoss";
                break;
            case FABRIC_QUERY_SERVICE_PARTITION_STATUS_RECONFIGURING:
                w << "Reconfiguring";
                break;
            case FABRIC_QUERY_SERVICE_PARTITION_STATUS_DELETING:
                w << "Deleting";
                break;
            default:
                w << "UNDEFINED FABRIC_QUERY_SERVICE_PARTITION_STATUS = " << value_.valueInt64_;
        }
        break;
    case Type_FABRIC_QUERY_NODE_STATUS:
        switch (value_.valueInt64_)
        {
            case FABRIC_QUERY_NODE_STATUS_UP:
                w << "Up";
                break;
            case FABRIC_QUERY_NODE_STATUS_DOWN:
                w << "Down";
                break;
            case FABRIC_QUERY_NODE_STATUS_ENABLING:
                w << "Enabling";
                break;
            case FABRIC_QUERY_NODE_STATUS_DISABLING:
                w << "Disabling";
                break;
            case FABRIC_QUERY_NODE_STATUS_DISABLED:
                w << "Disabled";
                break;
            case FABRIC_QUERY_NODE_STATUS_UNKNOWN:
                w << "Unknown";
                break;
            case FABRIC_QUERY_NODE_STATUS_REMOVED:
                w << "Removed";
                break;
            default:
                w << "UNDEFINED FABRIC_QUERY_NODE_STATUS = " << value_.valueInt64_;
        }
        break;
    case Type_FABRIC_PARTITION_KEY_TYPE:
        switch (value_.valueInt64_)
        {
            case FABRIC_PARTITION_KEY_TYPE_INVALID:
                w << "FABRIC_PARTITION_KEY_TYPE_INVALID";
                break;
            case FABRIC_PARTITION_KEY_TYPE_NONE:
                w << "FABRIC_PARTITION_KEY_TYPE_NONE";
                break;
            case FABRIC_PARTITION_KEY_TYPE_INT64:
                w << "FABRIC_PARTITION_KEY_TYPE_INT64";
                break;
            case FABRIC_PARTITION_KEY_TYPE_STRING:
                w << "FABRIC_PARTITION_KEY_TYPE_STRING";
                break;
            default:
                w << "UNDEFINED FABRIC_PARTITION_KEY_TYPE=" << value_.valueInt64_;
        }
        break;
    case Type_FABRIC_SERVICE_PARTITION_KIND:
        switch (value_.valueInt64_)
        {
            case FABRIC_SERVICE_PARTITION_KIND_INVALID:
                w << "FABRIC_SERVICE_PARTITION_KIND_INVALID";
                break;
            case FABRIC_SERVICE_PARTITION_KIND_SINGLETON:
                w << "FABRIC_SERVICE_PARTITION_KIND_SINGLETON";
                break;
            case FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE:
                w << "FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE";
                break;
            case FABRIC_SERVICE_PARTITION_KIND_NAMED:
                w << "FABRIC_SERVICE_PARTITION_KIND_NAMED";
                break;
            default:
                w << "UNDEFINED FABRIC_SERVICE_PARTITION_KIND=" << value_.valueInt64_;
        }
        break;
    case Type_FABRIC_SERVICE_LOAD_METRIC_WEIGHT:
        switch (value_.valueInt64_)
        {
            case FABRIC_SERVICE_LOAD_METRIC_WEIGHT_ZERO:
                w << "FABRIC_SERVICE_LOAD_METRIC_WEIGHT_ZERO";
                break;
            case FABRIC_SERVICE_LOAD_METRIC_WEIGHT_LOW:
                w << "FABRIC_SERVICE_LOAD_METRIC_WEIGHT_LOW";
                break;
            case FABRIC_SERVICE_LOAD_METRIC_WEIGHT_MEDIUM:
                w << "FABRIC_SERVICE_LOAD_METRIC_WEIGHT_MEDIUM";
                break;
            case FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH:
                w << "FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH";
                break;
            default:
                w << "UNDEFINED FABRIC_SERVICE_LOAD_METRIC_WEIGHT=" << value_.valueInt64_;
        }
        break;
    case Type_FABRIC_SERVICE_CORRELATION_SCHEME:
        switch (value_.valueInt64_)
        {
            case FABRIC_SERVICE_CORRELATION_SCHEME_INVALID:
                w << "FABRIC_SERVICE_CORRELATION_SCHEME_INVALID";
                break;
            case FABRIC_SERVICE_CORRELATION_SCHEME_AFFINITY:
                w << "FABRIC_SERVICE_CORRELATION_SCHEME_AFFINITY";
                break;
            case FABRIC_SERVICE_CORRELATION_SCHEME_ALIGNED_AFFINITY:
                w << "FABRIC_SERVICE_CORRELATION_SCHEME_ALIGNED_AFFINITY";
                break;
            case FABRIC_SERVICE_CORRELATION_SCHEME_NONALIGNED_AFFINITY:
                w << "FABRIC_SERVICE_CORRELATION_SCHEME_NONALIGNED_AFFINITY";
                break;
            default:
                w << "UNDEFINED FABRIC_SERVICE_CORRELATION_SCHEME=" << value_.valueInt64_;
        }
        break;
    case Type_FABRIC_FAULT_TYPE:
        switch (value_.valueInt64_)
        {
            case FABRIC_FAULT_TYPE_INVALID:
                w << "FABRIC_FAULT_TYPE_INVALID";
                break;
            case FABRIC_FAULT_TYPE_PERMANENT:
                w << "FABRIC_FAULT_TYPE_PERMANENT";
                break;
            case FABRIC_FAULT_TYPE_TRANSIENT:
                w << "FABRIC_FAULT_TYPE_TRANSIENT";
                break;
            default:
                w << "UNDEFINED FABRIC_FAULT_TYPE=" << value_.valueInt64_;
        }
        break;
    case Type_FABRIC_NODE_DEACTIVATION_INTENT:
        switch (value_.valueInt64_)
        {
            case FABRIC_NODE_DEACTIVATION_INTENT_INVALID:
                w << "FABRIC_NODE_DEACTIVATION_INTENT_INVALID";
                break;
            case FABRIC_NODE_DEACTIVATION_INTENT_PAUSE:
                w << "FABRIC_NODE_DEACTIVATION_INTENT_PAUSE";
                break;
            case FABRIC_NODE_DEACTIVATION_INTENT_RESTART:
                w << "FABRIC_NODE_DEACTIVATION_INTENT_RESTART";
                break;
            case FABRIC_NODE_DEACTIVATION_INTENT_REMOVE_DATA:
                w << "FABRIC_NODE_DEACTIVATION_INTENT_REMOVE_DATA";
                break;
            case FABRIC_NODE_DEACTIVATION_INTENT_REMOVE_NODE:
                w << "FABRIC_NODE_DEACTIVATION_INTENT_REMOVE_NODE";
                break;
            default:
                w << "Undefined FABRIC_NODE_DEACTIVATION_INTENT = " << value_.valueInt64_;
        }
        break;
    case Type_FABRIC_SERVICE_DESCRIPTION_KIND:
        switch (value_.valueInt64_)
        {
            case FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS:
                w << "FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS";
                break;
            case FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL:
                w << "FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL";
                break;
            default:
                w << "UNDEFINED FABRIC_SERVICE_DESCRIPTION_KIND=" << value_.valueInt64_;
        }
        break;
    case Type_FABRIC_PARTITION_SCHEME:
        switch (value_.valueInt64_)
        {
            case FABRIC_PARTITION_SCHEME_INVALID:
                w << "FABRIC_PARTITION_SCHEME_INVALID";
                break;
            case FABRIC_PARTITION_SCHEME_SINGLETON:
                w << "FABRIC_PARTITION_SCHEME_SINGLETON";
                break;
            case FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE:
                w << "FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE";
                break;
            case FABRIC_PARTITION_SCHEME_NAMED:
                w << "FABRIC_PARTITION_SCHEME_NAMED";
                break;
            default:
                w << "UNDEFINED FABRIC_PARTITION_SCHEME=" << value_.valueInt64_;
        }
        break;
    case Type_FABRIC_PROPERTY_TYPE_ID:
        switch (value_.valueInt64_)
        {
            case FABRIC_PROPERTY_TYPE_INVALID:
                w << "FABRIC_PROPERTY_TYPE_INVALID";
                break;
            case FABRIC_PROPERTY_TYPE_BINARY:
                w << "FABRIC_PROPERTY_TYPE_BINARY";
                break;
            case FABRIC_PROPERTY_TYPE_INT64:
                w << "FABRIC_PROPERTY_TYPE_INT64";
                break;
            case FABRIC_PROPERTY_TYPE_DOUBLE:
                w << "FABRIC_PROPERTY_TYPE_DOUBLE";
                break;
            case FABRIC_PROPERTY_TYPE_WSTRING:
                w << "FABRIC_PROPERTY_TYPE_WSTRING";
                break;
            case FABRIC_PROPERTY_TYPE_GUID:
                w << "FABRIC_PROPERTY_TYPE_GUID";
                break;
            default:
                w << "UNDEFINED FABRIC_PROPERTY_TYPE_ID=" << value_.valueInt64_;
        }
        break;
    case Type_FABRIC_PROPERTY_BATCH_OPERATION_KIND:
        switch (value_.valueInt64_)
        {
            case FABRIC_PROPERTY_BATCH_OPERATION_KIND_INVALID:
                w << "FABRIC_PROPERTY_BATCH_OPERATION_KIND_INVALID";
                break;
            case FABRIC_PROPERTY_BATCH_OPERATION_KIND_PUT:
                w << "FABRIC_PROPERTY_BATCH_OPERATION_KIND_PUT";
                break;
            case FABRIC_PROPERTY_BATCH_OPERATION_KIND_GET:
                w << "FABRIC_PROPERTY_BATCH_OPERATION_KIND_GET";
                break;
            case FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_EXISTS:
                w << "FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_EXISTS";
                break;
            case FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_SEQUENCE:
                w << "FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_SEQUENCE";
                break;
            case FABRIC_PROPERTY_BATCH_OPERATION_KIND_DELETE:
                w << "FABRIC_PROPERTY_BATCH_OPERATION_KIND_DELETE";
                break;
            case FABRIC_PROPERTY_BATCH_OPERATION_KIND_PUT_CUSTOM:
                w << "FABRIC_PROPERTY_BATCH_OPERATION_KIND_PUT_CUSTOM";
                break;
            default:
                w << "UNDEFINED FABRIC_PROPERTY_BATCH_OPERATION_KIND=" << value_.valueInt64_;
        }
        break;
    case Type_FABRIC_QUERY_SERVICE_STATUS:
        switch (value_.valueInt64_)
        {
        case FABRIC_QUERY_SERVICE_STATUS_UNKNOWN:
            w << "Unknown";
            break;
        case FABRIC_QUERY_SERVICE_STATUS_ACTIVE:
            w << "Active";
            break;
        case FABRIC_QUERY_SERVICE_STATUS_UPGRADING:
            w << "Upgrading";
            break;
        case FABRIC_QUERY_SERVICE_STATUS_DELETING:
            w << "Deleting";
            break;
        case FABRIC_QUERY_SERVICE_STATUS_CREATING:
            w << "Creating";
            break;
        case FABRIC_QUERY_SERVICE_STATUS_FAILED:
            w << "Failed";
            break;
        default:
            w << "Undefined FABRIC_QUERY_SERVICE_STATUS=" << value_.valueInt64_;
        }
        break;
    case Type_FABRIC_QUERY_SERVICE_OPERATION_NAME:
        switch (value_.valueInt64_)
        {
        case FABRIC_QUERY_SERVICE_OPERATION_NAME_INVALID:
            w << "FABRIC_QUERY_SERVICE_OPERATION_NAME_INVALID";
            break;

        case FABRIC_QUERY_SERVICE_OPERATION_NAME_NONE:
            w << "FABRIC_QUERY_SERVICE_OPERATION_NAME_NONE";
            break;

        case FABRIC_QUERY_SERVICE_OPERATION_NAME_OPEN:
            w << "FABRIC_QUERY_SERVICE_OPERATION_NAME_OPEN";
            break;

        case FABRIC_QUERY_SERVICE_OPERATION_NAME_CHANGEROLE:
            w << "FABRIC_QUERY_SERVICE_OPERATION_NAME_CHANGEROLE";
            break;

        case FABRIC_QUERY_SERVICE_OPERATION_NAME_CLOSE:
            w << "FABRIC_QUERY_SERVICE_OPERATION_NAME_CLOSE";
            break;

        case FABRIC_QUERY_SERVICE_OPERATION_NAME_ABORT:
            w << "FABRIC_QUERY_SERVICE_OPERATION_NAME_ABORT";
            break;

        default:
            w << "Undefined Type_FABRIC_QUERY_SERVICE_OPERATION_NAME=" << value_.valueInt64_;
        }
        break;

    case Type_FABRIC_QUERY_REPLICATOR_OPERATION_NAME:
        switch (value_.valueInt64_)
        {
        case FABRIC_QUERY_REPLICATOR_OPERATION_NAME_INVALID:
            w << "FABRIC_QUERY_REPLICATOR_OPERATION_NAME_INVALID";
            break;

        case FABRIC_QUERY_REPLICATOR_OPERATION_NAME_NONE:
            w << "FABRIC_QUERY_REPLICATOR_OPERATION_NAME_NONE";
            break;

        case FABRIC_QUERY_REPLICATOR_OPERATION_NAME_OPEN:
            w << "FABRIC_QUERY_REPLICATOR_OPERATION_NAME_OPEN";
            break;

        case FABRIC_QUERY_REPLICATOR_OPERATION_NAME_CHANGEROLE:
            w << "FABRIC_QUERY_REPLICATOR_OPERATION_NAME_CHANGEROLE";
            break;

        case FABRIC_QUERY_REPLICATOR_OPERATION_NAME_UPDATEEPOCH:
            w << "FABRIC_QUERY_REPLICATOR_OPERATION_NAME_UPDATEEPOCH";
            break;

        case FABRIC_QUERY_REPLICATOR_OPERATION_NAME_CLOSE:
            w << "FABRIC_QUERY_REPLICATOR_OPERATION_NAME_CLOSE";
            break;

        case FABRIC_QUERY_REPLICATOR_OPERATION_NAME_ABORT:
            w << "FABRIC_QUERY_REPLICATOR_OPERATION_NAME_ABORT";
            break;

        case FABRIC_QUERY_REPLICATOR_OPERATION_NAME_ONDATALOSS:
            w << "FABRIC_QUERY_REPLICATOR_OPERATION_NAME_ONDATALOSS";
            break;

        case FABRIC_QUERY_REPLICATOR_OPERATION_NAME_WAITFORCATCHUP:
            w << "FABRIC_QUERY_REPLICATOR_OPERATION_NAME_WAITFORCATCHUP";
            break;

        case FABRIC_QUERY_REPLICATOR_OPERATION_NAME_BUILD:
            w << "FABRIC_QUERY_REPLICATOR_OPERATION_NAME_BUILD";
            break;

        default:
            w << "Undefined Type_FABRIC_QUERY_REPLICATOR_OPERATION_NAME=" << value_.valueInt64_;
        }
        break;

    case Type_FABRIC_HEALTH_STATE_FILTER:
        switch (value_.valueInt64_)
        {
            case FABRIC_HEALTH_STATE_FILTER_DEFAULT:
                w << "HealthStateFilterDefault";
                break;
            case FABRIC_HEALTH_STATE_FILTER_NONE:
                w << "HealthStateFilterNone";
                break;
            case FABRIC_HEALTH_STATE_FILTER_OK:
                w << "HealthStateFilterOk";
                break;
            case FABRIC_HEALTH_STATE_FILTER_WARNING:
                w << "HealthStateFilterWarning";
                break;
            case FABRIC_HEALTH_STATE_FILTER_ERROR:
                w << "HealthStateFilterError";
                break;
            case FABRIC_HEALTH_STATE_FILTER_ALL:
                w << "HealthStateFilterAll";
                break;
           default:
                w << "UNDEFINED FABRIC_HEALTH_STATE_FILTER=" << value_.valueInt64_;
               break;
        }
        break;

    case Type_FABRIC_NODE_DEACTIVATION_TASK_TYPE:
        switch (value_.valueInt64_)
        {
        case FABRIC_NODE_DEACTIVATION_TASK_TYPE_INVALID:
            w << "Invalid";
            break;
        case FABRIC_NODE_DEACTIVATION_TASK_TYPE_INFRASTRUCTURE:
            w << "Infrastructure";
            break;
        case FABRIC_NODE_DEACTIVATION_TASK_TYPE_REPAIR:
            w << "Repair";
            break;
        case FABRIC_NODE_DEACTIVATION_TASK_TYPE_CLIENT:
            w << "Client";
            break;
        default:
            w << "Undefined FABRIC_NODE_DEACTIVATION_TASK_TYPE=" << value_.valueInt64_;
        }
        break;

    case Type_FABRIC_SERVICE_KIND:
        switch (value_.valueInt64_)
        {
        case FABRIC_SERVICE_KIND_INVALID:
            w << "FABRIC_SERVICE_KIND_INVALID";
            break;

        case FABRIC_SERVICE_KIND_STATELESS:
            w << "FABRIC_SERVICE_KIND_STATELESS";
            break;

        case FABRIC_SERVICE_KIND_STATEFUL:
            w << "FABRIC_SERVICE_KIND_STATEFUL";
            break;

        default:
            w << "Undefined FABRIC_SERVICE_KIND_STATEFUL=" << value_.valueInt64_;
        }
        break;

    case Type_FABRIC_TEST_COMMAND_PROGRESS_STATE:
        switch (value_.valueInt64_)
        {
            case FABRIC_TEST_COMMAND_PROGRESS_STATE_INVALID:
                w << "FABRIC_TEST_COMMAND_PROGRESS_STATE_INVALID";
                break;
            case FABRIC_TEST_COMMAND_PROGRESS_STATE_RUNNING:
                w << "FABRIC_TEST_COMMAND_PROGRESS_STATE_RUNNING";
                break;
            case FABRIC_TEST_COMMAND_PROGRESS_STATE_ROLLING_BACK:
                w << "FABRIC_TEST_COMMAND_PROGRESS_STATE_ROLLING_BACK";
                break;
            case FABRIC_TEST_COMMAND_PROGRESS_STATE_COMPLETED:
                w << "FABRIC_TEST_COMMAND_PROGRESS_STATE_COMPLETED";
                break;
            case FABRIC_TEST_COMMAND_PROGRESS_STATE_FAULTED:
                w << "FABRIC_TEST_COMMAND_PROGRESS_STATE_FAULTED";
                break;
            case FABRIC_TEST_COMMAND_PROGRESS_STATE_CANCELLED:
                w << "FABRIC_TEST_COMMAND_PROGRESS_STATE_CANCELLED";
                break;
            case FABRIC_TEST_COMMAND_PROGRESS_STATE_FORCE_CANCELLED:
                w << "FABRIC_TEST_COMMAND_PROGRESS_STATE_FORCE_CANCELLED";
                break;
            default:
                w << "Undefined FABRIC_TEST_COMMAND_PROGRESS_STATE = " << value_.valueInt64_;
        }
        break;

    case Type_FABRIC_TEST_COMMAND_TYPE:
        switch (value_.valueInt64_)
        {
            case FABRIC_TEST_COMMAND_TYPE_DEFAULT:
                w << "FABRIC_TEST_COMMAND_TYPE_DEFAULT";
                break;
            case FABRIC_TEST_COMMAND_TYPE_INVOKE_DATA_LOSS:
                w << "FABRIC_TEST_COMMAND_TYPE_INVOKE_DATA_LOSS";
                break;
            case FABRIC_TEST_COMMAND_TYPE_INVOKE_QUORUM_LOSS:
                w << "FABRIC_TEST_COMMAND_TYPE_INVOKE_QUORUM_LOSS";
                break;
            case FABRIC_TEST_COMMAND_TYPE_INVOKE_RESTART_PARTITION:
                w << "FABRIC_TEST_COMMAND_TYPE_INVOKE_RESTART_PARTITION";
                break;
            case FABRIC_TEST_COMMAND_TYPE_START_NODE_TRANSITION:
                w << "FABRIC_TEST_COMMAND_TYPE_START_NODE_TRANSITION";
                break;
            default:
                w << "Undefined FABRIC_TEST_COMMAND_TYPE = " << value_.valueInt64_;
        }
        break;

    case Type_FABRIC_PACKAGE_SHARING_POLICY_SCOPE:
        switch (value_.valueInt64_)
        {
            case FABRIC_PACKAGE_SHARING_POLICY_SCOPE_NONE:
                w << "FABRIC_PACKAGE_SHARING_POLICY_SCOPE_NONE";
                break;
            case FABRIC_PACKAGE_SHARING_POLICY_SCOPE_ALL:
                w << "FABRIC_PACKAGE_SHARING_POLICY_SCOPE_ALL";
                break;
            case FABRIC_PACKAGE_SHARING_POLICY_SCOPE_CODE:
                w << "FABRIC_PACKAGE_SHARING_POLICY_SCOPE_CODE";
                break;
            case FABRIC_PACKAGE_SHARING_POLICY_SCOPE_CONFIG:
                w << "FABRIC_PACKAGE_SHARING_POLICY_SCOPE_CONFIG";
                break;
            case FABRIC_PACKAGE_SHARING_POLICY_SCOPE_DATA:
                w << "FABRIC_PACKAGE_SHARING_POLICY_SCOPE_DATA";
                break;
            default:
                w << "Undefined FABRIC_PACKAGE_SHARING_POLICY_SCOPE = " << value_.valueInt64_;
        }
        break;

    case Type_FABRIC_SERVICE_ENDPOINT_ROLE:
        switch (value_.valueInt64_)
        {
            case FABRIC_SERVICE_ROLE_INVALID:
                w << "FABRIC_SERVICE_ROLE_INVALID";
                break;
            case FABRIC_SERVICE_ROLE_STATELESS:
                w << "FABRIC_SERVICE_ROLE_STATELESS";
                break;
            case FABRIC_SERVICE_ROLE_STATEFUL_PRIMARY:
                w << "FABRIC_SERVICE_ROLE_STATEFUL_PRIMARY";
                break;
            case FABRIC_SERVICE_ROLE_STATEFUL_SECONDARY:
                w << "FABRIC_SERVICE_ROLE_STATEFUL_SECONDARY";
                break;
            default:
                w << "Undefined FABRIC_SERVICE_ENDPOINT_ROLE = " << value_.valueInt64_;
        }
        break;

    case Type_FABRIC_APPLICATION_UPGRADE_STATE:
        switch (value_.valueInt64_)
        {
            case FABRIC_APPLICATION_UPGRADE_STATE_INVALID:
                w << "FABRIC_APPLICATION_UPGRADE_STATE_INVALID";
                break;
            case FABRIC_APPLICATION_UPGRADE_STATE_ROLLING_BACK_IN_PROGRESS:
                w << "FABRIC_APPLICATION_UPGRADE_STATE_ROLLING_BACK_IN_PROGRESS";
                break;
            case FABRIC_APPLICATION_UPGRADE_STATE_ROLLING_BACK_COMPLETED:
                w << "FABRIC_APPLICATION_UPGRADE_STATE_ROLLING_BACK_COMPLETED";
                break;
            case FABRIC_APPLICATION_UPGRADE_STATE_ROLLING_FORWARD_PENDING:
                w << "FABRIC_APPLICATION_UPGRADE_STATE_ROLLING_FORWARD_PENDING";
                break;
            case FABRIC_APPLICATION_UPGRADE_STATE_ROLLING_FORWARD_IN_PROGRESS:
                w << "FABRIC_APPLICATION_UPGRADE_STATE_ROLLING_FORWARD_IN_PROGRESS";
                break;
            case FABRIC_APPLICATION_UPGRADE_STATE_ROLLING_FORWARD_COMPLETED:
                w << "FABRIC_APPLICATION_UPGRADE_STATE_ROLLING_FORWARD_COMPLETED";
                break;
            case FABRIC_APPLICATION_UPGRADE_STATE_FAILED:
                w << "FABRIC_APPLICATION_UPGRADE_STATE_FAILED";
                break;
            case FABRIC_APPLICATION_UPGRADE_STATE_ROLLING_BACK_PENDING:
                w << "FABRIC_APPLICATION_UPGRADE_STATE_ROLLING_BACK_PENDING";
                break;
            default:
                w << "Undefined FABRIC_APPLICATION_UPGRADE_STATE = " << value_.valueInt64_;
        }
        break;

    case Type_FABRIC_ROLLING_UPGRADE_MODE:
        switch (value_.valueInt64_)
        {
        case FABRIC_ROLLING_UPGRADE_MODE_INVALID:
            w << "FABRIC_ROLLING_UPGRADE_MODE_INVALID";
            break;
        case FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_AUTO:
            w << "FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_AUTO";
            break;
        case FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_MANUAL:
            w << "FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_MANUAL";
            break;
        case FABRIC_ROLLING_UPGRADE_MODE_MONITORED:
            w << "FABRIC_ROLLING_UPGRADE_MODE_MONITORED";
            break;
        default:
            w << "Undefined FABRIC_ROLLING_UPGRADE_MODE = " << value_.valueInt64_;
        }
        break;

    case Type_FABRIC_UPGRADE_DOMAIN_STATE:
        switch (value_.valueInt64_)
        {
        case FABRIC_UPGRADE_DOMAIN_STATE_INVALID:
            w << "FABRIC_UPGRADE_DOMAIN_STATE_INVALID";
            break;
        case FABRIC_UPGRADE_DOMAIN_STATE_PENDING:
            w << "FABRIC_UPGRADE_DOMAIN_STATE_PENDING";
            break;
        case FABRIC_UPGRADE_DOMAIN_STATE_IN_PROGRESS:
            w << "FABRIC_UPGRADE_DOMAIN_STATE_IN_PROGRESS";
            break;
        case FABRIC_UPGRADE_DOMAIN_STATE_COMPLETED:
            w << "FABRIC_UPGRADE_DOMAIN_STATE_COMPLETED";
            break;
        default:
            w << "Undefined FABRIC_UPGRADE_DOMAIN_STATE = " << value_.valueInt64_;
        }
        break;

    case Type_FABRIC_UPGRADE_STATE:
        switch (value_.valueInt64_)
        {
        case FABRIC_UPGRADE_STATE_INVALID:
            w << "FABRIC_UPGRADE_STATE_INVALID";
            break;
        case FABRIC_UPGRADE_STATE_ROLLING_BACK_IN_PROGRESS:
            w << "FABRIC_UPGRADE_STATE_ROLLING_BACK_IN_PROGRESS";
            break;
        case FABRIC_UPGRADE_STATE_ROLLING_BACK_COMPLETED:
            w << "FABRIC_UPGRADE_STATE_ROLLING_BACK_COMPLETED";
            break;
        case FABRIC_UPGRADE_STATE_ROLLING_FORWARD_PENDING:
            w << "FABRIC_UPGRADE_STATE_ROLLING_FORWARD_PENDING";
            break;
        case FABRIC_UPGRADE_STATE_ROLLING_FORWARD_IN_PROGRESS:
            w << "FABRIC_UPGRADE_STATE_ROLLING_FORWARD_IN_PROGRESS";
            break;
        case FABRIC_UPGRADE_STATE_ROLLING_FORWARD_COMPLETED:
            w << "FABRIC_UPGRADE_STATE_ROLLING_FORWARD_COMPLETED";
            break;
        case FABRIC_UPGRADE_STATE_FAILED:
            w << "FABRIC_UPGRADE_STATE_FAILED";
            break;
        case FABRIC_UPGRADE_STATE_ROLLING_BACK_PENDING:
            w << "FABRIC_UPGRADE_STATE_ROLLING_BACK_PENDING";
            break;
        default:
            w << "Undefined FABRIC_UPGRADE_STATE = " << value_.valueInt64_;
        }
        break;

    case Type_FABRIC_UPGRADE_FAILURE_REASON:
        switch (value_.valueInt64_)
        {
        case FABRIC_UPGRADE_FAILURE_REASON_NONE:
            w << "FABRIC_UPGRADE_FAILURE_REASON_NONE";
            break;
        case FABRIC_UPGRADE_FAILURE_REASON_INTERRUPTED:
            w << "FABRIC_UPGRADE_FAILURE_REASON_INTERRUPTED";
            break;
        case FABRIC_UPGRADE_FAILURE_REASON_HEALTH_CHECK:
            w << "FABRIC_UPGRADE_FAILURE_REASON_HEALTH_CHECK";
            break;
        case FABRIC_UPGRADE_FAILURE_REASON_UPGRADE_DOMAIN_TIMEOUT:
            w << "FABRIC_UPGRADE_FAILURE_REASON_UPGRADE_DOMAIN_TIMEOUT";
            break;
        case FABRIC_UPGRADE_FAILURE_REASON_OVERALL_UPGRADE_TIMEOUT:
            w << "FABRIC_UPGRADE_FAILURE_REASON_OVERALL_UPGRADE_TIMEOUT";
            break;
        case FABRIC_UPGRADE_FAILURE_REASON_PROCESSING_FAILURE:
            w << "FABRIC_UPGRADE_FAILURE_REASON_PROCESSING_FAILURE";
            break;
        default:
            w << "Undefined Type_FABRIC_UPGRADE_FAILURE_REASON = " << value_.valueInt64_;
        }
        break;

    case Type_FABRIC_SERVICE_TYPE_REGISTRATION_STATUS:
        switch (value_.valueInt64_)
        {
        case FABRIC_SERVICE_TYPE_REGISTRATION_STATUS_INVALID:
            w << "FABRIC_SERVICE_TYPE_REGISTRATION_STATUS_INVALID";
            break;
        case FABRIC_SERVICE_TYPE_REGISTRATION_STATUS_DISABLED:
            w << "FABRIC_SERVICE_TYPE_REGISTRATION_STATUS_DISABLED";
            break;
        case FABRIC_SERVICE_TYPE_REGISTRATION_STATUS_NOT_REGISTERED:
            w << "FABRIC_SERVICE_TYPE_REGISTRATION_STATUS_NOT_REGISTERED";
            break;
        case FABRIC_SERVICE_TYPE_REGISTRATION_STATUS_REGISTERED:
            w << "FABRIC_SERVICE_TYPE_REGISTRATION_STATUS_REGISTERED";
            break;
        default:
            w << "Undefined FABRIC_SERVICE_TYPE_REGISTRATION_STATUS = " << value_.valueInt64_;
        }
        break;

    case Type_FABRIC_QUERY_SERVICE_REPLICA_STATUS:
        switch (value_.valueInt64_)
        {
        case FABRIC_QUERY_SERVICE_REPLICA_STATUS_INVALID:
            w << "Invalid";
            break;
        case FABRIC_QUERY_SERVICE_REPLICA_STATUS_INBUILD:
            w << "InBuild";
            break;
        case FABRIC_QUERY_SERVICE_REPLICA_STATUS_STANDBY:
            w << "Standby";
            break;
        case FABRIC_QUERY_SERVICE_REPLICA_STATUS_READY:
            w << "Ready";
            break;
        case FABRIC_QUERY_SERVICE_REPLICA_STATUS_DOWN:
            w << "Down";
            break;
        case FABRIC_QUERY_SERVICE_REPLICA_STATUS_DROPPED:
            w << "Dropped";
            break;
        default:
            w << "Undefined FABRIC_QUERY_SERVICE_REPLICA_STATUS = " << value_.valueInt64_;
        }
        break;

    case Type_FABRIC_UPGRADE_KIND:
        switch (value_.valueInt64_)
        {
        case FABRIC_UPGRADE_KIND_INVALID:
            w << "FABRIC_UPGRADE_KIND_INVALID";
            break;
        case FABRIC_UPGRADE_KIND_ROLLING:
            w << "FABRIC_UPGRADE_KIND_ROLLING";
            break;
        default:
            w << "Undefined FABRIC_UPGRADE_KIND = " << value_.valueInt64_;
        }
        break;

    case Type_FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_FLAGS:
    {
        bool isFirstVal = true;
        if (value_.valueInt64_ & FABRIC_STATEFUL_SERVICE_SETTINGS_NONE) {
            w << "FABRIC_STATEFUL_SERVICE_SETTINGS_NONE";
            isFirstVal = false;
        }
        if (value_.valueInt64_ & FABRIC_STATEFUL_SERVICE_SETTINGS_REPLICA_RESTART_WAIT_DURATION) {
            if (!isFirstVal) {
                w << "|";
            }
            w << "FABRIC_STATEFUL_SERVICE_SETTINGS_REPLICA_RESTART_WAIT_DURATION";
            isFirstVal = false;
        }
        if (value_.valueInt64_ & FABRIC_STATEFUL_SERVICE_SETTINGS_QUORUM_LOSS_WAIT_DURATION) {
            if (!isFirstVal) {
                w << "|";
            }
            w << "FABRIC_STATEFUL_SERVICE_SETTINGS_QUORUM_LOSS_WAIT_DURATION";
            isFirstVal = false;
        }
        if (value_.valueInt64_ & FABRIC_STATEFUL_SERVICE_SETTINGS_STANDBY_REPLICA_KEEP_DURATION) {
            if (!isFirstVal) {
                w << "|";
            }
            w << "FABRIC_STATEFUL_SERVICE_SETTINGS_STANDBY_REPLICA_KEEP_DURATION";
            isFirstVal = false;
        }
        break;
    }
    case Type_FABRIC_MOVE_COST:
        switch (value_.valueInt64_)
        {
        case FABRIC_MOVE_COST_ZERO:
            w << "FABRIC_MOVE_COST_ZERO";
            break;
        case FABRIC_MOVE_COST_LOW:
            w << "FABRIC_MOVE_COST_LOW";
            break;
        case FABRIC_MOVE_COST_MEDIUM:
            w << "FABRIC_MOVE_COST_MEDIUM";
            break;
        case FABRIC_MOVE_COST_HIGH:
            w << "FABRIC_MOVE_COST_HIGH";
            break;
        default:
            w << "Undefined FABRIC_MOVE_COST = " << value_.valueInt64_;
        }
        break;

    case Type_FABRIC_NODE_DEACTIVATION_STATUS:
        switch (value_.valueInt64_)
        {
        case FABRIC_NODE_DEACTIVATION_STATUS_NONE:
            w << "FABRIC_NODE_DEACTIVATION_STATUS_NONE";
            break;
        case FABRIC_NODE_DEACTIVATION_STATUS_SAFETY_CHECK_IN_PROGRESS:
            w << "FABRIC_NODE_DEACTIVATION_STATUS_SAFETY_CHECK_IN_PROGRESS";
            break;
        case FABRIC_NODE_DEACTIVATION_STATUS_SAFETY_CHECK_COMPLETE:
            w << "FABRIC_NODE_DEACTIVATION_STATUS_SAFETY_CHECK_COMPLETE";
            break;
        case FABRIC_NODE_DEACTIVATION_STATUS_COMPLETED:
            w << "FABRIC_NODE_DEACTIVATION_STATUS_COMPLETED";
            break;
        default:
            w << "Undefined FABRIC_NODE_DEACTIVATION_STATUS = " << value_.valueInt64_;
        }
        break;

    case Type_FABRIC_PLACEMENT_POLICY_TYPE:
        switch (value_.valueInt64_)
        {
        case FABRIC_PLACEMENT_POLICY_INVALID:
            w << "FABRIC_PLACEMENT_POLICY_INVALID";
            break;
        case FABRIC_PLACEMENT_POLICY_INVALID_DOMAIN:
            w << "FABRIC_PLACEMENT_POLICY_INVALID_DOMAIN";
            break;
        case FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN:
            w << "FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN";
            break;
        case FABRIC_PLACEMENT_POLICY_PREFERRED_PRIMARY_DOMAIN:
            w << "FABRIC_PLACEMENT_POLICY_PREFERRED_PRIMARY_DOMAIN";
            break;
        case FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN_DISTRIBUTION:
            w << "FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN_DISTRIBUTION";
            break;
        case FABRIC_PLACEMENT_POLICY_NONPARTIALLY_PLACE_SERVICE:
            w << "FABRIC_PLACEMENT_POLICY_NONPARTIALLY_PLACE_SERVICE";
            break;
        default:
            w << "Undefined FABRIC_PLACEMENT_POLICY_TYPE = " << value_.valueInt64_;
        }
        break;

    case Type_FABRIC_SERVICE_REPLICA_KIND:
        switch (value_.valueInt64_)
        {
        case FABRIC_SERVICE_REPLICA_KIND_INVALID:
            w << "FABRIC_SERVICE_REPLICA_KIND_INVALID";
            break;
        case FABRIC_SERVICE_REPLICA_KIND_KEY_VALUE_STORE:
            w << "FABRIC_SERVICE_REPLICA_KIND_KEY_VALUE_STORE";
            break;
        default:
            w << "Undefined FABRIC_SERVICE_REPLICA_KIND = " << value_.valueInt64_;
        }
        break;

    case Type_FABRIC_PARTITION_SELECTOR_TYPE:
        switch (value_.valueInt64_)
        {
            case FABRIC_PARTITION_SELECTOR_TYPE_NONE:
                w << "FABRIC_PARTITION_SELECTOR_TYPE_NONE";
                break;
            case FABRIC_PARTITION_SELECTOR_TYPE_SINGLETON:
                w << "FABRIC_PARTITION_SELECTOR_TYPE_SINGLETON";
                break;
            case FABRIC_PARTITION_SELECTOR_TYPE_NAMED:
                w << "FABRIC_PARTITION_SELECTOR_TYPE_NAMED";
                break;
            case FABRIC_PARTITION_SELECTOR_TYPE_UNIFORM_INT64:
                w << "FABRIC_PARTITION_SELECTOR_TYPE_UNIFORM_INT64";
                break;
            case FABRIC_PARTITION_SELECTOR_TYPE_PARTITION_ID:
                w << "FABRIC_PARTITION_SELECTOR_TYPE_PARTITION_ID";
                break;
            case FABRIC_PARTITION_SELECTOR_TYPE_RANDOM:
                w << "FABRIC_PARTITION_SELECTOR_TYPE_RANDOM";
                break;
            default:
                w << "Undefined FABRIC_NODE_DEACTIVATION_INTENT = " << value_.valueInt64_;
        }
        break;

    case Type_FABRIC_DATA_LOSS_MODE:
        switch (value_.valueInt64_)
        {
            case FABRIC_DATA_LOSS_MODE_INVALID:
                w << "FABRIC_DATA_LOSS_MODE_INVALID";
                break;
            case FABRIC_DATA_LOSS_MODE_PARTIAL:
                w << "FABRIC_DATA_LOSS_MODE_PARTIAL";
                break;
            case FABRIC_DATA_LOSS_MODE_FULL:
                w << "FABRIC_DATA_LOSS_MODE_FULL";
                break;
            default:
                w << "Undefined FABRIC_DATA_LOSS_MODE = " << value_.valueInt64_;
        }
        break;
    case Type_FABRIC_QUORUM_LOSS_MODE:
        switch (value_.valueInt64_)
        {
            case FABRIC_QUORUM_LOSS_MODE_INVALID:
                w << "FABRIC_QUORUM_LOSS_MODE_INVALID";
                break;
            case FABRIC_QUORUM_LOSS_MODE_QUORUM_REPLICAS:
                w << "FABRIC_QUORUM_LOSS_MODE_QUORUM_REPLICAS";
                break;
            case FABRIC_QUORUM_LOSS_MODE_ALL_REPLICAS:
                w << "FABRIC_QUORUM_LOSS_MODE_ALL_REPLICAS";
                break;
            default:
                w << "Undefined FABRIC_QUORUM_LOSS_MODE = " << value_.valueInt64_;
        }
        break;
    case Type_FABRIC_RESTART_PARTITION_MODE:
        switch (value_.valueInt64_)
        {
            case FABRIC_RESTART_PARTITION_MODE_INVALID:
                w << "FABRIC_RESTART_PARTITION_MODE_INVALID";
                break;
            case FABRIC_RESTART_PARTITION_MODE_ALL_REPLICAS_OR_INSTANCES:
                w << "FABRIC_RESTART_PARTITION_MODE_ALL_REPLICAS_OR_INSTANCES";
                break;
            case FABRIC_RESTART_PARTITION_MODE_ONLY_ACTIVE_SECONDARIES:
                w << "FABRIC_RESTART_PARTITION_MODE_ONLY_ACTIVE_SECONDARIES";
                break;
            default:
                w << "Undefined FABRIC_RESTART_PARTITION_MODE = " << value_.valueInt64_;
        }
        break;
    case Type_FABRIC_NODE_TRANSITION_TYPE:
        switch (value_.valueInt64_)
        {
        case FABRIC_NODE_TRANSITION_TYPE_INVALID:
            w << "FABRIC_NODE_TRANSITION_TYPE_INVALID";
            break;
        case FABRIC_NODE_TRANSITION_TYPE_START:
            w << "FABRIC_NODE_TRANSITION_TYPE_START";
            break;
        case FABRIC_NODE_TRANSITION_TYPE_STOP:
            w << "FABRIC_NODE_TRANSITION_TYPE_STOP";
            break;
        default:
            w << "Undefined FABRIC_NODE_TRANSITION_TYPE = " << value_.valueInt64_;
        }
        break;
    case Type_FABRIC_CHAOS_EVENT_KIND:
        switch (value_.valueInt64_)
        {
        case FABRIC_CHAOS_EVENT_KIND_INVALID:
            w << "FABRIC_CHAOS_EVENT_KIND_INVALID";
            break;
        case FABRIC_CHAOS_EVENT_KIND_STARTED:
            w << "FABRIC_CHAOS_EVENT_KIND_STARTED";
            break;
        case FABRIC_CHAOS_EVENT_KIND_EXECUTING_FAULTS:
            w << "FABRIC_CHAOS_EVENT_KIND_EXECUTING_FAULTS";
            break;
        case FABRIC_CHAOS_EVENT_KIND_WAITING:
            w << "FABRIC_CHAOS_EVENT_KIND_WAITING";
            break;
        case FABRIC_CHAOS_EVENT_KIND_VALIDATION_FAILED:
            w << "FABRIC_CHAOS_EVENT_KIND_VALIDATION_FAILED";
            break;
        case FABRIC_CHAOS_EVENT_KIND_TEST_ERROR:
            w << "FABRIC_CHAOS_EVENT_KIND_TEST_ERROR";
            break;
        case FABRIC_CHAOS_EVENT_KIND_STOPPED:
            w << "FABRIC_CHAOS_EVENT_KIND_STOPPED";
            break;
        default:
            w << "Undefined FABRIC_CHAOS_EVENT_KIND = " << value_.valueInt64_;
        }
        break;
    case Type_FABRIC_CHAOS_STATUS:
        switch (value_.valueInt64_)
        {
        case FABRIC_CHAOS_STATUS_INVALID:
            w << "FABRIC_CHAOS_STATUS_INVALID";
            break;
        case FABRIC_CHAOS_STATUS_RUNNING:
            w << "FABRIC_CHAOS_STATUS_RUNNING";
            break;
        case FABRIC_CHAOS_STATUS_STOPPED:
            w << "FABRIC_CHAOS_STATUS_STOPPED";
            break;
        default:
            w << "Undefined FABRIC_CHAOS_STATUS = " << value_.valueInt64_;
        }
        break;
    case Type_FABRIC_RECONFIGURATION_PHASE:
        switch (value_.valueInt64_)
        {
        case FABRIC_RECONFIGURATION_PHASE_INVALID:
            w << "FABRIC_RECONFIGURATION_PHASE_INVALID";
            break;
        case FABRIC_RECONFIGURATION_PHASE_NONE:
            w << "FABRIC_RECONFIGURATION_PHASE_NONE";
            break;
        case FABRIC_RECONFIGURATION_PHASE_ZERO:
            w << "FABRIC_RECONFIGURATION_PHASE_ZERO";
            break;
        case FABRIC_RECONFIGURATION_PHASE_ONE:
            w << "FABRIC_RECONFIGURATION_PHASE_ONE";
            break;
        case FABRIC_RECONFIGURATION_PHASE_TWO:
            w << "FABRIC_RECONFIGURATION_PHASE_TWO";
            break;
        case FABRIC_RECONFIGURATION_PHASE_THREE:
            w << "FABRIC_RECONFIGURATION_PHASE_THREE";
            break;
        case FABRIC_RECONFIGURATION_PHASE_FOUR:
            w << "FABRIC_RECONFIGURATION_PHASE_FOUR";
            break;
        case FABRIC_RECONFIGURATION_ABORT_PHASE_ZERO:
            w << "FABRIC_RECONFIGURATION_ABORT_PHASE_ZERO";
            break;
        default:
            w << "Undefined FABRIC_RECONFIGURATION_PHASE = " << value_.valueInt64_;
        }
        break;
    case Type_FABRIC_RECONFIGURATION_TYPE:
        switch (value_.valueInt64_)
        {
        case FABRIC_RECONFIGURATION_TYPE_INVALID:
            w << "FABRIC_RECONFIGURATION_TYPE_INVALID";
            break;
        case FABRIC_RECONFIGURATION_TYPE_SWAPPRIMARY:
            w << "FABRIC_RECONFIGURATION_TYPE_SWAPPRIMARY";
            break;
        case FABRIC_RECONFIGURATION_TYPE_FAILOVER:
            w << "FABRIC_RECONFIGURATION_TYPE_FAILOVER";
            break;
        case FABRIC_RECONFIGURATION_TYPE_OTHER:
            w << "FABRIC_RECONFIGURATION_TYPE_OTHER";
            break;
        case FABRIC_RECONFIGURATION_TYPE_NONE:
            w << "FABRIC_RECONFIGURATION_TYPE_NONE";
            break;
        default:
            w << "Undefined FABRIC_RECONFIGURATION_TYPE = " << value_.valueInt64_;
        }
        break;

    case Type_FABRIC_PROVISION_APPLICATION_TYPE_KIND:
        switch (value_.valueInt64_)
        {
        case FABRIC_PROVISION_APPLICATION_TYPE_KIND_INVALID:
            w << "FABRIC_PROVISION_APPLICATION_TYPE_KIND_INVALID";
            break;
        case FABRIC_PROVISION_APPLICATION_TYPE_KIND_IMAGE_STORE_PATH:
            w << "FABRIC_PROVISION_APPLICATION_TYPE_KIND_IMAGE_STORE_PATH";
            break;
        case FABRIC_PROVISION_APPLICATION_TYPE_KIND_EXTERNAL_STORE:
            w << "FABRIC_PROVISION_APPLICATION_TYPE_KIND_EXTERNAL_STORE";
            break;
        default:
            w << "Undefined FABRIC_PROVISION_APPLICATION_TYPE_KIND = " << value_.valueInt64_;
        }
        break;

    case Type_FABRIC_DIAGNOSTICS_SINKS_KIND:
        switch (value_.valueInt64_)
        {
        case FABRIC_DIAGNOSTICS_SINKS_KIND_INVALID:
            w << "FABRIC_DIAGNOSTICS_SINKS_KIND_INVALID";
            break;
        case FABRIC_DIAGNOSTICS_SINKS_KIND_AZUREINTERNAL:
            w << "FABRIC_DIAGNOSTICS_SINKS_KIND_AZUREINTERNAL";
            break;
        default:
            w << "Undefined Type_FABRIC_DIAGNOSTICS_SINKS_KIND = " << value_.valueInt64_;
        }
        break;

    case Type_FABRIC_NETWORK_TYPE:
        switch (value_.valueInt64_)
        {
        case FABRIC_NETWORK_TYPE_INVALID:
            w << "FABRIC_NETWORK_TYPE_INVALID";
            break;
        case FABRIC_NETWORK_TYPE_LOCAL:
            w << "FABRIC_NETWORK_TYPE_LOCAL";
            break;
        case FABRIC_NETWORK_TYPE_FEDERATED:
            w << "FABRIC_NETWORK_TYPE_FEDERATED";
            break;
        default:
            w << "Undefined FABRIC_NETWORK_TYPE = " << value_.valueInt64_;
        }
        break;

    case Type_FABRIC_NETWORK_STATUS:
        switch (value_.valueInt64_)
        {
        case FABRIC_NETWORK_STATUS_INVALID:
            w << "FABRIC_NETWORK_STATUS_INVALID";
            break;
        case FABRIC_NETWORK_STATUS_READY:
            w << "FABRIC_NETWORK_STATUS_READY";
            break;
        case FABRIC_NETWORK_STATUS_CREATING:
            w << "FABRIC_NETWORK_STATUS_CREATING";
            break;
        case FABRIC_NETWORK_STATUS_DELETING:
            w << "FABRIC_NETWORK_STATUS_DELETING";
            break;
        case FABRIC_NETWORK_STATUS_UPDATING:
            w << "FABRIC_NETWORK_STATUS_UPDATING";
            break;
        case FABRIC_NETWORK_STATUS_FAILED:
            w << "FABRIC_NETWORK_STATUS_FAILED";
            break;
        default:
            w << "Undefined FABRIC_NETWORK_STATUS = " << value_.valueInt64_;
        }
        break;

    case Type_XmlNodeType:
        switch (value_.valueInt64_)
        {
        case ::XmlNodeType_None:
            w << L"XmlNodeType_None";
            break;
        case ::XmlNodeType_Element:
            w << L"XmlNodeType_Element";
            break;
        case ::XmlNodeType_Attribute:
            w << L"XmlNodeType_Attribute";
            break;
        case ::XmlNodeType_Text:
            w << L"XmlNodeType_Text";
            break;
        case ::XmlNodeType_CDATA:
            w << L"XmlNodeType_CDATA";
            break;
        case ::XmlNodeType_ProcessingInstruction:
            w << L"XmlNodeType_ProcessingInstruction";
            break;
        case ::XmlNodeType_Comment:
            w << L"XmlNodeType_Comment";
            break;
        case ::XmlNodeType_DocumentType:
            w << L"XmlNodeType_DocumentType";
            break;
        case ::XmlNodeType_Whitespace:
            w << L"XmlNodeType_Whitespace";
            break;
        case ::XmlNodeType_EndElement:
            w << L"XmlNodeType_EndElement";
            break;
        case ::XmlNodeType_XmlDeclaration:
            w << L"XmlNodeType_XmlDeclaration";
            break;
        default:
            w << "Unknown XmlNodeType value=" << value_.valueInt64_;
        }
        break;

    case TypeErrorCode:
        w << static_cast<ErrorCodeValue::Enum>(value_.valueInt64_);
        break;
    case TypeStdException:
        w << "(exception " << typeid(*value_.valueStdException_).name() << " " << value_.valueStdException_->what() << ')';
        break;
    case TypeStdErrorCategory:
        w << value_.valueStdErrorCategory_->name();
        break;
    case TypeStdErrorCode:
        w << value_.valueStdErrorCode_->category() << '(' << value_.valueStdErrorCode_->value() << "): " << value_.valueStdErrorCode_->message();
        break;
    case TypeStdSystemError:
        w << "(system_error " << value_.valueStdSystemError_->code() << " " << value_.valueStdSystemError_->what() << ')';
        break;
    case TypeTextWritablePointer:
        if (value_.valuePointer_ == nullptr)
        {
            w << "nullptr";
        }
        else
        {
            auto casted = reinterpret_cast<ITextWritable const *>(value_.valuePointer_);
            casted->WriteTo(w, format);
        }
        break;

    default:
        throw "Unknown type";
        break;
    }
}
