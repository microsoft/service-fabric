// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

/*

This file contains specializations of all the template functions

*/

#include "ServiceModel/management/ResourceMonitor/public.h"

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReliabilityUnitTest
        {
            namespace StateManagement
            {
# pragma region Utility

                inline Federation::NodeInstance CreateNodeInstanceEx(int64 nodeId, int64 nodeInstanceId)
                {
                    return Federation::NodeInstance(Federation::NodeId(Common::LargeInteger(0, nodeId)), nodeInstanceId);
                }

# pragma endregion

#pragma region Primitives

                struct ReplicaAndNodeId
                {
                    int64 NodeId;
                    int64 NodeInstanceId;
                    int64 ReplicaId;
                    int64 ReplicaInstanceId;

                    ReplicaAndNodeId()
                    {
                        NodeId = -1;
                        NodeInstanceId = -1;
                        ReplicaId = -1;
                        ReplicaInstanceId = -1;
                    }

                    Federation::NodeInstance CreateNodeInstance() const
                    {
                        return CreateNodeInstanceEx(NodeId, NodeInstanceId);
                    }
                };

                template<>
                struct ReaderImpl < ReplicaAndNodeId >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, ReplicaAndNodeId & rv)
                    {
                        ErrorLogger errLogger(L"ReplicaAndNodeId", value);
                        Reader reader(value, context);

                        std::vector<int64> values;

                        // ReplicaId:ReplicaInstanceId (NodeId = Replica Id and NodeInstance = 1)
                        // NodeId:ReplicaId:ReplicaInstanceId (Node Instance = 1)*/
                        // read 4 int 64's 
                        for (int i = 0; i < 4; i++)
                        {
                            int64 val = 0;
                            if (!reader.Read<int64>(L':', val))
                            {
                                break;
                            }

                            values.push_back(val);
                        }

                        if (values.size() < 1 || values.size() > 4)
                        {
                            errLogger.Log(L"Failed to read replica/node id");
                            return false;
                        }

                        if (values.size() == 1)
                        {
                            int64 replicaId = values[0];

                            rv.NodeId = replicaId;
                            rv.NodeInstanceId = 1;
                            rv.ReplicaId = replicaId;
                            rv.ReplicaInstanceId = 1;
                        }
                        else if (values.size() == 2)
                        {
                            int64 replicaId = values[0];
                            int64 replicaInstanceId = values[1];

                            rv.NodeId = replicaId;
                            rv.NodeInstanceId = 1;
                            rv.ReplicaId = replicaId;
                            rv.ReplicaInstanceId = replicaInstanceId;
                        }
                        else if (values.size() == 3)
                        {
                            int64 nodeId = values[0];
                            int64 replicaId = values[1];
                            int64 replicaInstanceId = values[2];

                            rv.NodeId = nodeId;
                            rv.NodeInstanceId = 1;
                            rv.ReplicaId = replicaId;
                            rv.ReplicaInstanceId = replicaInstanceId;
                        }
                        else if (values.size() == 4)
                        {
                            rv.NodeId = values[0];
                            rv.NodeInstanceId = values[1];
                            rv.ReplicaId = values[2];
                            rv.ReplicaInstanceId = values[3];
                        }

                        return true;
                    }
                };

                template<>
                struct ReaderImpl < double >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const &, double & rv)
                    {
                        double val;
                        if (!Common::TryParseDouble(value, val))
                        {
                            return false;
                        }

                        rv = static_cast<double>(val);
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < int >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const &, int & rv)
                    {
                        int64 val;
                        if (!Common::TryParseInt64(value, val))
                        {
                            return false;
                        }

                        rv = static_cast<int>(val);
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < int64 >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const &, int64 & rv)
                    {
                        if (!Common::TryParseInt64(value, rv))
                        {
                            Verify::Fail(Common::wformatString("Failed to parse int64 from {0}", value));
                            return false;
                        }

                        return true;
                    }
                };

                template<>
                struct ReaderImpl < ULONG >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum e, ReadContext const & c, ULONG & rv)
                    {
                        int64 temp = 0;
                        bool parsed = ReaderImpl<int64>::Read(value, e, c, temp);
                        if (!parsed)
                        {
                            return parsed;
                        }

                        rv = static_cast<ULONG>(temp);
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < uint64 >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum e, ReadContext const & c, uint64 & rv)
                    {
                        int64 temp = 0;
                        bool parsed = ReaderImpl<int64>::Read(value, e, c, temp);
                        if (!parsed)
                        {
                            return parsed;
                        }

                        rv = static_cast<uint64>(temp);
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < bool >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const &, bool & rv)
                    {
                        if (value == L"true" || value == L"True")
                        {
                            rv = true;
                        }
                        else if (value == L"false" || value == L"False")
                        {
                            rv = false;
                        }

                        return true;
                    }
                };

#pragma endregion

#pragma region Common
                template<>
                struct ReaderImpl < Common::ErrorCode >
                {
                    class ReverseLookupMap
                    {
                    public:
                        ReverseLookupMap()
                        {
                            for (int i = FABRIC_E_FIRST_RESERVED_HRESULT; i < FABRIC_E_LAST_USED_HRESULT + 1; i++)
                            {
                                std::wstring val = Common::wformatString("{0}", (Common::ErrorCodeValue::Enum)i);
                                lookupTable_[val] = (Common::ErrorCodeValue::Enum)i;
                            }

                            for (int i = Common::ErrorCodeValue::FIRST_INTERNAL_ERROR_CODE_MINUS_ONE + 1; i < Common::ErrorCodeValue::LAST_INTERNAL_ERROR_CODE; i++)
                            {
                                std::wstring val = Common::wformatString("{0}", (Common::ErrorCodeValue::Enum)i);
                                lookupTable_[val] = (Common::ErrorCodeValue::Enum)i;
                            }

                            lookupTable_[L"Success"] = Common::ErrorCodeValue::Success;
                            lookupTable_[L"S_OK"] = Common::ErrorCodeValue::Success;
                            lookupTable_[L"InvalidArgument"] = Common::ErrorCodeValue::InvalidArgument;
                            lookupTable_[L"E_INVALIDARG"] = Common::ErrorCodeValue::InvalidArgument;
                        }

                        Common::ErrorCode Lookup(std::wstring const & value) const
                        {
                            auto it = lookupTable_.find(value);
                            Verify::IsTrueLogOnError(it != lookupTable_.cend(), Common::wformatString("Unable to find error code {0}", value));
                            return it->second;
                        }

                    private:
                        std::map<std::wstring, Common::ErrorCodeValue::Enum> lookupTable_;
                    };

                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const &, Common::ErrorCode & rv)
                    {
                        static std::unique_ptr<ReverseLookupMap> lookupMap;
                        if (lookupMap == nullptr)
                        {
                            lookupMap = Common::make_unique<ReverseLookupMap>();
                        }

                        rv = lookupMap->Lookup(value);
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < Common::Guid >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const &, Common::Guid & rv)
                    {
                        rv = Common::Guid(value);
                        return true;
                    }
                };

#pragma endregion

                // Epochs are three characters in length
                template<>
                struct ReaderImpl < Reliability::Epoch >
                {
                    static bool Read(std::wstring const & epoch, ReadOption::Enum, ReadContext const &, Reliability::Epoch & rv)
                    {
                        if (epoch.size() < 3)
                        {
                            Verify::Fail(Common::wformatString("ReaderImpl<Reliability::Epoch> - Reliability::Epoch is less than three characters {0}", epoch));
                            return false;
                        }

                        int64 v0 = epoch[2] - L'0';
                        int64 v1 = epoch[1] - L'0';
                        int64 v2 = epoch[0] - L'0';

                        rv = Reliability::Epoch(v2, (v1 << 32) + v0);
                        return true;
                    }
                };

#pragma region Enumerations

                template<typename T>
                class EnumReader
                {
                    DENY_COPY(EnumReader);

                public:
                    EnumReader(std::wstring const & fieldName)
                        : fieldName_(fieldName)
                    {
                    }

                    void Add(T val, std::wstring const & str)
                    {
                        values_[str] = val;
                    }

                    void Add(T val, std::wstring const & str1, std::wstring const & str2)
                    {
                        Add(val, str1);
                        Add(val, str2);
                    }

                    void Add(T val, std::wstring const & str1, std::wstring const & str2, std::wstring const & str3)
                    {
                        Add(val, str1, str2);
                        Add(val, str3);
                    }

                    bool Read(std::wstring const & str, T & rv) const
                    {
                        for (auto it = values_.cbegin(); it != values_.cend(); ++it)
                        {
                            if (Common::StringUtility::AreEqualCaseInsensitive(it->first, str))
                            {
                                rv = it->second;
                                return true;
                            }
                        }

                        PrintError(str);
                        return false;
                    }

                    void PrintError(std::wstring const & input) const
                    {
                        std::wstring rv = Common::wformatString("{0} Invalid Value {1}. Allowed = ", fieldName_, input);
                        for (auto it = values_.cbegin(); it != values_.cend(); ++it)
                        {
                            rv += it->first;
                            rv += L", ";
                        }

                        Verify::Fail(rv);
                    }


                private:
                    std::wstring const fieldName_;
                    std::map<std::wstring, T> values_;
                };

                template<>
                struct ReaderImpl < Common::SystemHealthReportCode::Enum >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const &, Common::SystemHealthReportCode::Enum & rv)
                    {
                        EnumReader<Common::SystemHealthReportCode::Enum> enumReader(L"SystemHealthReportCode");

                        enumReader.Add(Common::SystemHealthReportCode::RAP_ApiOk, L"rap_apiok");
                        enumReader.Add(Common::SystemHealthReportCode::RAP_ApiSlow, L"rap_apislow");
                        enumReader.Add(Common::SystemHealthReportCode::RA_DeleteReplica, L"ra_replicadeleted");
                        enumReader.Add(Common::SystemHealthReportCode::RA_ReplicaCreated, L"ra_replicacreated");
                        enumReader.Add(Common::SystemHealthReportCode::RA_ReplicaOpenStatusWarning, L"RA_ReplicaOpenStatusWarning");
                        enumReader.Add(Common::SystemHealthReportCode::RA_ReplicaOpenStatusHealthy, L"RA_ReplicaOpenStatusHealthy");
                        enumReader.Add(Common::SystemHealthReportCode::RA_ReplicaChangeRoleStatusError, L"RA_ReplicaChangeRoleStatusError");
                        enumReader.Add(Common::SystemHealthReportCode::RA_ReplicaChangeRoleStatusHealthy, L"RA_ReplicaChangeRoleStatusHealthy");
                        enumReader.Add(Common::SystemHealthReportCode::RA_ReplicaCloseStatusWarning, L"RA_ReplicaCloseStatusWarning");
                        enumReader.Add(Common::SystemHealthReportCode::RA_ReplicaCloseStatusHealthy, L"RA_ReplicaCloseStatusHealthy");
                        enumReader.Add(Common::SystemHealthReportCode::RA_ReplicaServiceTypeRegistrationStatusWarning, L"RA_ReplicaServiceTypeRegistrationStatusWarning");
                        enumReader.Add(Common::SystemHealthReportCode::RA_ReplicaServiceTypeRegistrationStatusHealthy, L"RA_ReplicaServiceTypeRegistrationStatusHealthy");

                        return enumReader.Read(value, rv);
                    }
                };

                template<>
                struct ReaderImpl < Storage::Api::OperationType::Enum >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const &, Storage::Api::OperationType::Enum & rv)
                    {
                        EnumReader<Storage::Api::OperationType::Enum> enumReader(L"RAStoreOperation");

                        enumReader.Add(Storage::Api::OperationType::Insert, L"i");
                        enumReader.Add(Storage::Api::OperationType::Delete, L"d");
                        enumReader.Add(Storage::Api::OperationType::Update, L"u");

                        return enumReader.Read(value, rv);
                    }
                };

                template<>
                struct ReaderImpl < FailoverUnitStates::Enum >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const &, FailoverUnitStates::Enum & rv)
                    {
                        EnumReader<FailoverUnitStates::Enum> enumReader(L"FT State");
                        enumReader.Add(FailoverUnitStates::Open, L"o", L"open");
                        enumReader.Add(FailoverUnitStates::Closed, L"c", L"closed");

                        return enumReader.Read(value, rv);
                    }
                };

                template<>
                struct ReaderImpl < ::FABRIC_REPLICA_ROLE >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const &, ::FABRIC_REPLICA_ROLE & rv)
                    {
                        EnumReader<::FABRIC_REPLICA_ROLE> enumReader(L"REPLICA_ROLE");
                        enumReader.Add(FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, L"ActiveSecondary", L"FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY");
                        enumReader.Add(FABRIC_REPLICA_ROLE_IDLE_SECONDARY, L"Idle", L"IdleSecondary", L"FABRIC_REPLICA_ROLE_IDLE_SECONDARY");
                        enumReader.Add(FABRIC_REPLICA_ROLE_NONE, L"None", L"FABRIC_REPLICA_ROLE_NONE");
                        enumReader.Add(FABRIC_REPLICA_ROLE_PRIMARY, L"Primary", L"FABRIC_REPLICA_ROLE_PRIMARY");
                        enumReader.Add(FABRIC_REPLICA_ROLE_UNKNOWN, L"Unknown", L"FABRIC_REPLICA_ROLE_UNKNOWN");
                        return enumReader.Read(value, rv);
                    }
                };

                template<>
                struct ReaderImpl < Reliability::FaultType::Enum >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const &, Reliability::FaultType::Enum & rv)
                    {
                        EnumReader<Reliability::FaultType::Enum> enumReader(L"FAULT_TYPE");
                        enumReader.Add(Reliability::FaultType::Permanent, L"Permanent", L"FABRIC_FAULT_TYPE_PERMANENT");
                        enumReader.Add(Reliability::FaultType::Transient, L"Transient", L"FABRIC_FAULT_TYPE_TRANSIENT");
                        return enumReader.Read(value, rv);
                    }
                };

                template<>
                struct ReaderImpl < Reliability::ReplicaStates::Enum >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const &, Reliability::ReplicaStates::Enum & rv)
                    {
                        EnumReader<Reliability::ReplicaStates::Enum> enumReader(L"ReplicaStates");
                        enumReader.Add(Reliability::ReplicaStates::StandBy, L"SB", L"StandBy");
                        enumReader.Add(Reliability::ReplicaStates::InBuild, L"IB", L"InBuild");
                        enumReader.Add(Reliability::ReplicaStates::Ready, L"RD", L"Ready");
                        enumReader.Add(Reliability::ReplicaStates::Dropped, L"DD", L"Dropped");
                        return enumReader.Read(value, rv);
                    }
                };

                template<>
                struct ReaderImpl < Reliability::ReplicaRole::Enum >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const &, Reliability::ReplicaRole::Enum & rv)
                    {
                        EnumReader<Reliability::ReplicaRole::Enum> enumReader(L"Reliability::ReplicaRole");
                        enumReader.Add(Reliability::ReplicaRole::None, L"N");
                        enumReader.Add(Reliability::ReplicaRole::Idle, L"I");
                        enumReader.Add(Reliability::ReplicaRole::Secondary, L"S");
                        enumReader.Add(Reliability::ReplicaRole::Primary, L"P");
                        enumReader.Add(Reliability::ReplicaRole::Unknown, L"U");
                        return enumReader.Read(value, rv);
                    }
                };

                template<>
                struct ReaderImpl < ReplicaStates::Enum >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const &, ReplicaStates::Enum & rv)
                    {
                        EnumReader<ReplicaStates::Enum> enumReader(L"ReplicaStates");
                        enumReader.Add(ReplicaStates::InCreate, L"IC");
                        enumReader.Add(ReplicaStates::InBuild, L"IB");
                        enumReader.Add(ReplicaStates::Ready, L"RD");
                        enumReader.Add(ReplicaStates::InDrop, L"ID");
                        enumReader.Add(ReplicaStates::Dropped, L"DD");
                        enumReader.Add(ReplicaStates::StandBy, L"SB");
                        return enumReader.Read(value, rv);
                    }
                };

                template<>
                struct ReaderImpl < ReplicaMessageStage::Enum >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const &, ReplicaMessageStage::Enum & rv)
                    {
                        EnumReader<ReplicaMessageStage::Enum> enumReader(L"ReplicaMessageStage");
                        enumReader.Add(ReplicaMessageStage::None, L"N");
                        enumReader.Add(ReplicaMessageStage::RAReplyPending, L"RA");
                        enumReader.Add(ReplicaMessageStage::RAProxyReplyPending, L"RAP");
                        return enumReader.Read(value, rv);
                    }
                };

                template<>
                struct ReaderImpl < FailoverUnitReconfigurationStage::Enum >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const &, FailoverUnitReconfigurationStage::Enum & rv)
                    {
                        EnumReader<FailoverUnitReconfigurationStage::Enum> enumReader(L"FailoverUnitReconfigurationStage");
                        enumReader.Add(FailoverUnitReconfigurationStage::Phase0_Demote, L"Phase0_Demote");
                        enumReader.Add(FailoverUnitReconfigurationStage::Phase1_GetLSN, L"Phase1_GetLSN");
                        enumReader.Add(FailoverUnitReconfigurationStage::Phase2_Catchup, L"Phase2_Catchup");
                        enumReader.Add(FailoverUnitReconfigurationStage::Phase3_Deactivate, L"Phase3_Deactivate");
                        enumReader.Add(FailoverUnitReconfigurationStage::Phase4_Activate, L"Phase4_Activate");
                        enumReader.Add(FailoverUnitReconfigurationStage::None, L"None");
                        enumReader.Add(FailoverUnitReconfigurationStage::Abort_Phase0_Demote, L"Abort_Phase0_Demote");
                        return enumReader.Read(value, rv);
                    }
                };

                template<>
                struct ReaderImpl < ServiceModel::UpgradeType::Enum >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const &, ServiceModel::UpgradeType::Enum & rv)
                    {
                        EnumReader<ServiceModel::UpgradeType::Enum> enumReader(L"UpgradeType");
                        enumReader.Add(ServiceModel::UpgradeType::Rolling, L"Rolling");
                        enumReader.Add(ServiceModel::UpgradeType::Rolling_ForceRestart, L"Rolling_ForceRestart");
                        enumReader.Add(ServiceModel::UpgradeType::Rolling_NotificationOnly, L"Rolling_NotificationOnly");
                        return enumReader.Read(value, rv);
                    }
                };

                template<>
                struct ReaderImpl < ProxyMessageFlags::Enum >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const &, ProxyMessageFlags::Enum & rv)
                    {
                        if (value == L"-")
                        {
                            rv = ProxyMessageFlags::None;
                            return true;
                        }

                        int flags = 0;
                        std::vector<std::wstring> parts;
                        Common::StringUtility::Split<std::wstring>(value, parts, L"|");

                        for (auto it = parts.cbegin(); it != parts.cend(); ++it)
                        {
                            if (*it == L"ER")
                            {
                                flags |= ProxyMessageFlags::EndReconfiguration;
                            }
                            else if (*it == L"C")
                            {
                                flags |= ProxyMessageFlags::Catchup;
                            }
                            else if (*it == L"A")
                            {
                                flags |= ProxyMessageFlags::Abort;
                            }
                            else if (*it == L"D")
                            {
                                flags |= ProxyMessageFlags::Drop;
                            }
                            else
                            {
                                Verify::Fail(Common::wformatString("Unknown proxy message flag {0}. Expected either ER|C|A|D", *it));
                                return false;
                            }
                        }

                        rv = static_cast<ProxyMessageFlags::Enum>(flags);
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < ::FABRIC_QUERY_SERVICE_REPLICA_STATUS >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const &, FABRIC_QUERY_SERVICE_REPLICA_STATUS & rv)
                    {
                        EnumReader<FABRIC_QUERY_SERVICE_REPLICA_STATUS> enumReader(L"QueryReplicaState");
                        enumReader.Add(FABRIC_QUERY_SERVICE_REPLICA_STATUS_DOWN, L"D", L"Down");
                        enumReader.Add(FABRIC_QUERY_SERVICE_REPLICA_STATUS_INBUILD, L"IB", L"InBuild");
                        enumReader.Add(FABRIC_QUERY_SERVICE_REPLICA_STATUS_READY, L"RD", L"Ready");
                        enumReader.Add(FABRIC_QUERY_SERVICE_REPLICA_STATUS_DROPPED, L"DD", L"Dropped");
                        enumReader.Add(FABRIC_QUERY_SERVICE_REPLICA_STATUS_STANDBY, L"SB", L"StandBy");
                        return enumReader.Read(value, rv);
                    }
                };

