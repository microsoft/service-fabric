// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ResourceMonitor
    {
        class ResourceUsageReport;
    }
}

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReliabilityUnitTest
        {
            namespace StateManagement
            {
#pragma region Message Processing
                namespace MessageType
                {
                    enum Enum
                    {
                        // FM
                        AddPrimary,
                        AddPrimaryReply,

                        AddReplica,
                        AddReplicaReply,

                        AddInstance,
                        AddInstanceReply,

                        RemoveInstance,
                        RemoveInstanceReply,

                        DoReconfiguration,
                        DoReconfigurationReply,

                        ChangeConfiguration,
                        ChangeConfigurationReply,

                        GetLSN,
                        GetLSNReply,

                        Deactivate,
                        DeactivateReply,

                        Activate,
                        ActivateReply,

                        RemoveReplica,
                        RemoveReplicaReply,

                        DropReplica,
                        DropReplicaReply,
                        DeleteReplica,
                        DeleteReplicaReply,

                        CreateReplica,
                        CreateReplicaReply,

                        ReplicaUp,
                        ReplicaUpReply,

                        NodeUpgradeRequest,
                        NodeUpgradeReply,
                        CancelApplicationUpgradeRequest,
                        CancelApplicationUpgradeReply,

                        NodeFabricUpgradeRequest,
                        NodeFabricUpgradeReply,
                        CancelFabricUpgradeRequest,
                        CancelFabricUpgradeReply,

                        NodeActivateRequest,
                        NodeDeactivateRequest,
                        NodeActivateReply,
                        NodeDeactivateReply,

                        GenerationUpdate,
                        GenerationUpdateReject,
                        GenerationProposal,
                        GenerationProposalReply,

                        ReplicaEndpointUpdated,
                        ReplicaEndpointUpdatedReply,

                        ServiceTypeNotification,
                        ServiceTypeNotificationReply,

                        ServiceTypeEnabled,
                        ServiceTypeEnabledReply,
                        ServiceTypeDisabled,
                        ServiceTypeDisabledReply,

                        PartitionNotification,
                        PartitionNotificationReply,
                        
                        // RAP
                        ReplicaOpen,
                        ReplicaOpenReply,

                        ReplicaClose,
                        ReplicaCloseReply,

                        StatefulServiceReopen,
                        StatefulServiceReopenReply,

                        ReplicatorBuildIdleReplica,
                        ReplicatorBuildIdleReplicaReply,

                        UpdateConfiguration,
                        UpdateConfigurationReply,

                        ReplicatorRemoveIdleReplica,
                        ReplicatorRemoveIdleReplicaReply,

                        ReplicatorGetStatus,
                        ReplicatorGetStatusReply,

                        ReplicatorUpdateEpochAndGetStatus,
                        ReplicatorUpdateEpochAndGetStatusReply,

                        CancelCatchupReplicaSetReply,

                        ReportFault,

                        ProxyReplicaEndpointUpdated,
                        ProxyReplicaEndpointUpdatedReply,

                        ReadWriteStatusRevokedNotification,
                        ReadWriteStatusRevokedNotificationReply,

                        NodeUpdateServiceRequest,
                        NodeUpdateServiceReply,

                        NodeUpAck,

                        DeployedServiceReplicaQueryResult,
                        DeployedServiceReplicaQueryRequest,

                        DeployedServiceReplicaDetailQueryRequest,
                        DeployedServiceReplicaDetailQueryReply,

                        ProxyDeployedServiceReplicaDetailQueryRequest,
                        ProxyDeployedServiceReplicaDetailQueryReply,

                        ClientReportFaultRequest,
                        ClientReportFaultReply,

                        ProxyUpdateServiceDescription,
                        ProxyUpdateServiceDescriptionReply,

                        ReportLoad,
                        ResourceUsageReport,
                    };
                }

                template<MessageType::Enum T>
                struct MessageTypeTraits
                {
                };

#define DECLARE_MESSAGE_TYPE_TRAITS(enum_name, bodytype) template<> struct MessageTypeTraits<enum_name> { static Common::WStringLiteral const Action; typedef bodytype BodyType; };  
#define DEFINE_MESSAGE_TYPE_TRAITS(enum_name, str) Common::WStringLiteral const Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement::MessageTypeTraits<enum_name>::Action(str);

                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::AddPrimary, Reliability::ReplicaMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::AddPrimaryReply, Reliability::ReplicaReplyMessageBody);

                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::AddReplica, Reliability::ReplicaMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::AddReplicaReply, Reliability::ReplicaReplyMessageBody);

                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::AddInstance, Reliability::ReplicaMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::AddInstanceReply, Reliability::ReplicaReplyMessageBody);

                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::RemoveInstance, Reliability::DeleteReplicaMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::RemoveInstanceReply, Reliability::ReplicaReplyMessageBody);

                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::DoReconfiguration, Reliability::DoReconfigurationMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::DoReconfigurationReply, Reliability::ConfigurationReplyMessageBody);

                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ChangeConfiguration, Reliability::ConfigurationMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ChangeConfigurationReply, Reliability::FailoverUnitReplyMessageBody);

                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::GetLSN, Reliability::ReplicaMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::GetLSNReply, Reliability::GetLSNReplyMessageBody);

                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::Deactivate, Reliability::DeactivateMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::DeactivateReply, Reliability::ReplicaReplyMessageBody);

                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::Activate, Reliability::ActivateMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ActivateReply, Reliability::ReplicaReplyMessageBody);

                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::RemoveReplica, Reliability::ReplicaMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::RemoveReplicaReply, Reliability::ReplicaReplyMessageBody);

                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::DropReplica, Reliability::ReplicaMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::DropReplicaReply, Reliability::ReplicaReplyMessageBody);

                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::DeleteReplica, Reliability::DeleteReplicaMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::DeleteReplicaReply, Reliability::ReplicaReplyMessageBody);

                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::CreateReplica, Reliability::RAReplicaMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::CreateReplicaReply, Reliability::ReplicaReplyMessageBody);

                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ReplicaUp, Reliability::ReplicaUpMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ReplicaUpReply, Reliability::ReplicaUpMessageBody);

                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ReplicaEndpointUpdated, Reliability::ReplicaMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ReplicaEndpointUpdatedReply, Reliability::ReplicaReplyMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::NodeUpgradeRequest, Reliability::UpgradeDescription);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::NodeUpgradeReply, Reliability::NodeUpgradeReplyMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::CancelApplicationUpgradeRequest, Reliability::UpgradeDescription);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::CancelApplicationUpgradeReply, Reliability::NodeUpgradeReplyMessageBody);

                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::NodeFabricUpgradeRequest, ServiceModel::FabricUpgradeSpecification);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::NodeFabricUpgradeReply, Reliability::NodeFabricUpgradeReplyMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::CancelFabricUpgradeRequest, ServiceModel::FabricUpgradeSpecification);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::CancelFabricUpgradeReply, Reliability::NodeFabricUpgradeReplyMessageBody);

                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::NodeActivateRequest, Reliability::NodeActivateRequestMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::NodeDeactivateRequest, Reliability::NodeDeactivateRequestMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::NodeActivateReply, Reliability::NodeActivateReplyMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::NodeDeactivateReply, Reliability::NodeDeactivateReplyMessageBody);

                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::GenerationUpdate, Reliability::GenerationUpdateMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::GenerationUpdateReject, Reliability::GenerationNumber);

                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::GenerationProposal, Reliability::GenerationNumber); // need to have a type here because template specialization on void is not allowed
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::GenerationProposalReply, Reliability::GenerationProposalReplyMessageBody);

                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ServiceTypeNotification, Reliability::ServiceTypeNotificationRequestMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ServiceTypeNotificationReply, Reliability::ServiceTypeNotificationReplyMessageBody);

                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ServiceTypeEnabled, Reliability::ServiceTypeUpdateMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ServiceTypeEnabledReply, Reliability::ServiceTypeUpdateReplyMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ServiceTypeDisabled, Reliability::ServiceTypeUpdateMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ServiceTypeDisabledReply, Reliability::ServiceTypeUpdateReplyMessageBody);

                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::PartitionNotification, Reliability::PartitionNotificationMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::PartitionNotificationReply, Reliability::ServiceTypeUpdateReplyMessageBody);

                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ReplicaOpen, Reliability::ReconfigurationAgentComponent::ProxyRequestMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ReplicaClose, Reliability::ReconfigurationAgentComponent::ProxyRequestMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ReplicatorBuildIdleReplica, Reliability::ReconfigurationAgentComponent::ProxyRequestMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::UpdateConfiguration, Reliability::ReconfigurationAgentComponent::ProxyRequestMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ReplicatorRemoveIdleReplica, Reliability::ReconfigurationAgentComponent::ProxyRequestMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ReplicatorGetStatus, Reliability::ReconfigurationAgentComponent::ProxyRequestMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ReplicatorUpdateEpochAndGetStatus, Reliability::ReconfigurationAgentComponent::ProxyRequestMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::StatefulServiceReopen, Reliability::ReconfigurationAgentComponent::ProxyRequestMessageBody);

                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ReplicaOpenReply, Reliability::ReconfigurationAgentComponent::ProxyReplyMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ReplicaCloseReply, Reliability::ReconfigurationAgentComponent::ProxyReplyMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::StatefulServiceReopenReply, Reliability::ReconfigurationAgentComponent::ProxyReplyMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ReplicatorBuildIdleReplicaReply, Reliability::ReconfigurationAgentComponent::ProxyReplyMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::UpdateConfigurationReply, Reliability::ReconfigurationAgentComponent::ProxyReplyMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ReplicatorRemoveIdleReplicaReply, Reliability::ReconfigurationAgentComponent::ProxyReplyMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ReplicatorGetStatusReply, Reliability::ReconfigurationAgentComponent::ProxyReplyMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ReplicatorUpdateEpochAndGetStatusReply, Reliability::ReconfigurationAgentComponent::ProxyReplyMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::CancelCatchupReplicaSetReply, Reliability::ReconfigurationAgentComponent::ProxyReplyMessageBody);

                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ProxyUpdateServiceDescription, Reliability::ReconfigurationAgentComponent::ProxyRequestMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ProxyUpdateServiceDescriptionReply, Reliability::ReconfigurationAgentComponent::ProxyUpdateServiceDescriptionReplyMessageBody);


                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ReportFault, Reliability::ReconfigurationAgentComponent::ReportFaultMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::NodeUpdateServiceRequest, Reliability::NodeUpdateServiceRequestMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::NodeUpdateServiceReply, Reliability::NodeUpdateServiceReplyMessageBody);

                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::NodeUpAck, Reliability::NodeUpAckMessageBody);

                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ProxyReplicaEndpointUpdated, Reliability::ReplicaMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ProxyReplicaEndpointUpdatedReply, Reliability::ReconfigurationAgentComponent::ProxyRequestMessageBody);

                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ReadWriteStatusRevokedNotification, Reliability::ReplicaMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ReadWriteStatusRevokedNotificationReply, Reliability::ReconfigurationAgentComponent::ProxyRequestMessageBody);

                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::DeployedServiceReplicaQueryRequest, ::Query::QueryRequestMessageBodyInternal);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::DeployedServiceReplicaQueryResult, ServiceModel::QueryResult);

                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::DeployedServiceReplicaDetailQueryRequest, ::Query::QueryRequestMessageBodyInternal);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::DeployedServiceReplicaDetailQueryReply, ServiceModel::QueryResult);

                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ProxyDeployedServiceReplicaDetailQueryRequest, Reliability::ReconfigurationAgentComponent::ProxyRequestMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ProxyDeployedServiceReplicaDetailQueryReply, Reliability::ReconfigurationAgentComponent::ProxyQueryReplyMessageBody<ServiceModel::DeployedServiceReplicaDetailQueryResult>);

                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ClientReportFaultRequest, Reliability::ReportFaultRequestMessageBody);
                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ClientReportFaultReply, Reliability::ReportFaultReplyMessageBody);

                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ReportLoad, Reliability::ReportLoadMessageBody);

                DECLARE_MESSAGE_TYPE_TRAITS(MessageType::ResourceUsageReport, Management::ResourceMonitor::ResourceUsageReport);