#pragma endregion

                template<>
                struct ReaderImpl < Reliability::GenerationNumber >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & rc, Reliability::GenerationNumber & rv)
                    {
                        ErrorLogger errLogger(L"Reliability::GenerationNumber", value);
                        Reader reader(value, rc);

                        int64 nodeId, genNumber;
                        if (!reader.Read<int64>(L':', nodeId))
                        {
                            errLogger.Log(L"NodeId");
                            return false;
                        }

                        if (!reader.Read<int64>(L' ', genNumber))
                        {
                            errLogger.Log(L"Reliability::GenerationNumber");
                            return false;
                        }

                        rv = Reliability::GenerationNumber(genNumber, Federation::NodeId(Common::LargeInteger(0, static_cast<uint64>(nodeId))));
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < Reliability::GenerationHeader >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & rc, Reliability::GenerationHeader & rv)
                    {
                        ErrorLogger errLogger(L"GenerationHeader", value);
                        Reader reader(value, rc);

                        Reliability::GenerationNumber genNumber;

                        if (!ValidatePrefix(reader, errLogger))
                        {
                            return false;
                        }

                        if (!reader.Read(L',', genNumber))
                        {
                            errLogger.Log(L"Reliability::GenerationNumber");
                            return false;
                        }

                        bool isForFMReplica = false;
                        std::wstring suffix = reader.ReadString(L' ');
                        if (suffix == L"fmm")
                        {
                            isForFMReplica = true;
                        }
                        else if (suffix == L"fm")
                        {
                            isForFMReplica = false;
                        }
                        else
                        {
                            errLogger.Log(L"unrecognized suffix" + suffix);
                            return false;
                        }

                        rv = Reliability::GenerationHeader(genNumber, isForFMReplica);
                        return true;
                    }

                    static bool ValidatePrefix(Reader & reader, ErrorLogger & errLogger)
                    {
                        if (reader.PeekChar() != L'g')
                        {
                            errLogger.Log(L"GenerationHeader: Invalid Prefix");
                            return false;
                        }

                        // skip g: prefix
                        reader.ReadChar();

                        if (reader.PeekChar() != L':')
                        {
                            errLogger.Log(L"GenerationHeader: Invalid Prefix");
                            return false;
                        }

                        reader.ReadChar();

                        return true;
                    }
                };

                struct PackageVersionAndLsnInfo
                {
                    ServiceModel::ServicePackageVersionInstance SPVI;
                    int64 FirstAcknowledgedLSN;
                    int64 LastAcknowledgedLSN;
                    wstring ReplicatorEndpoint;
                    wstring ServiceEndpoint;
                };

                template<>
                struct ReaderImpl<PackageVersionAndLsnInfo>
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & raw, PackageVersionAndLsnInfo & rv)
                    {
                        ErrorLogger errLogger(L"PackageVersionAndLsnInfo", value);
                        Reader reader(value, raw);

                        int64 firstAcknowledgedLSN = -1;
                        int64 lastAcknowledgedLSN = -1;

                        ServiceModel::ServicePackageVersionInstance spvi;
                        auto derivedPtr = raw.TryGetDerived<SingleFTReadContext>();
                        if (derivedPtr != nullptr)
                        {
                            spvi = derivedPtr->STInfo.ServicePackageVersionInstance;
                        }

                        std::wstring serviceEndpoint;
                        std::wstring replicatorEndpoint;

                        if (!reader.IsEOF)
                        {
                            if (reader.PeekChar() == L'n')
                            {
                                ServiceModel::RolloutVersion zero(0, 0);
                                ServiceModel::ApplicationVersion appVersion(zero);
                                spvi = ServiceModel::ServicePackageVersionInstance(ServiceModel::ServicePackageVersion(zero, zero), 0);
                            }
                            else if (reader.PeekChar() == L'(')
                            {
                                if (!reader.Read<ServiceModel::ServicePackageVersionInstance>(L'(', L')', spvi))
                                {
                                    errLogger.Log(L"Service Package Version Instance");
                                    return false;
                                }
                            }
                            else if (iswalpha(reader.PeekChar()))
                            {
                                if (!reader.IsEOF)
                                {
                                    serviceEndpoint = reader.ReadString(L' ');
                                }

                                if (!reader.IsEOF)
                                {
                                    replicatorEndpoint = reader.ReadString(L' ');
                                }
                            }
                            else
                            {
                                if (!reader.Read<int64>(L' ', firstAcknowledgedLSN))
                                {
                                    errLogger.Log(L"FirstAckLSN");
                                    return false;
                                }

                                if (!reader.Read<int64>(L' ', lastAcknowledgedLSN))
                                {
                                    errLogger.Log(L"LastAckLSN");
                                    return false;
                                }
                            }
                        }

                        rv.SPVI = spvi;
                        rv.FirstAcknowledgedLSN = firstAcknowledgedLSN;
                        rv.LastAcknowledgedLSN = lastAcknowledgedLSN;
                        rv.ReplicatorEndpoint = replicatorEndpoint;
                        rv.ServiceEndpoint = serviceEndpoint;
                        return true;
                    }
                };

                static bool ParseLoadEntry(Reader & reader, ErrorLogger & errLogger, Reliability::LoadBalancingComponent::LoadOrMoveCostDescription & loadReports)
                {
                    uint64 load = 0;
                    Federation::NodeId nodeId = Federation::NodeId();
                    bool isPrimary = false;
                    wchar_t role = reader.PeekChar();

                    if (role == L'S')
                    {
                        isPrimary = false;
                    }
                    else if (role == L'P')
                    {
                        isPrimary = true;
                    }
                    else
                    {
                        reader.ReadChar();
                        return true;
                    }

                    reader.ReadString(L'\\');
                    wstring metricName = reader.ReadString(L'\\');
                    if (!reader.Read<uint64>(L'\\', load))
                    {
                        errLogger.Log(L"MetricLoad");
                        return false;
                    }

                    if (!isPrimary)
                    {
                        if (!Federation::NodeId::TryParse(reader.ReadString(L' '), nodeId))
                        {
                            errLogger.Log(L"NodeId");
                            return false;
                        }
                    }

                    Reliability::LoadBalancingComponent::LoadMetric loadObject(move(metricName), (uint32)load);
                    vector<Reliability::LoadBalancingComponent::LoadMetric> loadVector;
                    loadVector.push_back(move(loadObject));

                    if (isPrimary)
                    {
                        loadReports.MergeLoads(Reliability::LoadBalancingComponent::ReplicaRole::Primary, move(loadVector), Stopwatch::Now());
                    }
                    else
                    {
                        loadReports.MergeLoads(Reliability::LoadBalancingComponent::ReplicaRole::Secondary, move(loadVector), Stopwatch::Now(), true, nodeId);
                    }

                    return true;
                }

                template<>
                struct ReaderImpl < Reliability::LoadBalancingComponent::LoadOrMoveCostDescription>
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & raw, Reliability::LoadBalancingComponent::LoadOrMoveCostDescription & rv)
                    {
                        Reader reader(value, raw);
                        ErrorLogger errLogger(L"Reliability::LoadBalancingComponent::LoadOrMoveCostDescription", value);

                        PartitionId partitionId = PartitionId(Common::Guid::Empty());
                        wstring serviceName = L"";
                        bool isStateful = false;

                        if (!reader.Read<Reliability::PartitionId>(L' ', partitionId))
                        {
                            errLogger.Log(L"PartitionId");
                            return false;
                        }

                        serviceName = reader.ReadString(L' ');
                        if (!reader.Read<bool>(L' ', isStateful))
                        {
                            errLogger.Log(L"IsStateful");
                            return false;
                        }

                        rv = Reliability::LoadBalancingComponent::LoadOrMoveCostDescription(partitionId.Guid, move(serviceName), isStateful);

                        reader.SkipDelimiters();

                        while (!reader.IsEOF)
                        {
                            if (!ParseLoadEntry(reader, errLogger, rv))
                            {
                                return false;
                            }
                        }

                        return true;
                    }
                };

                template<>
                struct ReaderImpl < Reliability::ReportLoadMessageBody>
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & raw, Reliability::ReportLoadMessageBody & rv)
                    {
                        Reader reader(value, raw);
                        ErrorLogger errLogger(L"Reliability::ReportLoadMessageBody", value);
                        std::vector<Reliability::LoadBalancingComponent::LoadOrMoveCostDescription> loads;

                        if (!reader.ReadVector<Reliability::LoadBalancingComponent::LoadOrMoveCostDescription>(L'[', L']', loads))
                        {
                            errLogger.Log(L"Reports");
                            return false;
                        }

                        rv = Reliability::ReportLoadMessageBody(move(loads), Stopwatch::Now());

                        return true;
                    }
                };

                template<>
                struct ReaderImpl < Management::ResourceMonitor::ResourceUsage>
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & raw, Management::ResourceMonitor::ResourceUsage & rv)
                    {
                        Reader reader(value, raw);
                        ErrorLogger errLogger(L"Management::ResourceMonitor::ResourceUsage", value);

                        uint64 memory = 0;
                        double cpuLoad = 0.0;

                        if (!reader.Read<uint64>(L' ', memory))
                        {
                            errLogger.Log(L"Memory");
                            return false;
                        }
                        if (!reader.Read<double>(L' ', cpuLoad))
                        {
                            errLogger.Log(L"Cpu");
                            return false;
                        }

                        rv = Management::ResourceMonitor::ResourceUsage(cpuLoad, memory, cpuLoad, memory);

                        return true;
                    }
                };

                template<>
                struct ReaderImpl < Management::ResourceMonitor::ResourceUsageReport>
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & raw, Management::ResourceMonitor::ResourceUsageReport & rv)
                    {
                        Reader reader(value, raw);
                        ErrorLogger errLogger(L"Management::ResourceMonitor::ResourceUsageReport", value);
                        std::map<Reliability::PartitionId, Management::ResourceMonitor::ResourceUsage> reports;

                        if (!reader.ReadMap<Reliability::PartitionId, Management::ResourceMonitor::ResourceUsage>(L'[', L']', reports))
                        {
                            errLogger.Log(L"ResourceUsageReports");
                            return false;
                        }

                        rv = Management::ResourceMonitor::ResourceUsageReport(move(reports));

                        return true;
                    }
                };

                template<>
                struct ReaderImpl < Reliability::ReplicaDescription >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & raw, Reliability::ReplicaDescription & rv)
                    {
                        ErrorLogger errLogger(L"Reliability::ReplicaDescription", value);
                        Reader reader(value, raw);

                        Reliability::ReplicaRole::Enum pcRole;
                        Reliability::ReplicaRole::Enum ccRole;
                        ReplicaStates::Enum state;
                        bool isUp;


                        if (!reader.Read<Reliability::ReplicaRole::Enum>(L'/', pcRole))
                        {
                            errLogger.Log(L"pcRole");
                            return false;
                        }

                        if (!reader.Read<Reliability::ReplicaRole::Enum>(L' ', ccRole))
                        {
                            errLogger.Log(L"ccRole");
                            return false;
                        }

                        if (!reader.Read<ReplicaStates::Enum>(L' ', state))
                        {
                            errLogger.Log(L"State");
                            return false;
                        }

                        isUp = reader.ReadString(L' ') == L"U";

                        ReplicaAndNodeId replicaAndNodeId;
                        if (!reader.Read<ReplicaAndNodeId>(L' ', replicaAndNodeId))
                        {
                            errLogger.Log(L"ReplicaID");
                            return false;
                        }

                        PackageVersionAndLsnInfo info;
                        if (!reader.ReadOptional<PackageVersionAndLsnInfo>(L'\0', info))
                        {
                            errLogger.Log(L"PackageVersionAndLsnInfo");
                            return false;
                        }

                        rv = Reliability::ReplicaDescription(
                            replicaAndNodeId.CreateNodeInstance(),
                            replicaAndNodeId.ReplicaId,
                            replicaAndNodeId.ReplicaInstanceId,
                            pcRole,
                            ccRole,
                            ReplicaStates::ToReplicaDescriptionState(state),
                            isUp,
                            info.LastAcknowledgedLSN,
                            info.FirstAcknowledgedLSN,
                            info.ServiceEndpoint,
                            info.ReplicatorEndpoint,
                            info.SPVI);

                        return true;
                    }
                };

                template<>
                struct ReaderImpl < Reliability::FailoverUnitDescription >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & rawContext, Reliability::FailoverUnitDescription & rv)
                    {
                        SingleFTReadContext const & context = rawContext.GetDerived<SingleFTReadContext>();

                        ErrorLogger errLogger(L"Reliability::FailoverUnitDescription", value);
                        Reader reader(value, context);

                        Reliability::Epoch pcEpoch, ccEpoch;

                        if (!reader.Read<Reliability::Epoch>(L'/', pcEpoch))
                        {
                            errLogger.Log(L"pcEpoch");
                            return false;
                        }

                        if (!reader.Read<Reliability::Epoch>(L' ', ccEpoch))
                        {
                            errLogger.Log(L"ccEpoch");
                            return false;
                        }

                        rv = Reliability::FailoverUnitDescription(context.FUID, Reliability::ConsistencyUnitDescription(context.CUID), ccEpoch, pcEpoch);
                        return true;
                    }
                };


                template<>
                struct ReaderImpl < ServiceModel::RolloutVersion >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, ServiceModel::RolloutVersion & rv)
                    {
                        ErrorLogger errLogger(L"ServiceModel::RolloutVersion", value);
                        Reader reader(value, context);

                        ULONG major, minor;

                        if (!reader.Read<ULONG>(L'.', major))
                        {
                            errLogger.Log(L"Major");
                            return false;
                        }

                        if (!reader.Read<ULONG>(L' ', minor))
                        {
                            errLogger.Log(L"Minor");
                            return false;
                        }

                        rv = ServiceModel::RolloutVersion(major, minor);
                        return true;
                    }
                };

                template <>
                struct ReaderImpl< Reliability::ServiceTypeInfo >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, Reliability::ServiceTypeInfo & rv)
                    {
                        ErrorLogger errLogger(L"ServiceModel::ServicePackageVersionInstance", value);
                        Reader reader(value, context);

                        auto const & casted = context.GetDerived<MultipleReadContextContainer>();

                        auto stContext = casted.GetSingleSDContext(value);
                        if (stContext == nullptr)
                        {
                            errLogger.Log(L"Context not found");
                            return false;
                        }

                        rv = Reliability::ServiceTypeInfo(
                            ServiceModel::VersionedServiceTypeIdentifier(stContext->ServiceTypeId, stContext->ServicePackageVersionInstance), 
                            stContext->CodePackageName);
                        return true;
                    }
                };

                template <>
                struct ReaderImpl< ServiceModel::ServiceTypeIdentifier >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, ServiceModel::ServiceTypeIdentifier & rv)
                    {
                        ErrorLogger errLogger(L"ServiceModel::ServiceTypeIdentifier", value);
                        Reader reader(value, context);

                        auto const & casted = context.GetDerived<MultipleReadContextContainer>();

                        auto stContext = casted.GetSingleSDContext(value);
                        if (stContext == nullptr)
                        {
                            errLogger.Log(L"Context not found");
                            return false;
                        }

                        rv = stContext->ServiceTypeId;
                        return true;
                    }
                };

                template <>
                struct ReaderImpl< Reliability::PartitionId >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, Reliability::PartitionId & rv)
                    {
                        ErrorLogger errLogger(L"Reliability::PartitionId", value);
                        Reader reader(value, context);

                        std::wstring guidString(value);
                        Common::StringUtility::TrimSpaces(guidString);

                        Common::Guid guid;
                        bool success = Common::Guid::TryParse(guidString, guid);
                        if (!success)
                        {
                            errLogger.Log(L"Failed to parse Guid");
                            return false;
                        }

                        rv = Reliability::PartitionId(guid);
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < ServiceModel::ServicePackageVersionInstance >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, ServiceModel::ServicePackageVersionInstance & rv)
                    {
                        ErrorLogger errLogger(L"ServiceModel::ServicePackageVersionInstance", value);
                        Reader reader(value, context);

                        // 1:0:1:0:1 where first is the app version (represented as a rollout version), second is the rollout version and third is the instance
                        ServiceModel::RolloutVersion appVersionRolloutVersion;
                        if (!reader.Read<ServiceModel::RolloutVersion>(L':', appVersionRolloutVersion))
                        {
                            errLogger.Log(L"ApplicationVersion");
                            return false;
                        }

                        ServiceModel::RolloutVersion rolloutVersion;
                        if (!reader.Read<ServiceModel::RolloutVersion>(L':', rolloutVersion))
                        {
                            errLogger.Log(L"RolloutVersion");
                            return false;
                        }

                        uint64 instance;
                        if (!reader.Read<uint64>(L' ', instance))
                        {
                            errLogger.Log(L"Instance");
                            return false;
                        }

                        ServiceModel::ApplicationVersion appVersion(appVersionRolloutVersion);
                        rv = ServiceModel::ServicePackageVersionInstance(ServiceModel::ServicePackageVersion(appVersion, rolloutVersion), instance);
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < Common::FabricCodeVersion >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, Common::FabricCodeVersion & rv)
                    {
                        ErrorLogger errLogger(L"FabricVersion", value);
                        Reader reader(value, context);

                        ULONG major, minor, hotfix;

                        if (!reader.Read<ULONG>(L'.', major))
                        {
                            errLogger.Log(L"Major");
                            return false;
                        }

                        if (!reader.Read<ULONG>(L'.', minor))
                        {
                            errLogger.Log(L"minor");
                            return false;
                        }

                        if (!reader.Read<ULONG>(L':', hotfix))
                        {
                            errLogger.Log(L"hotfix");
                            return false;
                        }

                        rv = Common::FabricCodeVersion(major, minor, hotfix, 0);
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < Common::FabricVersionInstance >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, Common::FabricVersionInstance & rv)
                    {
                        ErrorLogger errLogger(L"FabricVersionInstance", value);
                        Reader reader(value, context);

                        Common::FabricCodeVersion codeVersion;
                        uint64 instance;
                        std::wstring cfg;

                        if (!reader.Read(L':', codeVersion))
                        {
                            errLogger.Log(L"CodeVersion");
                            return false;
                        }

                        cfg = reader.ReadString(L':');

                        Common::FabricConfigVersion configVersion;
                        auto error = configVersion.FromString(cfg);
                        if (!error.IsSuccess())
                        {
                            errLogger.Log(L"ConfigVersion");
                            return false;
                        }

                        if (!reader.Read<uint64>(L'\0', instance))
                        {
                            errLogger.Log(L"Instance");
                            return false;
                        }

                        rv = Common::FabricVersionInstance(
                            Common::FabricVersion(codeVersion, configVersion),
                            instance);

                        return true;
                    }
                };

                template<>
                struct ReaderImpl < Reliability::ReplicaDeactivationInfo >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, Reliability::ReplicaDeactivationInfo & rv)
                    {
                        ErrorLogger errLogger(L"Reliability::ReplicaDeactivationInfo", value);
                        Reader reader(value, context);

                        Reliability::Epoch epoch;
                        int64 catchupLSN;

                        if (!reader.Read(L':', epoch))
                        {
                            errLogger.Log(L"DeactivationEpoch");
                            return false;
                        }

                        if (!reader.Read(L' ', catchupLSN))
                        {
                            errLogger.Log(L"CatchupLSN");
                            return false;
                        }

                        if (epoch.IsInvalid())
                        {
                            if (catchupLSN == FABRIC_INVALID_SEQUENCE_NUMBER)
                            {
                                rv = Reliability::ReplicaDeactivationInfo();
                            }
                            else if (catchupLSN == Constants::UnknownLSN)
                            {
                                rv = Reliability::ReplicaDeactivationInfo::Dropped;
                            }
                            else
                            {
                                errLogger.Log(L"DeactivationInfo");
                                return false;
                            }
                        }
                        else
                        {
                            rv = Reliability::ReplicaDeactivationInfo(epoch, catchupLSN);
                        }
                        return true;
                    }

                    static Reliability::ReplicaDeactivationInfo GetDefault(Reliability::Epoch const & epoch)
                    {
                        return Reliability::ReplicaDeactivationInfo(epoch, 0);
                    }

                    static Reliability::ReplicaDeactivationInfo GetDefault(Reliability::FailoverUnitDescription const & ftDesc)
                    {
                        return GetDefault(ftDesc.CurrentConfigurationEpoch);
                    }

                    static bool ReadOptional(Reader & reader, Reliability::Epoch const & epoch, Reliability::ReplicaDeactivationInfo & rv)
                    {
                        if (reader.PeekChar() == L'{')
                        {
                            return reader.Read(L'{', L'}', rv);
                        }
                        else
                        {
                            rv = ReaderImpl<Reliability::ReplicaDeactivationInfo>::GetDefault(epoch);
                            return true;
                        }
                    }

                    static bool ReadOptional(Reader & reader, Reliability::FailoverUnitDescription const & ftDesc, Reliability::ReplicaDeactivationInfo & rv)
                    {
                        return ReadOptional(reader, ftDesc.CurrentConfigurationEpoch, rv);
                    }
                };

                template<>
                struct ReaderImpl < Reliability::ReplicaInfo >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, Reliability::ReplicaInfo & rv)
                    {
                        ErrorLogger errLogger(L"Reliability::ReplicaInfo", value);
                        Reader reader(value, context);

                        Reliability::ReplicaRole::Enum pcRole, icRole, ccRole;

                        if (!reader.Read<Reliability::ReplicaRole::Enum>(L'/', pcRole))
                        {
                            errLogger.Log(L"pcRole");
                            return false;
                        }

                        if (!reader.Read<Reliability::ReplicaRole::Enum>(L'/', icRole))
                        {
                            errLogger.Log(L"icRole");
                            return false;
                        }

                        if (!reader.Read<Reliability::ReplicaRole::Enum>(L' ', ccRole))
                        {
                            errLogger.Log(L"ccRole");
                            return false;
                        }

                        Reliability::ReplicaStates::Enum state;
                        if (!reader.Read<Reliability::ReplicaStates::Enum>(L' ', state))
                        {
                            errLogger.Log(L"State");
                            return false;
                        }

                        bool isUp = reader.ReadString(L' ') == L"U";

                        ReplicaAndNodeId replicaIdAndNodeId;
                        if (!reader.Read<ReplicaAndNodeId>(L' ', replicaIdAndNodeId))
                        {
                            errLogger.Log(L"ReplicaId");
                            return false;
                        }

                        PackageVersionAndLsnInfo info;
                        if (!reader.ReadOptional<PackageVersionAndLsnInfo>(L'\0', info))
                        {
                            errLogger.Log(L"info");
                            return false;
                        }
                        
                        Reliability::ReplicaDescription description(
                            replicaIdAndNodeId.CreateNodeInstance(),
                            replicaIdAndNodeId.ReplicaId,
                            replicaIdAndNodeId.ReplicaInstanceId,
                            pcRole,
                            ccRole,
                            state,
                            isUp,
                            info.LastAcknowledgedLSN,
                            info.FirstAcknowledgedLSN,
                            info.ServiceEndpoint,
                            info.ReplicatorEndpoint,
                            info.SPVI);

                        rv = Reliability::ReplicaInfo(description, icRole);

                        return true;
                    }
                };

                template<>
                struct ReaderImpl < Reliability::FailoverUnitInfo >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & rawContext, Reliability::FailoverUnitInfo & rv)
                    {
                        SingleFTReadContext const & thisFTContext = rawContext.GetDerived<SingleFTReadContext>();

                        ErrorLogger errLogger(L"FTInfo", value);
                        Reader reader(value, thisFTContext);

                        Reliability::Epoch pcEpoch, icEpoch, ccEpoch;

                        if (!reader.Read<Reliability::Epoch>(L'/', pcEpoch))
                        {
                            errLogger.Log(L"pcEpoch");
                            return false;
                        }

                        if (!reader.Read<Reliability::Epoch>(L'/', icEpoch))
                        {
                            errLogger.Log(L"icEpoch");
                            return false;
                        }

                        if (!reader.Read<Reliability::Epoch>(L' ', ccEpoch))
                        {
                            errLogger.Log(L"ccEpoch");
                            return false;
                        }

                        std::vector<Reliability::ReplicaInfo> replicas;
                        if (!reader.ReadVector<Reliability::ReplicaInfo>(L'[', L']', replicas))
                        {
                            errLogger.Log(L"Replicas");
                            return false;
                        }

                        bool isReportFromPrimary = false;
                        if (!reader.IsEOF)
                        {
                            if (!reader.Read(L'(', L')', isReportFromPrimary))
                            {
                                errLogger.Log(L"IsReportFromPrimary");
                                return false;
                            }
                        }

                        rv = Reliability::FailoverUnitInfo(thisFTContext.STInfo.SD,
                            Reliability::FailoverUnitDescription(thisFTContext.FUID, Reliability::ConsistencyUnitDescription(thisFTContext.CUID), ccEpoch, pcEpoch),
                            icEpoch,
                            isReportFromPrimary,
                            false,
                            std::move(replicas));
                        return true;
                    }
                };

                template <>
                struct ReaderImpl < FailoverUnitProxy::ConfigurationReplicaStore >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & rawContext, FailoverUnitProxy::ConfigurationReplicaStore & rv)
                    {
                        ErrorLogger errLogger(L"FTInfo", value);
                        Reader reader(value, rawContext);

                        std::vector<Reliability::ReplicaDescription> replicas;

                        if (!reader.ReadVector<Reliability::ReplicaDescription>(L'[', L']', replicas))
                        {
                            errLogger.Log(L"replicas");
                            return false;
                        }

                        for (auto const & it : replicas)
                        {
                            rv[it.FederationNodeId] = it;
                        }
                        
                        return true;
                    }
                }; 

#pragma region Message Body

                template <typename TBody>
                inline bool ReadServiceTypeNotificationMessageBody(
                    std::wstring const & desc, 
                    std::wstring const & value, 
                    ReadContext const & context, 
                    TBody & rv)
                {
                    ErrorLogger errLogger(desc, value);
                    Reader reader(value, context);

                    std::vector<Reliability::ServiceTypeInfo> v;
                    if (!reader.ReadVector(L'[', L']', v))
                    {
                        errLogger.Log(L"ServiceTypeInfos");
                        return false;
                    }

                    rv = TBody(std::move(v));
                    return true;
                }

                template <>
                struct ReaderImpl < Reliability::ServiceTypeNotificationRequestMessageBody >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, Reliability::ServiceTypeNotificationRequestMessageBody & rv)
                    {
                        return ReadServiceTypeNotificationMessageBody(L"ServiceTypeNotificationRequest", value, context, rv);
                    }
                };

                template <>
                struct ReaderImpl < Reliability::ServiceTypeNotificationReplyMessageBody >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, Reliability::ServiceTypeNotificationReplyMessageBody & rv)
                    {
                        return ReadServiceTypeNotificationMessageBody(L"ServiceTypeNotificationReplyMessageBody ", value, context, rv);
                    }
                };

                template <>
                struct ReaderImpl < Reliability::ServiceTypeUpdateMessageBody >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, Reliability::ServiceTypeUpdateMessageBody & rv)
                    {
                        Reader reader(value, context);
                        ErrorLogger errLogger(L"ServiceTypeUpdateMessageBody", value);

                        std::vector<ServiceModel::ServiceTypeIdentifier> serviceTypeIds;
                        if (!reader.ReadVector(L'[', L']', serviceTypeIds))
                        {
                            errLogger.Log(L"ServiceTypeInfos");
                            return false;
                        }

                        uint64 sequenceNumber;
                        if (!reader.Read(L' ', sequenceNumber))
                        {
                            errLogger.Log(L"SequenceNumber");
                            return false;
                        }

                        rv = ServiceTypeUpdateMessageBody(std::move(serviceTypeIds), sequenceNumber);
                        return true;
                    }
                };

                template <>
                struct ReaderImpl < Reliability::PartitionNotificationMessageBody >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, Reliability::PartitionNotificationMessageBody & rv)
                    {
                        Reader reader(value, context);
                        ErrorLogger errLogger(L"PartitionNotificationMessageBody", value);

                        std::vector<PartitionId> partitionIds;
                        if (!reader.ReadVector(L'[', L']', partitionIds))
                        {
                            errLogger.Log(L"PartitionIds");
                            return false;
                        }

                        uint64 sequenceNumber;
                        if (!reader.Read(L' ', sequenceNumber))
                        {
                            errLogger.Log(L"SequenceNumber");
                            return false;
                        }

                        rv = PartitionNotificationMessageBody(std::move(partitionIds), sequenceNumber);
                        return true;
                    }
                };

                template <>
                struct ReaderImpl < Reliability::ServiceTypeUpdateReplyMessageBody >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, Reliability::ServiceTypeUpdateReplyMessageBody & rv)
                    {
                        Reader reader(value, context);
                        ErrorLogger errLogger(L"ServiceTypeUpdateReplyMessageBody", value);

                        uint64 sequenceNumber;
                        if (!reader.Read(L' ', sequenceNumber))
                        {
                            errLogger.Log(L"SequenceNumber");
                            return false;
                        }

                        rv = ServiceTypeUpdateReplyMessageBody(sequenceNumber);
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < ServiceModel::ServicePackageUpgradeSpecification >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, ServiceModel::ServicePackageUpgradeSpecification & rv)
                    {
                        ErrorLogger errLogger(L"ServicePackageUpgradeSpecification", value);
                        Reader reader(value, context);

                        std::wstring packageName = reader.ReadString(L' ');
                        ServiceModel::RolloutVersion version;
                        if (!reader.Read(L'\0', version))
                        {
                            errLogger.Log(L"RolloutVersion");
                            return false;
                        }

                        rv = ServiceModel::ServicePackageUpgradeSpecification(
                            packageName, 
                            version, 
                            std::vector<std::wstring>()); // code package names
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < ServiceModel::ServiceTypeRemovalSpecification >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, ServiceModel::ServiceTypeRemovalSpecification & rv)
                    {
                        ErrorLogger errLogger(L"ServiceTypeRemovalSpecification", value);
                        Reader reader(value, context);

                        std::wstring packageName = reader.ReadString(L' ');
                        std::wstring serviceTypeName = reader.ReadString(L' ');

                        rv = ServiceModel::ServiceTypeRemovalSpecification(packageName, ServiceModel::RolloutVersion());
                        rv.AddServiceTypeName(serviceTypeName);
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < Reliability::UpgradeDescription >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, Reliability::UpgradeDescription & rv)
                    {
                        ErrorLogger errLogger(L"Reliability::UpgradeDescription", value);
                        Reader reader(value, context);

                        uint64 instanceId;
                        if (!reader.Read<uint64>(L' ', instanceId))
                        {
                            errLogger.Log(L"InstanceId");
                            return false;
                        }

                        ServiceModel::UpgradeType::Enum upgradeType;
                        if (!reader.Read(L' ', upgradeType))
                        {
                            errLogger.Log(L"UpgradeType");
                            return false;
                        }

                        bool isMonitored;
                        if (!reader.Read(L' ', isMonitored))
                        {
                            errLogger.Log(L"IsMonitored");
                            return false;
                        }

                        std::vector<ServiceModel::ServiceTypeRemovalSpecification> removalSpec;
                        std::vector<ServiceModel::ServicePackageUpgradeSpecification> v;
                        if (!reader.ReadVector(L'[', L']', v))
                        {
                            errLogger.Log(L"ServicePackageUpgradeSpecification");
                            return false;
                        }

                        if (!reader.IsEOF)
                        {
                            // skip delimiter between lists
                            reader.ReadString(L'|');

                            if (!reader.IsEOF)
                            {
                                if (!reader.ReadVector(L'[', L']', removalSpec))
                                {
                                    errLogger.Log(L"removalSpec");
                                    return false;
                                }
                            }
                        }


                        ServiceModel::ApplicationUpgradeSpecification appUpgradeSpec(
                            Default::GetInstance().ApplicationIdentifier,
                            Default::GetInstance().UpgradedApplicationVersion,
                            Default::GetInstance().ApplicationName,
                            instanceId,
                            upgradeType,
                            isMonitored,
                            false,
                            std::move(v),
                            std::move(removalSpec));

                        rv = Reliability::UpgradeDescription(std::move(appUpgradeSpec));
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < Reliability::NodeFabricUpgradeReplyMessageBody >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, Reliability::NodeFabricUpgradeReplyMessageBody & rv)
                    {
                        ErrorLogger errLogger(L"Reliability::NodeFabricUpgradeReplyMessageBody", value);
                        Reader reader(value, context);

                        Common::FabricVersionInstance version;
                        if (!reader.Read<Common::FabricVersionInstance>(L'\0', version))
                        {
                            errLogger.Log(L"Instance");
                            return false;
                        }

                        rv = Reliability::NodeFabricUpgradeReplyMessageBody(version);
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < Reliability::ConfigurationMessageBody >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & raw, Reliability::ConfigurationMessageBody & rv)
                    {
                        auto context = raw.GetDerived<SingleFTReadContext>();
                        ErrorLogger errLogger(L"Reliability::ConfigurationMessageBody", value);
                        Reader reader(value, context);

                        Reliability::FailoverUnitDescription ftDesc;
                        if (!reader.Read<Reliability::FailoverUnitDescription>(L' ', ftDesc))
                        {
                            errLogger.Log(L"FTDesc");
                            return false;
                        }

                        std::vector<Reliability::ReplicaDescription> replicas;
                        if (!reader.ReadVector<Reliability::ReplicaDescription>(L'[', L']', replicas))
                        {
                            errLogger.Log(L"Replicas");
                            return false;
                        }

                        rv = Reliability::ConfigurationMessageBody(ftDesc, context.STInfo.SD, std::move(replicas));
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < Reliability::DoReconfigurationMessageBody >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & raw, Reliability::DoReconfigurationMessageBody  & rv)
                    {
                        auto context = raw.GetDerived<SingleFTReadContext>();
                        ErrorLogger errLogger(L"Reliability::ConfigurationMessageBody", value);
                        Reader reader(value, context);

                        Reliability::FailoverUnitDescription ftDesc;
                        if (!reader.Read<Reliability::FailoverUnitDescription>(L' ', ftDesc))
                        {
                            errLogger.Log(L"FTDesc");
                            return false;
                        }

                        std::vector<Reliability::ReplicaDescription> replicas;
                        if (!reader.ReadVector<Reliability::ReplicaDescription>(L'[', L']', replicas))
                        {
                            errLogger.Log(L"Replicas");
                            return false;
                        }

                        Common::TimeSpan phase0Duration = Common::TimeSpan::MaxValue;
                        if (!reader.IsEOF)
                        {
                            int seconds = 0;
                            if (!reader.Read<int>(L' ', seconds))
                            {
                                errLogger.Log(L"Phase0 Duration");
                                return false;
                            }

                            phase0Duration = Common::TimeSpan::FromSeconds(seconds);
                        }

                        rv = Reliability::DoReconfigurationMessageBody(ftDesc, context.STInfo.SD, std::move(replicas), phase0Duration);
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < Reliability::ActivateMessageBody >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & raw, Reliability::ActivateMessageBody  & rv)
                    {
                        auto context = raw.GetDerived<SingleFTReadContext>();
                        ErrorLogger errLogger(L"Reliability::ActivateMessageBody", value);
                        Reader reader(value, context);

                        Reliability::FailoverUnitDescription ftDesc;
                        if (!reader.Read<Reliability::FailoverUnitDescription>(L' ', ftDesc))
                        {
                            errLogger.Log(L"FTDesc");
                            return false;
                        }

                        Reliability::ReplicaDeactivationInfo replicaDeactivationInfo;
                        if (!ReaderImpl<Reliability::ReplicaDeactivationInfo>::ReadOptional(reader, ftDesc, replicaDeactivationInfo))
                        {
                            errLogger.Log(L"DeactivationInfo");
                            return false;
                        }
                        
                        std::vector<Reliability::ReplicaDescription> replicas;
                        if (!reader.ReadVector<Reliability::ReplicaDescription>(L'[', L']', replicas))
                        {
                            errLogger.Log(L"Replicas");
                            return false;
                        }

                        rv = Reliability::ActivateMessageBody(ftDesc, context.STInfo.SD, std::move(replicas), replicaDeactivationInfo);
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < Reliability::DeactivateMessageBody >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & raw, Reliability::DeactivateMessageBody & rv)
                    {
                        auto context = raw.GetDerived<SingleFTReadContext>();
                        ErrorLogger errLogger(L"Reliability::DeactivateMessageBody", value);
                        Reader reader(value, context);

                        Reliability::Epoch deactivationEpoch;
                        Reliability::FailoverUnitDescription ftDesc;
                        if (!reader.Read<Reliability::FailoverUnitDescription>(L' ', ftDesc))
                        {
                            errLogger.Log(L"FTDesc");
                            return false;
                        }

                        Reliability::ReplicaDeactivationInfo replicaDeactivationInfo;
                        if (!ReaderImpl<Reliability::ReplicaDeactivationInfo>::ReadOptional(reader, ftDesc, replicaDeactivationInfo))
                        {
                            errLogger.Log(L"DeactivationInfo");
                            return false;
                        }

                        bool isForce = false;
                        if (!reader.Read<bool>(L' ', isForce))
                        {
                            errLogger.Log(L"IsForce");
                            return false;
                        }

                        std::vector<Reliability::ReplicaDescription> replicas;
                        if (!reader.ReadVector<Reliability::ReplicaDescription>(L'[', L']', replicas))
                        {
                            errLogger.Log(L"Replicas");
                            return false;
                        }

                        rv = Reliability::DeactivateMessageBody(ftDesc, context.STInfo.SD, std::move(replicas), replicaDeactivationInfo, isForce);
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < ReportFaultMessageBody >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, ReportFaultMessageBody & rv)
                    {
                        ErrorLogger errLogger(L"ReportFaultMessageBody", value);
                        Reader reader(value, context);

                        Reliability::FailoverUnitDescription ftDesc;
                        if (!reader.Read<Reliability::FailoverUnitDescription>(L' ', ftDesc))
                        {
                            errLogger.Log(L"FTDesc");
                            return false;
                        }

                        Reliability::ReplicaDescription replica;
                        if (!reader.Read<Reliability::ReplicaDescription>(L'[', L']', replica))
                        {
                            errLogger.Log(L"Reliability::ReplicaDescription");
                            return false;
                        }

                        Reliability::FaultType::Enum faultType;
                        if (!reader.Read<Reliability::FaultType::Enum>(L'\0', faultType))
                        {
                            errLogger.Log(L"Reliability::FaultType");
                            return false;
                        }

                        rv = ReportFaultMessageBody(ftDesc, replica, faultType, Common::ActivityDescription::Empty);
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < Reliability::FailoverUnitReplyMessageBody >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, Reliability::FailoverUnitReplyMessageBody & rv)
                    {
                        ErrorLogger errLogger(L"Reliability::FailoverUnitReplyMessageBody", value);
                        Reader reader(value, context);

                        Reliability::FailoverUnitDescription ftDesc;
                        if (!reader.Read<Reliability::FailoverUnitDescription>(L' ', ftDesc))
                        {
                            errLogger.Log(L"FTDesc");
                            return false;
                        }

                        Common::ErrorCode ec;
                        if (!reader.Read<Common::ErrorCode>(L'\0', ec))
                        {
                            errLogger.Log(L"Common::ErrorCode");
                            return false;
                        }

                        rv = Reliability::FailoverUnitReplyMessageBody(ftDesc, ec);
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < Reliability::ReplicaMessageBody >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & rawContext, Reliability::ReplicaMessageBody & rv)
                    {
                        SingleFTReadContext const & context = rawContext.GetDerived<SingleFTReadContext>();
                        ErrorLogger errLogger(L"Reliability::ReplicaMessageBody", value);
                        Reader reader(value, context);

                        Reliability::FailoverUnitDescription ftDesc;
                        if (!reader.Read<Reliability::FailoverUnitDescription>(L' ', ftDesc))
                        {
                            errLogger.Log(L"FTDesc");
                            return false;
                        }

                        Reliability::ReplicaDescription replica;
                        if (!reader.Read<Reliability::ReplicaDescription>(L'[', L']', replica))
                        {
                            errLogger.Log(L"Reliability::ReplicaDescription");
                            return false;
                        }

                        rv = Reliability::ReplicaMessageBody(ftDesc, replica, context.STInfo.SD);
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < Reliability::DeleteReplicaMessageBody >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & rawContext, Reliability::DeleteReplicaMessageBody & rv)
                    {
                        SingleFTReadContext const & context = rawContext.GetDerived<SingleFTReadContext>();
                        ErrorLogger errLogger(L"Reliability::DeleteReplicaMessageBody", value);
                        Reader reader(value, context);

                        Reliability::FailoverUnitDescription ftDesc;
                        if (!reader.Read<Reliability::FailoverUnitDescription>(L' ', ftDesc))
                        {
                            errLogger.Log(L"FTDesc");
                            return false;
                        }

                        Reliability::ReplicaDescription replica;
                        if (!reader.Read<Reliability::ReplicaDescription>(L'[', L']', replica))
                        {
                            errLogger.Log(L"Reliability::ReplicaDescription");
                            return false;
                        }

                        bool isForce = false;
                        reader.Read<bool>(L' ', isForce);

                        rv = Reliability::DeleteReplicaMessageBody(ftDesc, replica, context.STInfo.SD, isForce);
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < Reliability::RAReplicaMessageBody >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & rawContext, Reliability::RAReplicaMessageBody & rv)
                    {
                        SingleFTReadContext const & context = rawContext.GetDerived<SingleFTReadContext>();
                        ErrorLogger errLogger(L"Reliability::ReplicaMessageBody", value);
                        Reader reader(value, context);

                        Reliability::FailoverUnitDescription ftDesc;
                        if (!reader.Read<Reliability::FailoverUnitDescription>(L' ', ftDesc))
                        {
                            errLogger.Log(L"FTDesc");
                            return false;
                        }

                        Reliability::ReplicaDeactivationInfo deactivationInfo;
                        if (!ReaderImpl<Reliability::ReplicaDeactivationInfo>::ReadOptional(reader, ftDesc, deactivationInfo))
                        {
                            errLogger.Log(L"DeactivationInfo");
                            return false;
                        }

                        Reliability::ReplicaDescription replica;
                        if (!reader.Read<Reliability::ReplicaDescription>(L'[', L']', replica))
                        {
                            errLogger.Log(L"Reliability::ReplicaDescription");
                            return false;
                        }

                        rv = Reliability::RAReplicaMessageBody(ftDesc, replica, context.STInfo.SD, deactivationInfo);
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < Reliability::ReplicaReplyMessageBody >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, Reliability::ReplicaReplyMessageBody & rv)
                    {
                        ErrorLogger errLogger(L"Reliability::ReplicaReplyMessageBody", value);
                        Reader reader(value, context);

                        Reliability::FailoverUnitDescription ftDesc;

                        if (!reader.Read<Reliability::FailoverUnitDescription>(L' ', ftDesc))
                        {
                            errLogger.Log(L"FTDesc");
                            return false;
                        }

                        Reliability::ReplicaDescription replica;
                        if (!reader.Read<Reliability::ReplicaDescription>(L'[', L']', replica))
                        {
                            errLogger.Log(L"Reliability::ReplicaDescription");
                            return false;
                        }

                        Common::ErrorCode err;
                        if (!reader.Read<Common::ErrorCode>(L' ', err))
                        {
                            errLogger.Log(L"Common::ErrorCode");
                            return false;
                        }

                        rv = Reliability::ReplicaReplyMessageBody(ftDesc, replica, err);
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < Reliability::GetLSNReplyMessageBody>
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, Reliability::GetLSNReplyMessageBody & rv)
                    {
                        ErrorLogger errLogger(L"Reliability::GetLSNReplyMessageBody", value);
                        Reader reader(value, context);

                        Reliability::FailoverUnitDescription ftDesc;
                        Reliability::ReplicaDeactivationInfo deactivationInfo;

                        if (!reader.Read<Reliability::FailoverUnitDescription>(L' ', ftDesc))
                        {
                            errLogger.Log(L"FTDesc");
                            return false;
                        }

                        if (!ReaderImpl<Reliability::ReplicaDeactivationInfo>::ReadOptional(reader, ftDesc, deactivationInfo))
                        {
                            errLogger.Log(L"DeactivationInfo");
                            return false;
                        }

                        Reliability::ReplicaDescription replica;
                        if (!reader.Read<Reliability::ReplicaDescription>(L'[', L']', replica))
                        {
                            errLogger.Log(L"Reliability::ReplicaDescription");
                            return false;
                        }
                        
                        Common::ErrorCode err;
                        if (!reader.Read<Common::ErrorCode>(L' ', err))
                        {
                            errLogger.Log(L"Common::ErrorCode");
                            return false;
                        }

                        rv = Reliability::GetLSNReplyMessageBody(ftDesc, replica, deactivationInfo, err);
                        return true;
                    }
                };

                typedef std::pair<Reliability::FailoverUnitId, Reliability::ReplicaDescription> ReplicaListMessageBodyItem;

                template<>
                struct ReaderImpl < ReplicaListMessageBodyItem >
                {
                    // FT-Id [ReplicaDesc]
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & rawContext, ReplicaListMessageBodyItem & rv)
                    {
                        MultipleReadContextContainer const & context = rawContext.GetDerived<MultipleReadContextContainer>();
                        ErrorLogger errLogger(L"ReplicaListMessageBodyItem", value);
                        Reader reader(value, context);

                        std::wstring shortName = reader.ReadString(L' ');
                        SingleFTReadContext const * singleFTContext = context.GetSingleFTContext(shortName);
                        if (singleFTContext == nullptr)
                        {
                            errLogger.Log(L"FTShortName");
                            return false;
                        }

                        Reliability::ReplicaDescription description;
                        reader.SetReadContext(*singleFTContext);
                        if (!reader.Read<Reliability::ReplicaDescription>(L'[', L']', description))
                        {
                            errLogger.Log(L"ReplciaDescription");
                            return false;
                        }

                        rv = std::make_pair(singleFTContext->FUID, description);
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < Reliability::ReplicaListMessageBody >
                {
                    // {FT-Id [Replica Desc]} {FT-Id [Replica Desc]} {FT-Id [Replica Desc]} 
                    static bool Read(std::wstring const & value, ReadOption::Enum option, ReadContext const & context, Reliability::ReplicaListMessageBody & rv)
                    {
                        ErrorLogger errLogger(L"Reliability::ReplicaListMessageBody", value);
                        Reader reader(value, context);

                        std::vector<ReplicaListMessageBodyItem> v;
                        if (!reader.ReadVector<ReplicaListMessageBodyItem>(L'{', L'}', option, v))
                        {
                            errLogger.Log(L"ReplicaListMessageBodyItem");
                            return false;
                        }

                        std::map<Reliability::FailoverUnitId, Reliability::ReplicaDescription> m;
                        for (auto it = v.begin(); it != v.end(); ++it)
                        {
                            m[it->first] = it->second;
                        }

                        rv = Reliability::ReplicaListMessageBody(std::move(m));
                        return true;
                    }
                };

                // pair so that we have something to read over here
                typedef std::pair<bool, Reliability::FailoverUnitInfo> FTInfoBodyItem;

                template<>
                struct ReaderImpl < FTInfoBodyItem >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & rawContext, FTInfoBodyItem & rv)
                    {
                        MultipleReadContextContainer const & multipleFTContext = rawContext.GetDerived<MultipleReadContextContainer>();
                        ErrorLogger errLogger(L"FTBodyItem", value);
                        Reader reader(value, multipleFTContext);

                        std::wstring shortName = reader.ReadString(L' ');
                        SingleFTReadContext const * thisFTContext = multipleFTContext.GetSingleFTContext(shortName);
                        if (thisFTContext == nullptr)
                        {
                            errLogger.Log(L"ShortName");
                            return false;
                        }

                        Reliability::FailoverUnitInfo ftInfo;
                        reader.SetReadContext(*thisFTContext);
                        if (!reader.Read<Reliability::FailoverUnitInfo>(L'\0', ftInfo))
                        {
                            errLogger.Log(L"FTInfo");
                            return false;
                        }

                        rv = std::make_pair(true, ftInfo);
                        return true;
                    }

                    static bool ReadFTInfo(Reader & reader, ErrorLogger & errLogger, std::vector<Reliability::FailoverUnitInfo> & rv)
                    {
                        std::vector<FTInfoBodyItem> v;
                        if (!reader.ReadVector<FTInfoBodyItem>(L'{', L'}', v))
                        {
                            errLogger.Log(L"FailoverUnitInfoList");
                            return false;
                        }

                        for (auto it = v.begin(); it != v.end(); ++it)
                        {
                            rv.push_back(it->second);
                        }

                        return true;
                    }
                };

                template<>
                struct ReaderImpl < Reliability::ReplicaUpMessageBody >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, Reliability::ReplicaUpMessageBody & rv)
                    {
                        ErrorLogger errLogger(L"Reliability::ReplicaUpMessageBody", value);
                        Reader reader(value, context);
                        bool isLast, isFromFMM;

                        if (!reader.Read<bool>(L' ', isLast))
                        {
                            errLogger.Log(L"IsLast");
                            return false;
                        }

                        if (!reader.Read<bool>(L' ', isFromFMM))
                        {
                            errLogger.Log(L"IsFromFMM");
                            return false;
                        }

                        std::vector<Reliability::FailoverUnitInfo> replicaList;
                        if (!ReaderImpl<FTInfoBodyItem>::ReadFTInfo(reader, errLogger, replicaList))
                        {
                            errLogger.Log(L"FailoverUnitInfoList");
                            return false;
                        }

                        if (reader.IsEOF)
                        {
                            errLogger.Log(L"Delimiter between two replica up lists not found");
                            return false;
                        }

                        // skip delimiter between lists
                        reader.ReadString(L'|');

                        std::vector<Reliability::FailoverUnitInfo> droppedReplicas;
                        if (!ReaderImpl<FTInfoBodyItem>::ReadFTInfo(reader, errLogger, droppedReplicas))
                        {
                            errLogger.Log(L"FailoverUnitInfoList");
                            return false;
                        }

                        rv = Reliability::ReplicaUpMessageBody(
                            std::move(replicaList),
                            std::move(droppedReplicas),
                            isLast,
                            isFromFMM);
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < Reliability::NodeUpgradeReplyMessageBody >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, Reliability::NodeUpgradeReplyMessageBody  & rv)
                    {
                        ErrorLogger errLogger(L"Reliability::NodeUpgradeReplyMessageBody", value);
                        Reader reader(value, context);

                        int instanceId;
                        if (!reader.Read<int>(L' ', instanceId))
                        {
                            errLogger.Log(L"InstanceId");
                            return false;
                        }

                        std::vector<Reliability::FailoverUnitInfo> replicaList;
                        if (!ReaderImpl<FTInfoBodyItem>::ReadFTInfo(reader, errLogger, replicaList))
                        {
                            errLogger.Log(L"FailoverUnitInfoList");
                            return false;
                        }

                        // skip delimiter between lists
                        reader.ReadString(L'|');

                        std::vector<Reliability::FailoverUnitInfo> droppedReplicas;
                        if (!ReaderImpl<FTInfoBodyItem>::ReadFTInfo(reader, errLogger, droppedReplicas))
                        {
                            errLogger.Log(L"FailoverUnitInfoList");
                            return false;
                        }

                        ServiceModel::ApplicationUpgradeSpecification spec(
                            Default::GetInstance().ApplicationIdentifier,
                            Default::GetInstance().ApplicationVersion,
                            Default::GetInstance().ApplicationName,
                            instanceId,
                            ServiceModel::UpgradeType::Rolling,
                            false,
                            false,
                            std::vector<ServiceModel::ServicePackageUpgradeSpecification>(),
                            std::vector<ServiceModel::ServiceTypeRemovalSpecification>());

                        rv = Reliability::NodeUpgradeReplyMessageBody(spec, std::move(replicaList), std::move(droppedReplicas));
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < Reliability::NodeUpAckMessageBody >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, Reliability::NodeUpAckMessageBody & rv)
                    {
                        Reader reader(value, context);
                        ErrorLogger errLogger(L"Reliability::NodeUpAckMessageBody", value);
                        bool isActivated = false;

                        if (!reader.Read(L' ', isActivated))
                        {
                            errLogger.Log(L"IsNodeActivated");
                            return false;
                        }

                        uint64 activationSequenceNumber = 0;
                        if (!reader.Read(L' ', activationSequenceNumber))
                        {
                            errLogger.Log(L"ActivationSequenceNumber");
                            return false;
                        }

                        auto versionInstance = Default::GetInstance().NodeVersionInstance;
                        if (!reader.IsEOF)
                        {
                            if (!reader.Read(L' ', versionInstance))
                            {
                                errLogger.Log(L"nodeVersionInstance");
                                return false;
                            }
                        }

                        rv = Reliability::NodeUpAckMessageBody(std::move(versionInstance), std::vector<Reliability::UpgradeDescription>(), isActivated, activationSequenceNumber);
                        return true;
                    }
                };

                template<typename T>
                inline bool ReadActivateDeactivateNodeRequestObject(std::wstring const & value, std::wstring const & name, ReadContext const & context, T & rv)
                {
                    Reader reader(value, context);
                    ErrorLogger errLogger(name, value);

                    int64 sequenceNumber;
                    if (!reader.Read(L' ', sequenceNumber))
                    {
                        errLogger.Log(L"SequenceNumber");
                        return false;
                    }

                    bool isFmm;
                    if (!reader.Read(L' ', isFmm))
                    {
                        errLogger.Log(L"IsFmm");
                        return false;
                    }

                    rv = T(sequenceNumber, isFmm);
                    return true;
                }

                template<typename T>
                inline bool ReadActivateDeactivateReplyObject(std::wstring const & value, std::wstring const & name, ReadContext const & context, T & rv)
                {
                    Reader reader(value, context);
                    ErrorLogger errLogger(name, value);

                    int64 sequenceNumber;
                    if (!reader.Read(L' ', sequenceNumber))
                    {
                        errLogger.Log(L"SequenceNumber");
                        return false;
                    }

                    rv = T(sequenceNumber);
                    return true;
                }

                template<>
                struct ReaderImpl < Reliability::NodeActivateRequestMessageBody >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, Reliability::NodeActivateRequestMessageBody & rv)
                    {
                        return ReadActivateDeactivateNodeRequestObject(value, L"Reliability::NodeActivateRequestMessageBody", context, rv);
                    }
                };

                template<>
                struct ReaderImpl < Reliability::NodeDeactivateRequestMessageBody >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, Reliability::NodeDeactivateRequestMessageBody & rv)
                    {
                        return ReadActivateDeactivateNodeRequestObject(value, L"Reliability::NodeDeactivateRequestMessageBody", context, rv);
                    }
                };

                template<>
                struct ReaderImpl < Reliability::NodeActivateReplyMessageBody >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, Reliability::NodeActivateReplyMessageBody & rv)
                    {
                        return ReadActivateDeactivateReplyObject(value, L"Reliability::NodeActivateReplyMessageBody", context, rv);
                    }
                };

                template<>
                struct ReaderImpl < Reliability::NodeDeactivateReplyMessageBody >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, Reliability::NodeDeactivateReplyMessageBody & rv)
                    {
                        return ReadActivateDeactivateReplyObject(value, L"Reliability::NodeDeactivateReplyMessageBody", context, rv);
                    }
                };

                template<>
                struct ReaderImpl < Reliability::GenerationUpdateMessageBody >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, Reliability::GenerationUpdateMessageBody&rv)
                    {
                        ErrorLogger errLogger(L"Reliability::GenerationUpdateMessageBody", value);
                        Reader reader(value, context);

                        Reliability::Epoch fmServiceEpoch;
                        if (!reader.Read(L' ', fmServiceEpoch))
                        {
                            errLogger.Log(L"FMServiceEpoch");
                            return false;
                        }

                        rv = Reliability::GenerationUpdateMessageBody(fmServiceEpoch);
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < Reliability::GenerationProposalReplyMessageBody >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, Reliability::GenerationProposalReplyMessageBody & rv)
                    {
                        ErrorLogger errLogger(L"Reliability::GenerationProposalReplyMessageBody", value);
                        Reader reader(value, context);

                        Reliability::GenerationNumber generation, proposedGeneration;

                        if (!reader.Read(L' ', generation))
                        {
                            errLogger.Log(L"message Generation");
                            return false;
                        }

                        if (!reader.Read(L' ', proposedGeneration))
                        {
                            errLogger.Log(L"proposed Generation on node");
                            return false;
                        }

                        rv = Reliability::GenerationProposalReplyMessageBody(generation, proposedGeneration);
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < ProxyRequestMessageBody >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, ProxyRequestMessageBody & rv)
                    {
                        ErrorLogger errLogger(L"ProxyRequestMessageBody", value);
                        Reader reader(value, context);

                        Reliability::FailoverUnitDescription ftDesc;
                        if (!reader.Read<Reliability::FailoverUnitDescription>(L' ', ftDesc))
                        {
                            errLogger.Log(L"FTDesc");
                            return false;
                        }

                        Reliability::ReplicaDescription localReplica;
                        if (!reader.Read<Reliability::ReplicaDescription>(L'[', L']', localReplica))
                        {
                            errLogger.Log(L"LocalReplica");
                            return false;
                        }

                        std::vector<Reliability::ReplicaDescription> remoteReplicas;
                        if (!reader.IsEOF)
                        {
                            if (!reader.ReadVector<Reliability::ReplicaDescription>(L'[', L']', remoteReplicas))
                            {
                                errLogger.Log(L"RemoteReplicas");
                                return false;
                            }
                        }

                        ProxyMessageFlags::Enum flags = ProxyMessageFlags::None;
                        if (!reader.IsEOF)
                        {
                            if (!reader.Read<ProxyMessageFlags::Enum>(L' ', flags))
                            {
                                errLogger.Log(L"Flags");
                                return false;
                            }
                        }

                        bool hasSD = false;
                        if (!reader.IsEOF && reader.PeekChar() == L's')
                        {
                            hasSD = true;
                        }

                        rv = ProxyRequestMessageBody(ftDesc, localReplica, std::move(remoteReplicas), flags);

                        if (hasSD)
                        {
                            rv.Test_SetServiceDescription(context.GetDerived<SingleFTReadContext>().STInfo.SD);
                        }

                        return true;
                    }
                };

                template<>
                struct ReaderImpl < ProxyUpdateServiceDescriptionReplyMessageBody >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, ProxyUpdateServiceDescriptionReplyMessageBody & rv)
                    {
                        ErrorLogger errLogger(L"ProxyUpdateServiceDescriptionReplyMessageBody", value);
                        Reader reader(value, context);

                        Reliability::FailoverUnitDescription ftDesc;
                        if (!reader.Read<Reliability::FailoverUnitDescription>(L' ', ftDesc))
                        {
                            errLogger.Log(L"FTDesc");
                            return false;
                        }

                        Reliability::ReplicaDescription localReplica;
                        if (!reader.Read<Reliability::ReplicaDescription>(L'[', L']', localReplica))
                        {
                            errLogger.Log(L"LocalReplica");
                            return false;
                        }

                        int updateVersion = 0;
                        if (!reader.Read(L' ', updateVersion))
                        {
                            errLogger.Log(L"UpdateVersion");
                            return false;
                        }

                        Common::ErrorCode error;
                        if (!reader.Read<Common::ErrorCode>(L' ', error))
                        {
                            errLogger.Log(L"Common::ErrorCode");
                            return false;
                        }

                        auto sd = context.GetDerived<SingleFTReadContext>().STInfo.SD;
                        sd.UpdateVersion = updateVersion;

                        rv = ProxyUpdateServiceDescriptionReplyMessageBody(std::move(error), ftDesc, localReplica, sd);
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < ProxyErrorCode >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, ProxyErrorCode & rv)
                    {
                        ErrorLogger errLogger(L"ProxyErrorCode", value);
                        Reader reader(value, context);

                        Common::ErrorCode error;
                        if (!reader.Read<Common::ErrorCode>(L' ', error))
                        {
                            errLogger.Log(L"Common::ErrorCode");
                            return false;
                        }

                        bool isRAPError = error.IsError(Common::ErrorCodeValue::RAProxyDemoteCompleted) ||
                            error.IsError(Common::ErrorCodeValue::RAProxyStateChangedOnDataLoss);

                        if (isRAPError)
                        {
                            rv = ProxyErrorCode::CreateRAPError(std::move(error));
                        }
                        else
                        {
                            Common::ApiMonitoring::ApiNameDescription name(
                                Common::ApiMonitoring::InterfaceName::IReplicator,
                                Common::ApiMonitoring::ApiName::Abort,
                                L"");

                            rv = ProxyErrorCode::CreateUserApiError(std::move(error), std::move(name));
                        }

                        return true;
                    }
                };

                template<>
                struct ReaderImpl < ProxyReplyMessageBody >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, ProxyReplyMessageBody & rv)
                    {
                        ErrorLogger errLogger(L"ProxyReplyMessageBody", value);
                        Reader reader(value, context);

                        Reliability::FailoverUnitDescription ftDesc;
                        if (!reader.Read<Reliability::FailoverUnitDescription>(L' ', ftDesc))
                        {
                            errLogger.Log(L"FTDesc");
                            return false;
                        }

                        Reliability::ReplicaDescription localReplica;
                        if (!reader.Read<Reliability::ReplicaDescription>(L'[', L']', localReplica))
                        {
                            errLogger.Log(L"LocalReplica");
                            return false;
                        }

                        std::vector<Reliability::ReplicaDescription> remote;
                        if (!reader.ReadVector<Reliability::ReplicaDescription>(L'[', L']', remote))
                        {
                            errLogger.Log(L"Remote Replica");
                            return false;
                        }

                        ProxyErrorCode error;
                        if (!reader.Read<ProxyErrorCode>(L' ', error))
                        {
                            errLogger.Log(L"ProxyErrorCode");
                            return false;
                        }

                        ProxyMessageFlags::Enum flags = ProxyMessageFlags::None;
                        if (!reader.IsEOF)
                        {
                            if (!reader.Read<ProxyMessageFlags::Enum>(L'\0', flags))
                            {
                                errLogger.Log(L"Flags");
                                return false;
                            }
                        }

                        rv = ProxyReplyMessageBody(ftDesc, localReplica, std::move(remote), std::move(error), flags);
                        return true;
                    }
                };

                template<typename T>
                struct ReaderImpl < ProxyQueryReplyMessageBody<T> >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, ProxyQueryReplyMessageBody<T> & rv)
                    {
                        ErrorLogger errLogger(L"ProxyQueryReplyMessageBody", value);
                        Reader reader(value, context);

                        Reliability::FailoverUnitDescription ftDesc;
                        if (!reader.Read<Reliability::FailoverUnitDescription>(L' ', ftDesc))
                        {
                            errLogger.Log(L"FTDesc");
                            return false;
                        }

                        Reliability::ReplicaDescription localReplica;
                        if (!reader.Read<Reliability::ReplicaDescription>(L'[', L']', localReplica))
                        {
                            errLogger.Log(L"LocalReplica");
                            return false;
                        }

                        Common::ErrorCode error;
                        if (!reader.Read<Common::ErrorCode>(L' ', error))
                        {
                            errLogger.Log(L"Common::ErrorCode");
                            return false;
                        }

                        int timeStamp = 0;
                        if (!reader.Read(L' ', timeStamp))
                        {
                            errLogger.Log(L"Timestamp");
                            return false;
                        }

                        T input;
                        if (!reader.Read<T>(L'[', L']', input))
                        {
                            errLogger.Log(L"Input");
                            return false;
                        }

                        rv = ProxyQueryReplyMessageBody<T>(
                            std::move(error),
                            ftDesc,
                            localReplica,
                            Common::StopwatchTime(timeStamp),
                            std::move(input));
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < ServiceModel::FabricUpgradeSpecification >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, ServiceModel::FabricUpgradeSpecification & rv)
                    {
                        ErrorLogger errLogger(L"FabricUpgradeSpecification", value);

                        Reader reader(value, context);

                        Common::FabricVersionInstance version;
                        if (!reader.Read<Common::FabricVersionInstance>(L' ', version))
                        {
                            errLogger.Log(L"Instance");
                            return false;
                        }

                        ServiceModel::UpgradeType::Enum upgradeType;
                        if (!reader.Read(L' ', upgradeType))
                        {
                            errLogger.Log(L"UpgradeType");
                            return false;
                        }

                        rv = ServiceModel::FabricUpgradeSpecification(version.Version, version.InstanceId, upgradeType, false, false);
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < ::Query::QueryRequestMessageBodyInternal >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, ::Query::QueryRequestMessageBodyInternal & rv)
                    {
                        ErrorLogger errLogger(L"QueryRequest", value);
                        Reader reader(value, context);

                        std::wstring queryType = reader.ReadString(L' ');
                        if (queryType == L"DeployedServiceQueryRequest")
                        {
                            return ReadDeployedServiceReplicaQueryRequest(reader, errLogger, context, rv);
                        }
                        else if (queryType == L"DeployedServiceReplicaDetailQueryRequest")
                        {
                            return ReadDeployedServiceReplicaDetailQueryRequest(reader, errLogger, context, rv);
                        }
                        else if (queryType == L"InvalidQueryName")
                        {
                            return ReadInvalidQueryNameQueryRequest(rv);
                        }
                        else if (queryType == L"QueryTypeParallel")
                        {
                            return ReadQueryTypeMultipleRequest(rv);
                        }
                        else
                        {
                            errLogger.Log(L"Unknown query type");
                            return false;
                        }
                    }

                    static bool ReadDeployedServiceReplicaQueryRequest(Reader & reader, ErrorLogger &, ReadContext const & context, ::Query::QueryRequestMessageBodyInternal & rv)
                    {
                        ServiceModel::QueryArgumentMap m;

                        std::wstring appName;
                        if (!reader.IsEOF)
                        {
                            appName = reader.ReadString(L' ');
                            m.Insert(::Query::QueryResourceProperties::Application::ApplicationName, appName);
                        }

                        std::wstring packageName;
                        if (!reader.IsEOF)
                        {
                            packageName = reader.ReadString(L' ');
                            m.Insert(::Query::QueryResourceProperties::ServiceType::ServiceManifestName, packageName);
                        }

                        std::wstring partitionId;
                        if (!reader.IsEOF)
                        {
                            partitionId = GetPartitionId(reader.ReadString(L' '), context);
                            m.Insert(::Query::QueryResourceProperties::Partition::PartitionId, partitionId);
                        }

                        rv = ::Query::QueryRequestMessageBodyInternal(::Query::QueryNames::GetServiceReplicaListDeployedOnNode, ::Query::QueryType::Simple, m);

                        return true;
                    }

                    static bool ReadDeployedServiceReplicaDetailQueryRequest(Reader & reader, ErrorLogger &, ReadContext const & context, ::Query::QueryRequestMessageBodyInternal & rv)
                    {
                        std::wstring partitionId = GetPartitionId(reader.ReadString(L' '), context);

                        std::wstring replicaId = reader.ReadString(L' ');

                        ServiceModel::QueryArgumentMap m;
                        m.Insert(::Query::QueryResourceProperties::Partition::PartitionId, partitionId);
                        m.Insert(::Query::QueryResourceProperties::Replica::ReplicaId, replicaId);

                        rv = ::Query::QueryRequestMessageBodyInternal(::Query::QueryNames::GetDeployedServiceReplicaDetail, ::Query::QueryType::Simple, m);

                        return true;
                    }

                    static bool ReadInvalidQueryNameQueryRequest(::Query::QueryRequestMessageBodyInternal & rv)
                    {
                        ServiceModel::QueryArgumentMap m;

                        rv = ::Query::QueryRequestMessageBodyInternal(::Query::QueryNames::GetQueries, ::Query::QueryType::Simple, m);

                        return true;
                    }

                    static bool ReadQueryTypeMultipleRequest(::Query::QueryRequestMessageBodyInternal & rv)
                    {
                        ServiceModel::QueryArgumentMap m;

                        rv = ::Query::QueryRequestMessageBodyInternal(::Query::QueryNames::GetServiceReplicaListDeployedOnNode, ::Query::QueryType::Parallel, m);

                        return true;
                    }

                    static std::wstring GetPartitionId(std::wstring const & value, ReadContext const & context)
                    {
                        // The partition id can be represented either as a ft short name (SL1 etc etc)
                        // Or just a value [for testing scenarios where the partition id parsing fails etc]
                        MultipleReadContextContainer const & casted = context.GetDerived<MultipleReadContextContainer>();
                        auto ftContext = casted.GetSingleFTContext(value);

                        if (ftContext == nullptr)
                        {
                            return value;
                        }
                        else
                        {
                            return Common::wformatString(ftContext->FUID);
                        }
                    }
                };

                template<>
                struct ReaderImpl < ServiceModel::DeployedServiceReplicaDetailQueryResult >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, ServiceModel::DeployedServiceReplicaDetailQueryResult & rv)
                    {
                        Reader reader(value, context);
                        ErrorLogger errLogger(L"DeployedServiceReplicaDetailQueryResult", value);

                        rv = ServiceModel::DeployedServiceReplicaDetailQueryResult(
                            L"fabric:/a",
                            Common::Guid::Empty(),
                            1,
                            FABRIC_QUERY_SERVICE_OPERATION_NAME_ABORT,
                            Common::DateTime::Zero,
                            std::vector<ServiceModel::LoadMetricReport>());

                        return true;
                    }
                };

                template<>
                struct ReaderImpl < ServiceModel::DeployedServiceReplicaQueryResult >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, ServiceModel::DeployedServiceReplicaQueryResult & rv)
                    {
                        MultipleReadContextContainer const & multipleFTContext = context.GetDerived<MultipleReadContextContainer>();

                        ErrorLogger errLogger(L"DeployedServiceReplicaQueryResult", value);
                        Reader reader(value, context);

                        std::wstring shortName = reader.ReadString(L' ');
                        auto singleFTContext = multipleFTContext.GetSingleFTContext(shortName);
                        if (!singleFTContext)
                        {
                            errLogger.Log(L"FTShortName");
                            return false;
                        }

                        reader.SetReadContext(*singleFTContext);

                        bool isStateful = true;
                        if (!reader.Read(L' ', isStateful))
                        {
                            errLogger.Log(L"IsStateful");
                            return false;
                        }

                        int replicaId = 0;
                        if (!reader.Read(L' ', replicaId))
                        {
                            errLogger.Log(L"ReplicaId");
                            return false;
                        }

                        Reliability::ReplicaRole::Enum role;
                        if (!reader.Read(L' ', role))
                        {
                            errLogger.Log(L"Role");
                            return false;
                        }

                        auto previousConfigRole = ReplicaRole::None;
                        if(role == ReplicaRole::Unknown)
                        {
                            previousConfigRole = ReplicaRole::Unknown;
                        }

                        FABRIC_QUERY_SERVICE_REPLICA_STATUS status;
                        if (!reader.Read(L' ', status))
                        {
                            errLogger.Log(L"Status");
                            return false;
                        }

                        auto servicePackageActivationIdSPtr = std::make_shared<std::wstring>(L"");
                        int hostProcessId = 0;
                        wstring codePackageName = L"";
                        FABRIC_RECONFIGURATION_PHASE reconfigPhase = FABRIC_RECONFIGURATION_PHASE_INVALID;
                        FABRIC_RECONFIGURATION_TYPE reconfigType = FABRIC_RECONFIGURATION_TYPE_INVALID;
                        auto entityEntries = singleFTContext->RA->LocalFailoverUnitMapObj.GetAllFailoverUnitEntries(false);

                        for (auto entityEntry : entityEntries)
                        {
                            auto failoverUnitEntry = static_pointer_cast<Infrastructure::EntityEntry<FailoverUnit>>(entityEntry);
                            {
                                auto ft = failoverUnitEntry->CreateReadLock();

                                if (ft->FailoverUnitId.Guid == singleFTContext->FUID.Guid)
                                {
                                    if (ft->ServiceTypeRegistration != nullptr)
                                    {
                                        hostProcessId = ft->ServiceTypeRegistration->HostProcessId;
                                        codePackageName = singleFTContext->STInfo.CodePackageName;
                                        if (singleFTContext->STInfo.SD.PackageActivationMode == ServiceModel::ServicePackageActivationMode::ExclusiveProcess)
                                        {
                                            servicePackageActivationIdSPtr = std::make_shared<std::wstring>(Common::Guid::NewGuid().ToString());
                                        }
                                    }

                                    reconfigPhase = FailoverUnitReconfigurationStage::ConvertToPublicReconfigurationPhase(ft->ReconfigurationStage);
                                    if(ft->ReconfigurationStage == FailoverUnitReconfigurationStage::None)
                                    {
                                        reconfigType = ReconfigurationType::ConvertToPublicReconfigurationType(ReconfigurationType::None);
                                    }
                                    else
                                    {
                                        reconfigType = ReconfigurationType::ConvertToPublicReconfigurationType(ft->ReconfigurationType);
                                    }
                                }
                            }
                        }

                        if (isStateful)
                        {
                            rv = ServiceModel::DeployedServiceReplicaQueryResult(
                                singleFTContext->STInfo.SD.Name,
                                singleFTContext->STInfo.ServiceTypeId.ServiceTypeName,
                                singleFTContext->STInfo.SD.Type.ServicePackageId.ServicePackageName,
                                servicePackageActivationIdSPtr,
                                codePackageName,
                                singleFTContext->FUID.Guid,
                                replicaId,
                                Reliability::ReplicaRole::ConvertToPublicReplicaRole(role),
                                status,
                                L"",
                                hostProcessId,
                                ServiceModel::ReconfigurationInformation(
                                    Reliability::ReplicaRole::ConvertToPublicReplicaRole(previousConfigRole),
                                    reconfigPhase,
                                    reconfigType,
									DateTime::Zero));
                        }
                        else
                        {
                            rv = ServiceModel::DeployedServiceReplicaQueryResult(
                                singleFTContext->STInfo.SD.Name,
                                singleFTContext->STInfo.ServiceTypeId.ServiceTypeName,
                                singleFTContext->STInfo.SD.Type.ServicePackageId.ServicePackageName,
                                servicePackageActivationIdSPtr,
                                singleFTContext->STInfo.CodePackageName,
                                singleFTContext->FUID.Guid,
                                replicaId,
                                status,
                                L"",
                                hostProcessId);
                        }

                        return true;
                    }
                };

                template<>
                struct ReaderImpl < ServiceModel::QueryResult >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, ServiceModel::QueryResult & rv)
                    {
                        MultipleReadContextContainer const & readContext = context.GetDerived<MultipleReadContextContainer>();

                        ErrorLogger errLogger(L"QueryReply", value);
                        Reader reader(value, readContext);

                        // Read error
                        Common::ErrorCode err;
                        if (!reader.Read(L' ', err))
                        {
                            errLogger.Log(L"Common::ErrorCode");
                            return false;
                        }

                        if (!err.IsSuccess())
                        {
                            rv = ServiceModel::QueryResult(err);
                            return true;
                        }

                        std::wstring resultType = reader.ReadString(L' ');
                        if (resultType == L"DeployedServiceQueryResult")
                        {
                            std::vector<ServiceModel::DeployedServiceReplicaQueryResult> items;
                            if (!reader.ReadVector(L'[', L']', items))
                            {
                                errLogger.Log(L"Items");
                                return false;
                            }

                            rv = ServiceModel::QueryResult(std::move(items));
                            return true;
                        }
                        else if (resultType == L"DeployedServiceReplicaDetailQueryResult")
                        {
                            ServiceModel::DeployedServiceReplicaDetailQueryResult result;
                            if (!reader.Read(L'[', L']', result))
                            {
                                errLogger.Log(L"Result");
                                return false;
                            }

                            rv = ServiceModel::QueryResult(std::move(Common::make_unique<ServiceModel::DeployedServiceReplicaDetailQueryResult>(std::move(result))));
                            return true;
                        }
                        else
                        {
                            errLogger.Log(Common::wformatString("Result Type: {0}", resultType));
                            return false;
                        }
                    };
                };

                template<>
                struct ReaderImpl < ServiceModel::HealthReport >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & rawContext, ServiceModel::HealthReport & rv)
                    {
                        auto const & readContext = rawContext.GetDerived<SingleFTReadContext>();

                        ErrorLogger errLogger(L"HealthReport", value);
                        Reader reader(value, readContext);
                        ReplicaAndNodeId replicaAndNodeId;
                        Common::SystemHealthReportCode::Enum code;
                        std::wstring description;
                        std::wstring prop;
                        bool isDelete = false;

                        // Read the type
                        auto type = reader.ReadString(L' ');
                        if (type == L"delete")
                        {
                            isDelete = true;
                        }

                        if (!reader.Read(L' ', code))
                        {
                            errLogger.Log(L"Delete: code");
                            return false;
                        }

                        if (!reader.Read(L' ', replicaAndNodeId))
                        {
                            errLogger.Log(L"Delete: Replica and node id");
                            return false;
                        }

                        if (!isDelete)
                        {
                            prop = reader.ReadString(L' ');
                            description = reader.ReadString(L'\0');
                        }

                        auto entity = CreateEntity(readContext, replicaAndNodeId);
                        auto attributes = CreateAttributes(replicaAndNodeId);

                        if (isDelete)
                        {
                            rv = ServiceModel::HealthReport::CreateSystemDeleteEntityHealthReport(
                                code,
                                std::move(entity));

                            return true;
                        }
                        else
                        {
                            rv = ServiceModel::HealthReport::CreateSystemHealthReport(
                                code,
                                std::move(entity),
                                prop,
                                description,
                                std::move(attributes));

                            // Override the description so that the behavior of not concatenating
                            // automatically from the resource strings is testable 
                            // the idea is that CIT can set the entire description enabling testing of resources
                            rv.Description = description;

                            return true;
                        }
                    }

                private:
                    static ServiceModel::AttributeList CreateAttributes(ReplicaAndNodeId const & replicaAndNodeId)
                    {
                        ServiceModel::AttributeList rv;
                        rv.AddAttribute(*ServiceModel::HealthAttributeNames::NodeId, Common::wformatString(replicaAndNodeId.NodeId));
                        rv.AddAttribute(*ServiceModel::HealthAttributeNames::NodeInstanceId, Common::wformatString(replicaAndNodeId.NodeInstanceId));
                        return rv;
                    }

                    static ServiceModel::EntityHealthInformationSPtr CreateEntity(SingleFTReadContext const & readContext, ReplicaAndNodeId const & replicaAndNodeId)
                    {
                        if (readContext.STInfo.SD.IsStateful)
                        {
                            return ServiceModel::EntityHealthInformation::CreateStatefulReplicaEntityHealthInformation(
                                readContext.FUID.Guid,
                                replicaAndNodeId.ReplicaId,
                                replicaAndNodeId.ReplicaInstanceId);
                        }
                        else
                        {
                            return ServiceModel::EntityHealthInformation::CreateStatelessInstanceEntityHealthInformation(
                                readContext.FUID.Guid,
                                replicaAndNodeId.ReplicaId);
                        }
                    }
                };

                template<>
                struct ReaderImpl < Reliability::NodeUpdateServiceRequestMessageBody >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, Reliability::NodeUpdateServiceRequestMessageBody & rv)
                    {
                        MultipleReadContextContainer const & readContext = context.GetDerived<MultipleReadContextContainer>();

                        ErrorLogger errLogger(L"Reliability::NodeUpdateServiceRequestMessageBody", value);
                        Reader reader(value, readContext);

                        // get teh sd read context
                        std::wstring id = reader.ReadString(L' ');
                        auto sdReadContext = readContext.GetSingleSDContext(id);
                        if (sdReadContext == nullptr)
                        {
                            errLogger.Log(L"ShortName");
                            return false;
                        }

                        uint64 updateVersion = 0;
                        if (!reader.Read(L' ', updateVersion))
                        {
                            errLogger.Log(L"UpdateVersion");
                            return false;
                        }

                        uint64 instance = 0;
                        if (!reader.Read(L'\0', instance))
                        {
                            errLogger.Log(L"Instance");
                            return false;
                        }

                        Reliability::ServiceDescription sd = sdReadContext->SD;
                        sd.UpdateVersion = updateVersion;
                        sd.Instance = instance;

                        rv = Reliability::NodeUpdateServiceRequestMessageBody(sd);
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < Reliability::NodeUpdateServiceReplyMessageBody >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, Reliability::NodeUpdateServiceReplyMessageBody & rv)
                    {
                        MultipleReadContextContainer const & readContext = context.GetDerived<MultipleReadContextContainer>();

                        ErrorLogger errLogger(L"Reliability::NodeUpdateServiceReplyMessageBody", value);
                        Reader reader(value, readContext);

                        // get teh sd read context
                        std::wstring id = reader.ReadString(L' ');
                        auto sdReadContext = readContext.GetSingleSDContext(id);
                        if (sdReadContext == nullptr)
                        {
                            errLogger.Log(L"ShortName");
                            return false;
                        }

                        uint64 updateVersion = 0;
                        if (!reader.Read(L' ', updateVersion))
                        {
                            errLogger.Log(L"UpdateVersion");
                            return false;
                        }

                        uint64 instance = 0;
                        if (!reader.Read(L'\0', instance))
                        {
                            errLogger.Log(L"Instance");
                            return false;
                        }

                        rv = Reliability::NodeUpdateServiceReplyMessageBody(sdReadContext->SD.Name, instance, updateVersion);
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < Reliability::ReportFaultRequestMessageBody >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, Reliability::ReportFaultRequestMessageBody & rv)
                    {
                        ErrorLogger errLogger(L"Reliability::ReportFaultRequestMessageBody", value);
                        Reader reader(value, context);

                        std::wstring nodeName;
                        nodeName = reader.ReadString(L' ');

                        std::wstring ftShortName;
                        ftShortName = reader.ReadString(L' ');

                        auto ftContext = context.GetDerived<MultipleReadContextContainer>().GetSingleFTContext(ftShortName);
                        if (ftContext == nullptr)
                        {
                            errLogger.Log(L"FT Context");
                            return false;
                        }

                        Common::Guid partitionId = ftContext->FUID.Guid;

                        int64 replicaId;
                        if (!reader.Read(L' ', replicaId))
                        {
                            errLogger.Log(L"ReplciaId");
                            return false;
                        }

                        Reliability::FaultType::Enum faultType;
                        if (!reader.Read(L' ', faultType))
                        {
                            errLogger.Log(L"Reliability::FaultType");
                            return false;
                        }

                        bool isForce = false;
                        if (!reader.IsEOF)
                        {
                            if (!reader.Read(L' ', isForce))
                            {
                                errLogger.Log(L"Force");
                                return false;
                            }
                        }

                        rv = Reliability::ReportFaultRequestMessageBody(std::move(nodeName), faultType, replicaId, partitionId, isForce, Common::ActivityDescription::Empty);
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < Reliability::ReportFaultReplyMessageBody >
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, Reliability::ReportFaultReplyMessageBody & rv)
                    {
                        ErrorLogger errLogger(L"Reliability::ReportFaultReplyMessageBody", value);
                        Reader reader(value, context);

                        Common::ErrorCode error;
                        if (!reader.Read(L' ', error))
                        {
                            errLogger.Log(L"Error");
                            return false;
                        }

                        std::wstring errorMessage = reader.ReadString(L'\0');

                        rv = Reliability::ReportFaultReplyMessageBody(error, errorMessage);
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < Replica>
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum , ReadContext const & context, Replica & rv)
                    {
                        ErrorLogger errLogger(L"Replica", value);
                        Reader reader(value, context);

                        Reliability::ReplicaRole::Enum pcRole = ReplicaRole::None;
                        if (!reader.Read<Reliability::ReplicaRole::Enum>(L'/', pcRole))
                        {
                            errLogger.Log(L"pcRole");
                            return false;
                        }

                        wchar_t nextDelimiter = L'\0';
                        if (!reader.PeekNextDelimiter({L' ', L'/'}, nextDelimiter))
                        {
                            errLogger.Log(L"Unknown next deliminter between PC and next role");
                            return false;
                        }

                        if (nextDelimiter == L'/')
                        {
                            return ReadLongFormat(reader, errLogger, context, pcRole, rv);
                        }
                        else
                        {
                            return ReadShortFormat(reader, errLogger, context, pcRole, rv);
                        }
                    }

                private:
                    static bool ReadShortFormat(Reader & reader, ErrorLogger & errLogger, ReadContext const & , ReplicaRole::Enum pcRole, Replica & rv)
                    {
                        ReplicaAndNodeId replicaId;
                        ReplicaRole::Enum ccRole = ReplicaRole::None;
                        if (!reader.Read(' ', ccRole))
                        {
                            errLogger.Log(L"CCRole");
                            return false;
                        }

                        if (!reader.Read(' ', replicaId))
                        {
                            errLogger.Log(L"id");
                            return false;
                        }

                        rv = Replica(replicaId.CreateNodeInstance(), replicaId.ReplicaId, replicaId.ReplicaInstanceId);
                        rv.PreviousConfigurationRole = pcRole;
                        rv.CurrentConfigurationRole = ccRole;
                        return true;
                    }

                    static bool ReadLongFormat(Reader & reader, ErrorLogger & errLogger, ReadContext const & context, ReplicaRole::Enum pcRole, Replica & rv)
                    {
                        auto const & readContext = context.GetDerived<SingleFTReadContext>();

                        Reliability::ReplicaRole::Enum icRole, ccRole;
                        if (!reader.Read<Reliability::ReplicaRole::Enum>(L'/', icRole))
                        {
                            errLogger.Log(L"icRole");
                            return false;
                        }

                        if (!reader.Read<Reliability::ReplicaRole::Enum>(L' ', ccRole))
                        {
                            errLogger.Log(L"ccRole");
                            return false;
                        }

                        ReplicaStates::Enum state;
                        if (!reader.Read<ReplicaStates::Enum>(L' ', state))
                        {
                            errLogger.Log(L"State");
                            return false;
                        }

                        bool isUp = reader.ReadString(L' ') == L"U";

                        ReplicaMessageStage::Enum messageStage;
                        if (!reader.Read<ReplicaMessageStage::Enum>(L' ', messageStage))
                        {
                            errLogger.Log(L"MessageStage");
                            return false;
                        }

                        bool isToBeDeactivated = false;
                        bool isToBeActivated = false;
                        bool isReplicatorRemovePending = false;
                        bool isToBeRestarted = false;

                        std::wstring pendingChar = reader.ReadString(L' ');

                        if (pendingChar == L"T")
                        {
                            isToBeDeactivated = true;
                        }
                        else if (pendingChar == L"A")
                        {
                            isToBeActivated = true;
                        }
                        else if (pendingChar == L"R")
                        {
                            isReplicatorRemovePending = true;
                        }
                        else if (pendingChar == L"B")
                        {
                            isToBeRestarted = true;
                        }

                        ReplicaAndNodeId replicaAndNodeId;
                        if (!reader.Read<ReplicaAndNodeId>(L' ', replicaAndNodeId))
                        {
                            errLogger.Log(L"ReplicaId");
                            return false;
                        }

                        int64 firstAcknowledgedLSN = -1;
                        int64 lastAcknowledgedLSN = -1;
                        Reliability::ReplicaDeactivationInfo deactivationInfo;

                        ServiceModel::ServicePackageVersionInstance spvi = readContext.STInfo.ServicePackageVersionInstance;
                        std::wstring serviceLocation = std::wstring();
                        std::wstring replicationEndpoint = std::wstring();

                        ReadLSNInformation(reader, errLogger, firstAcknowledgedLSN, lastAcknowledgedLSN, deactivationInfo);

                        if (!reader.IsEOF)
                        {
                            if (reader.PeekChar() == L'(')
                            {
                                if (!reader.Read<ServiceModel::ServicePackageVersionInstance>(L'(', L')', spvi))
                                {
                                    errLogger.Log(L"ServicePackageVersionInstance");
                                    return false;
                                }
                            }
                        }

                        if (!reader.IsEOF)
                        {
                            // service location and replication endpoints are specified as:
                            // s:<location> r:<endpoint>

                            if (reader.PeekChar() == L's')
                            {
                                reader.ReadChar();
                                reader.ReadChar();

                                serviceLocation = reader.ReadString(L' ');

                                if (reader.IsEOF)
                                {
                                    errLogger.Log(L"Replicator endpoint not specified");
                                    return false;
                                }

                                if (reader.ReadChar() != L'r')
                                {
                                    errLogger.Log(L"Invalid replicator endpoint");
                                    return false;
                                }

                                reader.ReadChar();

                                replicationEndpoint = reader.ReadString(L' ');
                            }
                        }

                        Reliability::ReplicaDescription description(replicaAndNodeId.CreateNodeInstance(), replicaAndNodeId.ReplicaId, replicaAndNodeId.ReplicaInstanceId);
                        description.PackageVersionInstance = spvi;

                        rv = Replica(description.FederationNodeInstance, description.ReplicaId, description.InstanceId);

                        rv.IntermediateConfigurationRole = icRole;
                        rv.State = state;
                        rv.IsUp = isUp;
                        rv.MessageStage = messageStage;
                        rv.ToBeDeactivated = isToBeDeactivated;
                        rv.ToBeActivated = isToBeActivated;
                        rv.ToBeRestarted = isToBeRestarted;
                        rv.ReplicatorRemovePending = isReplicatorRemovePending;
                        rv.Test_SetProgress(firstAcknowledgedLSN, lastAcknowledgedLSN, deactivationInfo);
                        rv.ServiceLocation = serviceLocation;
                        rv.ReplicationEndpoint = replicationEndpoint;
                        rv.PackageVersionInstance = spvi;
                        rv.PreviousConfigurationRole = pcRole;
                        rv.CurrentConfigurationRole = ccRole;
                        return true;
                    }

                    static void ReadLSNInformation(
                        Reader & reader,
                        ErrorLogger & errLogger,
                        __out int64 & firstAcknowledgedLSN,
                        __out int64 & lastAcknowledegedLSN,
                        __out Reliability::ReplicaDeactivationInfo & replicaDeactivationInfo)
                    {
                        firstAcknowledgedLSN = FABRIC_INVALID_SEQUENCE_NUMBER;
                        lastAcknowledegedLSN = FABRIC_INVALID_SEQUENCE_NUMBER;
                        replicaDeactivationInfo = Reliability::ReplicaDeactivationInfo();

                        if (reader.IsEOF)
                        {
                            return;
                        }

                        auto firstChar = reader.PeekChar();
                        bool isMinus = firstChar == L'-';
                        bool isDigit = iswdigit(firstChar) != 0;
                        if (!isMinus && !isDigit)
                        {
                            return;
                        }

                        if (!reader.Read<int64>(L' ', firstAcknowledgedLSN))
                        {
                            errLogger.Log(L"FirstAckLSN");
                            return;
                        }

                        if (reader.IsEOF)
                        {
                            errLogger.Log(L"Last LSN not specified");
                            return;
                        }

                        if (!reader.Read<int64>(L' ', lastAcknowledegedLSN))
                        {
                            errLogger.Log(L"LastAckLSN");
                            return;
                        }

                        if (reader.IsEOF)
                        {
                            return;
                        }

                        if (!reader.Read(L' ', replicaDeactivationInfo))
                        {
                            errLogger.Log(L"replicaDeactivationInfo");
                            return;
                        }
                    }
                };

                template<>
                struct ReaderImpl<ReplicaStoreHolder>
                {
                    static bool Read(std::wstring const & value, ReadOption::Enum, ReadContext const & context, ReplicaStoreHolder & rv)
                    {
                        ErrorLogger errLogger(L"Replica", value);
                        Reader reader(value, context);

                        std::vector<Replica> replicas;
                        if (!reader.ReadVector(L'[', L']', replicas))
                        {
                            errLogger.Log(L"Replicas");
                            return false;
                        }

                        rv = ReplicaStoreHolder(std::move(replicas));
                        return true;
                    }
                };

                template<>
                struct ReaderImpl < FailoverUnitSPtr >
                {
                    static bool Read(
                        std::wstring const & value,
                        ReadOption::Enum,
                        ReadContext const & context,
                        FailoverUnitSPtr & rv)
                    {
                        SingleFTReadContext const & readContext = context.GetDerived<SingleFTReadContext>();
                        Verify::IsTrueLogOnError(readContext.RA != nullptr, L"RA is passed in");
                        Reliability::Epoch deactivationEpoch;

                        FailoverUnitSPtr localFT = std::make_shared<FailoverUnit>(
                            readContext.RA->Config,
                            readContext.FUID,
                            Reliability::ConsistencyUnitDescription(readContext.CUID),
                            readContext.STInfo.SD);

                        FailoverUnit* failoverUnit = localFT.get();

                        ErrorLogger errLogger(L"FailoverUnit", value);
                        Reader reader(value, readContext);

                        FailoverUnitStates::Enum state;
                        if (!reader.Read(L' ', state))
                        {
                            errLogger.Log(L"FTState");
                            return false;
                        }

                        FailoverUnitReconfigurationStage::Enum reconfigStage;
                        if (!reader.Read(L' ', reconfigStage))
                        {
                            errLogger.Log(L"ReconfigStage");
                            return false;
                        }

                        Reliability::Epoch pcEpoch, icEpoch, ccEpoch;
                        if (!reader.Read(L'/', pcEpoch))
                        {
                            errLogger.Log(L"pcEpoch");
                            return false;
                        }

                        if (!reader.Read(L'/', icEpoch))
                        {
                            errLogger.Log(L"icEpoch");
                            return false;
                        }

                        if (!reader.Read(L' ', ccEpoch))
                        {
                            errLogger.Log(L"ccEpoch");
                            return false;
                        }

                        Reliability::ReplicaDeactivationInfo deactivationInfo;
                        if (state == Reliability::ReconfigurationAgentComponent::FailoverUnitStates::Closed)
                        {
                            deactivationInfo = Reliability::ReplicaDeactivationInfo::Dropped;
                        }
                        else if (!ReaderImpl<Reliability::ReplicaDeactivationInfo>::ReadOptional(reader, ccEpoch, deactivationInfo))
                        {
                            errLogger.Log(L"DeactivationInfo");
                            return false;
                        }

                        int localReplicaId, localReplicaInstanceId;
                        if (!reader.Read(L':', localReplicaId))
                        {
                            errLogger.Log(L"LocalReplicaId");
                            return false;
                        }

                        if (!reader.Read(L' ', localReplicaInstanceId))
                        {
                            errLogger.Log(L"LocalReplicaInstanceId");
                            return false;
                        }

                        failoverUnit->Test_SetState(state);

                        failoverUnit->PreviousConfigurationEpoch = pcEpoch;
                        failoverUnit->IntermediateConfigurationEpoch = icEpoch;
                        failoverUnit->CurrentConfigurationEpoch = ccEpoch;
                        failoverUnit->Test_SetDeactivationInfo(deactivationInfo);

                        failoverUnit->Test_SetLocalReplicaId(localReplicaId);
                        failoverUnit->Test_SetLocalReplicaInstanceId(localReplicaInstanceId);
                        failoverUnit->Test_SetLastStableEpoch(pcEpoch);

                        int64 downReplicaInstanceId = -1;
                        if (iswdigit(reader.PeekChar()))
                        {
                            if (!reader.Read(L' ', downReplicaInstanceId))
                            {
                                errLogger.Log(L"PreReopenLocalReplica/DownReplicaInstanceId");
                                return false;
                            }

                            failoverUnit->Test_FMMessageStateObj().Test_SetDownReplicaInstanceId(downReplicaInstanceId);
                        }

                        ServiceModel::ServicePackageVersionInstance spvi = readContext.STInfo.ServicePackageVersionInstance;
                        if (reader.PeekChar() == L'(')
                        {
                            if (!reader.Read(L'(', L')', spvi))
                            {
                                errLogger.Log(L"ServicePackageVersionInstance");
                                return false;
                            }
                        }

                        std::set<wchar_t> flagsSet;
                        ReplicaCloseMode replicaCloseMode;
                        int specifiedUpdateVersion = 0;
                        ReadFlagsSet(reader, flagsSet, specifiedUpdateVersion, replicaCloseMode);
                        UpdateFlags(failoverUnit, flagsSet, errLogger);
                        failoverUnit->Test_SetReplicaCloseMode(replicaCloseMode);
                        
                        if (flagsSet.find(L'M') != flagsSet.end())
                        {
                            failoverUnit->Test_SetServiceTypeRegistration(readContext.STInfo.CreateServiceTypeRegistration(spvi, readContext.FUID.Guid));
                        }

                        if (!reader.IsEOF && reader.PeekChar() == L'(')
                        {
                            // Parse sender node
                            reader.ReadString(L':');

                            int senderNodeId = 0;
                            if (!reader.Read(L')', ReadOption::None, senderNodeId))
                            {
                                errLogger.Log(L"SenderNodeId");
                                return false;
                            }

                            failoverUnit->SenderNode = CreateNodeInstanceEx(senderNodeId, 1);
                        }

                        SetServiceDescription(failoverUnit, specifiedUpdateVersion, readContext.STInfo.SD);

                        vector<Replica> replicas;
                        if (!reader.ReadVector(L'[', L']', replicas))
                        {
                            errLogger.Log(L"Replicas");
                            return false;
                        }

                        for (auto const & it : replicas)
                        {
                           failoverUnit->Test_GetReplicaStore().Test_AddReplica(it);
                        }

                        // TEst code so okay to get pointers like this
                        Reliability::ReconfigurationAgentComponent::ReconfigurationState reconfigState(&failoverUnit->FailoverUnitDescription, &failoverUnit->ServiceDescription);
                        reconfigState.Test_SetReconfigurationStage(reconfigStage);

                        if (reconfigStage != FailoverUnitReconfigurationStage::None)
                        {
                            reconfigState.Test_SetStartTime(Common::Stopwatch::Now());
                            
                            if (!failoverUnit->FailoverUnitDescription.IsPrimaryChangeBetweenPCAndCC)
                            {
                                reconfigState.Test_SetReconfigType(ReconfigurationType::Other);
                            }
                            else if (failoverUnit->LocalReplica.CurrentConfigurationRole == ReplicaRole::Primary)
                            {
                                reconfigState.Test_SetReconfigType(ReconfigurationType::Failover);
                            }
                            else
                            {
                                reconfigState.Test_SetReconfigType(ReconfigurationType::SwapPrimary);
                            }
                        }

                        failoverUnit->Test_SetReconfigurationState(reconfigState);

                        // Set the retryable error state
                        if (failoverUnit->LocalReplicaClosePending.IsSet)
                        {
                            failoverUnit->RetryableErrorStateObj.Test_Set(RetryableErrorStateName::ReplicaClose, 0);
                        }
                        else if (failoverUnit->LocalReplicaOpenPending.IsSet)
                        {
                            auto mode = failoverUnit->OpenMode == RAReplicaOpenMode::Open ? RetryableErrorStateName::ReplicaOpen : RetryableErrorStateName::ReplicaReopen;
                            failoverUnit->RetryableErrorStateObj.Test_Set(mode, 0);
                        }
                        else if (failoverUnit->Test_GetReconfigurationState().IsCatchupReconfigurationStage)
                        {
                            failoverUnit->RetryableErrorStateObj.Test_Set(RetryableErrorStateName::ReplicaChangeRoleAtCatchup, 0);
                        }

                        rv = std::move(localFT);
                        return true;
                    }

                    static ReplicaCloseModeName::Enum ReadCloseMode(wchar_t ch)
                    {
                        switch (ch)
                        {
                        case L'a':
                            return ReplicaCloseModeName::Abort;
                        case L'c':
                            return ReplicaCloseModeName::Close;
                        case L'd':
                            return ReplicaCloseModeName::Drop;
                        case L'n':
                            return ReplicaCloseModeName::DeactivateNode;
                        case L'r':
                            return ReplicaCloseModeName::Restart;
                        case L'l':
                            return ReplicaCloseModeName::Delete;
                        case L'v':
                            return ReplicaCloseModeName::Deactivate;
                        case L'f':
                            return ReplicaCloseModeName::ForceDelete;
                        case L'q':
                            return ReplicaCloseModeName::QueuedDelete;
                        case L'b':
                            return ReplicaCloseModeName::ForceAbort;
                        case L'o':
                            return ReplicaCloseModeName::Obliterate;
                        default:
                            return ReplicaCloseModeName::None;
                        };
                    }

                    static void ReadFlagsSet(
                        Reader & reader,
                        std::set<wchar_t> & flagsSet,
                        int & specifiedUpdateVersionOut,
                        ReplicaCloseMode & replicaCloseModeOut)
                    {
                        std::wstring flags = reader.ReadString(L' ');

                        specifiedUpdateVersionOut = -1;
                        replicaCloseModeOut = ReplicaCloseMode();

                        if (flags == L"-")
                        {
                            return;
                        }

                        for (size_t i = 0; i < flags.size(); i++)
                        {
                            auto ch = flags[i];
                            if (iswdigit(ch))
                            {
                                specifiedUpdateVersionOut = ch - L'0';
                            }
                            else
                            {
                                if (ch == L'H')
                                {
                                    if (i == flags.size() - 1)
                                    {
                                        Verify::Fail(L"Unexpected EOF while reading flags - replica close mode unspecified");
                                        continue;
                                    }

                                    replicaCloseModeOut = ReplicaCloseMode(ReadCloseMode(flags[i + 1]));
                                }

                                flagsSet.insert(ch);
                            }
                        }

                        return;
                    }

                    static bool UpdateFlags(
                        FailoverUnit * failoverUnit,
                        std::set<wchar_t> const  & flagsSet,
                        ErrorLogger & errLogger)
                    {
                        UpdateFlagState(flagsSet, L'N', failoverUnit->MessageRetryActiveFlag);
                        UpdateFlagState(flagsSet, L'P', failoverUnit->CleanupPendingFlag);
                        UpdateFlagState(flagsSet, L'H', failoverUnit->LocalReplicaClosePending);
                        UpdateFlagState(flagsSet, L'S', failoverUnit->LocalReplicaOpenPending);
                        UpdateFlagState(flagsSet, L'T', failoverUnit->Test_GetLocalReplicaServiceDescriptionUpdatePending());
                        UpdateFlagState(flagsSet, L'U', failoverUnit->Test_GetReplicaUploadState().Test_GetSet());
                        
                        bool replicaUpPending = flagsSet.find('K') != flagsSet.end();
                        bool replicaDownPending = flagsSet.find('I') != flagsSet.end();
                        bool replicaDroppedPending = flagsSet.find('G') != flagsSet.end();
                        bool endpointUpdatePending = flagsSet.find('E') != flagsSet.end();
                        int count = 0;

                        if (replicaUpPending)
                        {
                            failoverUnit->Test_FMMessageStateObj().Test_SetMessageStage(FMMessageStage::ReplicaUp);
                            count++;
                        }

                        if (replicaDownPending)
                        {
                            failoverUnit->Test_FMMessageStateObj().Test_SetMessageStage(FMMessageStage::ReplicaDown);
                            count++;
                        }

                        if (replicaDroppedPending)
                        {
                            failoverUnit->Test_FMMessageStateObj().Test_SetMessageStage(FMMessageStage::ReplicaDropped);
                            count++;
                        }

                        if (endpointUpdatePending)
                        {
                            failoverUnit->Test_FMMessageStateObj().Test_SetMessageStage(FMMessageStage::EndpointAvailable);
                            
                            failoverUnit->Test_GetEndpointPublishState().Test_SetEndpointPublishPending();
                        }

                        if (count > 1)
                        {
                            errLogger.Log(L"Too many fm flags");
                            return false;
                        }

                        failoverUnit->IsUpdateReplicatorConfiguration = flagsSet.find(L'B') != flagsSet.end();
                        failoverUnit->Test_SetLocalReplicaOpen(flagsSet.find(L'C') != flagsSet.end());
                        failoverUnit->Test_SetLocalReplicaDeleted(flagsSet.find(L'L') != flagsSet.end());

                        return true;
                    }

                    static void SetServiceDescription(
                        FailoverUnit * failoverUnit,
                        int specifiedUpdateVersion,
                        Reliability::ServiceDescription const & original)
                    {
                        auto copy = original;

                        if (specifiedUpdateVersion != -1)
                        {
                            copy.UpdateVersion = static_cast<uint64>(specifiedUpdateVersion);
                        }

                        failoverUnit->Test_SetServiceDescription(copy);
                    }

                    static void UpdateFlagState(
                        std::set<wchar_t> const & flagsSet, 
                        wchar_t id, 
                        Infrastructure::SetMembershipFlag & setMembershipFlag)
                    {
                        setMembershipFlag.Test_SetValue(flagsSet.find(id) != flagsSet.end());
                    }
                };

#pragma endregion

            }
        }
    }
}