#pragma endregion


                // This class is used to encapsulate default values that are passed down to the individual template functions
                // Callers should derive from this class and in the implementation of their class pass down the correct object
                class ReadContext
                {
                public:
                    template<typename T>
                    T & GetDerived()
                    {
                        T* derivedPtr = TryGetDerived<T>();
                        ASSERT_IF(derivedPtr == nullptr, "Invalid Read Context");
                        return *derivedPtr;
                    }

                    template<typename T>
                    T const & GetDerived() const
                    {
                        T const * derivedPtr = TryGetDerived<T>();
                        ASSERT_IF(derivedPtr == nullptr, "Invalid Read Context");
                        return *derivedPtr;
                    }

                    template<typename T>
                    T const * TryGetDerived() const
                    {
                        return dynamic_cast<T const *>(this);                        
                    }

                    template<typename T>
                    T * TryGetDerived() 
                    {
                        return dynamic_cast<T*>(this);
                    }

                    virtual ~ReadContext() {}
                };

                // Logs an error while parsing a specific type 
                class ErrorLogger
                {
                public:
                    ErrorLogger(std::wstring const & type, std::wstring const & value)
                        : type_(type),
                        value_(value)
                    {
                    }

                    void Log(std::wstring const & field)
                    {
                        TestLog::WriteError(Common::wformatString("PARSE ERROR ({0}): Failed to read {1} for {2}", type_, field, value_));
                    }

                private:
                    std::wstring type_;
                    std::wstring value_;
                };

                // Specialize this template with your type to support parsing complex types
                // All specializations must live in StateReaderImpl.Test.h
                template<typename T>
                struct ReaderImpl
                {
                    static bool Read(std::wstring const &, ReadOption::Enum, ReadContext const &, T &)
                    {
                        static_assert(false, "Not specialized");
                    }
                };

                // Implementation of the reader code itself
                // This is a simple class for reading strings
                // It parses complex types by looking up the specific implementation of the type
                // To implement complex type parsing look at StateReaderImpl.Test.h
                class Reader
                {
                    DENY_COPY(Reader);
                public:

#pragma region Member Functions

                    Reader(std::wstring const & input, ReadContext const & context, wchar_t delim = L' ')
                        : input_(input),
                        index_(0),
                        delim_(delim),
                        context_(&context)
                    {
                    }

                    __declspec(property(get = get_IsEOF)) bool IsEOF;
                    bool get_IsEOF() const { return index_ >= input_.length(); }

                    wchar_t ReadChar()
                    {
                        AssertInvariants();
                        return input_[index_++];
                    }

                    // Peek characters to identifiy which of the input array delimiters are the next
                    bool PeekNextDelimiter(std::initializer_list<wchar_t> const & delimiters, __out wchar_t & delimiter)
                    {
                        delimiter = L'\0';

                        auto index = index_;

                        while (!IsEOF)
                        {
                            for (auto ch : delimiters)
                            {
                                if (ch == input_[index])
                                {
                                    delimiter = ch;
                                    return true;
                                }
                            }

                            ++index;
                        }

                        return false;
                    }

                    bool HasCharacter(wchar_t ch) const
                    {
                        if (IsEOF)
                        {
                            return false;
                        }

                        for (size_t i = index_; i < input_.length(); i++)
                        {
                            if (input_[i] == ch)
                            {
                                return true;
                            }
                            else if (input_[i] == delim_)
                            {
                                return false;
                            }
                        }

                        return false;
                    }

                    void SkipDelimiters()
                    {
                        for (;;)
                        {
                            if (IsEOF)
                            {
                                break;
                            }

                            if (PeekChar() == delim_)
                            {
                                ReadChar();
                            }
                            else
                            {
                                break;
                            }
                        }
                    }

                    // Read the string till the terminator (consuming the terminator)
                    // Put us at the next character
                    std::wstring ReadString(wchar_t terminator)
                    {
                        std::wstring output;
                        for (;;)
                        {
                            if (IsEOF)
                            {
                                break;
                            }

                            wchar_t current = ReadChar();
                            if (current == terminator)
                            {
                                break;
                            }

                            output += current;
                        }

                        SkipDelimiters();
                        return output;
                    }

                    wchar_t PeekChar() const
                    {
                        AssertInvariants();
                        return input_[index_];
                    }

                    // Create a reader and throw if unable to read
                    template<typename T>
                    static T ReadHelper(std::wstring const & str, ReadContext const & context)
                    {
                        Reader r(str, context);

                        T obj;
                        bool rv = r.Read(L'\0', ReadOption::None, obj);
                        ASSERT_IF(!rv, "Unable to parse {0}", str);

                        return obj;
                    }

                    // Create a reader and throw if unable to read
                    template<typename T>
                    static T ReadHelper(std::wstring const & str)
                    {
                        ReadContext context;
                        Reader r(str, context);

                        T obj;
                        bool rv = r.Read(L'\0', ReadOption::None, obj);
                        ASSERT_IF(!rv, "Unable to parse {0}", str);

                        return obj;
                    }

                    template<typename T>
                    static std::vector<T> ReadVector(wchar_t start, wchar_t end, std::wstring const & str)
                    {
                        ReadContext context;
                        Reader r(str, context);

                        std::vector<T> v;
                        bool rv = r.ReadVector(start, end, ReadOption::None, v);
                        ASSERT_IF(!rv, "Unable to parse {0}", str);

                        return v;
                    }

                    // Read an object from the current position until terminator
                    template<typename T>
                    bool Read(wchar_t terminator, T & rv)
                    {
                        return Read<T>(terminator, ReadOption::None, rv);
                    }

                    template<typename T>
                    bool Read(wchar_t terminator, ReadOption::Enum option, T & rv)
                    {
                        return Read<T>(false, terminator, option, rv);
                    }

                    template<typename T>
                    bool ReadOptional(wchar_t terminator, T & rv)
                    {
                        return ReadOptional<T>(terminator, ReadOption::None, rv);
                    }

                    template<typename T>
                    bool ReadOptional(wchar_t terminator, ReadOption::Enum option, T & rv)
                    {
                        return Read<T>(true, terminator, option, rv);
                    }

                    template<typename T>
                    bool Read(bool isOptional, wchar_t terminator, ReadOption::Enum option, T & rv)
                    {
                        if (IsEOF && !isOptional)
                        {
                            return false;
                        }

                        std::wstring token = ReadString(terminator);
                        return ReaderImpl<T>::Read(token, option, *context_, rv);
                    }

                    // Read an object delimited by a start and an end terminator
                    template<typename T>
                    bool Read(wchar_t startDelimiter, wchar_t endDelimiter, T & rv)
                    {
                        return Read<T>(startDelimiter, endDelimiter, ReadOption::None, rv);
                    }

                    template<typename T>
                    bool Read(wchar_t startDelimiter, wchar_t endDelimiter, ReadOption::Enum option, T & rv)
                    {
                        for (;;)
                        {
                            AssertInvariants();

                            ASSERT_IF(ReadChar() != startDelimiter, "unknown character found");

                            return Read<T>(endDelimiter, option, rv);
                        }
                    }

                    // read an array of items. each item is delimited by start and end delimiters
                    template<typename T>
                    bool ReadVector(wchar_t startDelimiter, wchar_t endDelimiter, std::vector<T> & rv)
                    {
                        return ReadVector<T>(startDelimiter, endDelimiter, ReadOption::None, rv);
                    }

                    template<typename T>
                    bool ReadVector(wchar_t startDelimiter, wchar_t endDelimiter, ReadOption::Enum option, std::vector<T> & rv)
                    {
                        for (;;)
                        {
                            if (!HasCharacter(startDelimiter) || IsEOF)
                            {
                                break;
                            }

                            T val;
                            if (!Read<T>(startDelimiter, endDelimiter, option, val))
                            {
                                return false;
                            }

                            rv.push_back(std::move(val));
                        }

                        return true;
                    }

                    // read a map of items. each entry is delimited by start and end delimiters. format of a single entry is [key value]
                    template<typename K, typename T>
                    bool ReadMap(wchar_t startDelimiter, wchar_t endDelimiter, std::map<K, T> & rv)
                    {
                        return ReadMap<K, T>(startDelimiter, endDelimiter, ReadOption::None, rv);
                    }

                    template<typename K, typename T>
                    bool ReadMap(wchar_t startDelimiter, wchar_t endDelimiter, ReadOption::Enum option, std::map<K, T> & rv)
                    {
                        for (;;)
                        {
                            if (!HasCharacter(startDelimiter) || IsEOF)
                            {
                                break;
                            }

                            K key;
                            if (!Read<K>(startDelimiter, ' ', option, key))
                            {
                                return false;
                            }

                            T val;
                            if (!Read<T>(endDelimiter, option, val))
                            {
                                return false;
                            }

                            rv[key] = val;
                        }

                        return true;
                    }

                    void SetReadContext(ReadContext const & context)
                    {
                        context_ = &context;
                    }

#pragma endregion          

                private:

                    void AssertInvariants() const
                    {
                        ASSERT_IF(IsEOF, "EOF");
                    }

                    wchar_t const delim_;
                    ReadContext const * context_;

                    std::wstring input_;
                    size_t index_;
                };
            }
        }

    }
}
