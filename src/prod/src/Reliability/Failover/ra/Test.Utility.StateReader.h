// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
/* 
This file contains code that defines the RA State Machine Unit testing infrastructure

Important aspects of this are:

DEFAULT NAMESPACE
This contains defaults that are used for the run. These are constant values that are not typed in over and over and over again.

The RA and this node is (1:1)

STRING REPRESENTATION OF ITEMS

Notation:
- a '`' is a separator. there should be nothing between the fields
- a || represents that only one of those values is valid
- a <> represents an item that is optional

- ErrorCode: These are parsed as the string representation. Write HostingDeploymentInProgress for ErrorCodeValue::HostingDeploymentInProgress

- Epoch: An epoch is parsed as three numbers. DataLossNumber`PrimaryChange`ConfigurationNumber

- ReplicaRole: N||I||S||P -> None|Idle|Secondary|Primary

- ReplicaState: IC||IB||RD||ID||DD||SB -> InCreate|InBuild|Ready|InDrop|Dropped|StandBy

- ReplicaMessageStage: N||RA||RAP -> None|RA|RAP

- FT Reconfiguration Stage: Phase1_GetLSN||Phase2_Catchup||Phase3_Deactivate||Phase4_Activate||None||Abort_Phase2_Catchup

- RA Proxy Message Flags: -|| ER|C|A|D -> a '-' indicates none. The rest are all values that can be bitwise-or'd together. EndReconfiguration|Catchup|Abort|Drop

- ReplicaDescription: - 
    pcRole/ccRole ReplicaState U|D ReplicaIdAndNodeId [n]
        Invalid service package version

    Otherwise SPVI is the default
    pcRole/ccRole ReplicaState U|D ReplicaIdAndNodeId (Service PackageVersionInstance)

    pcRole/ccRole ReplicaState U|D ReplicaIdAndNodeId <First Ack LSN> <Last Ack LSN>

    pcRole/ccRole ReplicaState U|D ReplicaIdAndNodeId ReplEndpoint ServiceEndpoint  

- FailoverUnitDescription: "pcEpoch/ccEpoch"

- Generation Number: nodeId:number

- Generation Header: Optional and available for all messages as the first argument. Format is g=genNumber,fmm|fm where fmm or fm indicates who set the header. 
                     The default generation header is added to all messages with generation number 0
                     Tagged as gh

- ServicePackageVersionInstance: use "1.0:1.0:1" as this is the initial. Represented as "ApplicationMajorVersion.ApplicationMinorVersion:RolloutMajor.RolloutMinor:Instance"

- ConfigurationMessageBody: "<gh> FTDescription [Replica1] [Replica2] [Replica3]"

- DeactivateMessageBody: "<gh> FTDescription {Deactivation Info} isForce [Replica1] [Replica2] [Replica3]"

- ActivateMessageBody: "<gh> FTDescription {Deactivation Info} [Replica1] [Replica2] [Replica3]"

- FailoverUnitReplyMessageBody: "<gh> FTDescription EC" where EC = error code

- ReplicaMessageBody: "<gh> FTDescription [Replica]" 

- RAReplicaMessageBody: "<gh> FTDescription {DeactivationInfo} [Replica]"

- ReplicaReplyMessageBody: "<gh> FTDescription [Replica] ErrorCode"

- GetLSNReplyMessageBody: "<gh> FTDescription {DeactivationInfo} [Replica] ErrorCode

- ReplicaListMessageBody: "<gh> {FTShortName [Replica Desc]} {FTShortName [Replica Desc]} {FTShortName [Replica Desc]}"

- ProxyRequestMessageBody: "FTDescription [LocalReplica] [Remote1] [Remote2] ProxyMessageFlags <s>" where the s indicates that the message has service description

- ReportFaultMessageBody: "FTDescription [ReplicaDescription] FAULT_TYPE" where FAULT_TYPE = Transient|Permanent

- ProxyReplyMessageBody : "FTDescription [LocalReplica] [Remote1] [Remote2] Error Flags"

- ProxyQueryReplyMessageBody<DeployedServiceReplicaDetailQueryResult>: FTDescription [LocalReplica] Error TimeStamp [DeployedServiceReplicaDetailQueryResult]

- ProxyUpdateServiceDescriptionReplyMessageBody: "FTDescription [LocalReplica] [SD Version] Error" 

- ReplicaIdAndNodeId: ReplicaId:ReplicaInstanceId (NodeId = Replica Id and NodeInstance = 1)
                      NodeId:ReplicaId:ReplicaInstanceId (Node Instance = 1)

- ReplicaInfo: pcRole/icRole/ccRole ReplicaState U|D ReplicaIdAndNodeId <First Ack LSN> <Last Ack LSN> <Service Package Version Instance>

- Failover Unit Info: pcEpoch/icEpoch/ccEpoch [ReplicaInfo] [ReplicaInfo] (isReportFromPrimary) where isReportFromPrimary is optional and defaults to false

- ReplicaUpMessageBody: "isLast isFromFMM {FTShortName FuInfo} {FTShortName FuInfo} | {FTShortName FuInfo} {FTShortName FuInfo}. the first set of ft is the up ft list and the next is the dropped ft list

- NodeFabricUpgradeReply: <code major>.<code minor>.<code hotfix>:<cfg>:<instance id>

- NodeFabricUpgradeRequest: <code major>.<code minor>.<code hotfix>:<cfg>:<instance id> UpgradeType

- CancelFabricUpgradeReply: <code major>.<code minor>.<code hotfix>:<cfg>:<instance id>

- CancelFabricUpgradeRequest: <code major>.<code minor>.<code hotfix>:<cfg>:<instance id> UpgradeType

- UpgradeType: Rolling | Rolling_ForceRestart | Rolling_NotificationOnly

- NodeUpgradeRequest: InstanceId UpgradeType IsMonitored [PackageName RolloutVersion] [PackageName RolloutVersion] | [DeletedServiceType1] [DeletedST2]

- NodeUpgradeReply: <instance id> {FTShortName FuInfo} {FTShortName FuInfo} | {FTShortName FuInfo} {FTShortName FuInfo}. (the first is up ft list. the second is dropped ft list. app id is built using the default app name and default app number

- CancelApplicationUpgradeRequest: InstanceId UpgradeType IsMonitored [PackageName RolloutVersion] [PackageName RolloutVersion] | [DeletedServiceType1] [DeletedST2]

- CancelApplicationUpgradeReply: <instance id> {FTShortName FuInfo} {FTShortName FuInfo} | {FTShortName FuInfo} {FTShortName FuInfo}. (the first is up ft list. the second is dropped ft list. app id is built using the default app name and default app number

- NOdeUPdateServiceRequestMessageBody: SDShortName UpdateVersion [where update version is the new instance id]

- NodeUpdateServiceReplyMessageBody: SDShortName UpdateVersion [where update version is the nwew id] Instance [where instance is the instance of the service]

- NodeActivateRequestMessageBody: sequencenumber isFmm

- NodeDeactivateRequestMessageBody: sequencenumber isFmm

- NodeActivateReplyMessageBody : sequence number 

- NodeDeactivateReplyMessageBody : sequence number

- GenerationUpdateMessageBody : FMServiceEpoch

- DeployedServiceQueryRequest: "DeployedServiceQueryRequest" ApplicationName ServiceManifestName <PartitionId>

- DeployedServiceQueryReply: ErrorCode "DeployedServiceQueryResult" [DeployedServiceReplicaQueryResult1] [DeployedServiceReplicaQueryResult2]

- DeployedServiceReplicaQueryResult: FTShortNAme IsStateful ReplicaId Role(P/N/S/I) State(SB/IB/RD/DD/D) CodePackageName

- DeployedServiceReplicaDetailQueryRequest: DeployedServiceReplicaDetailQueryRequest ShortName ReplicaId 

- DeployedServiceReplicaDetailQueryReply: ErrorCode "DeployedServiceReplicaDetailQueryResult" [DeployedServiceReplicaDetailQueryResult]

- DeployedServiceReplicaDetailQueryResult: CurrentlyAutogenerated as a real impl is not required for RA tests

- ClientReportFaultRequestMessageBody: node_name ShortName ReplicaId FaultType

- ClientReportFaultReplyMessageBody: errorcode errorMessage

- HealthReport: delete <HealthReportCode> NodeId:NodeInstance:ReplicaId:ReplicaInstance
              : create <HealthReportCode> NodeId:NodeInstance:ReplicaId:ReplicaInstance Property Description 

- ServiceTypeNotificationRequestMessageBody : [Service Type Context 1] [Service Type Context 2]

- ServiceTypeNotificationReplyMessageBody : [Service Type Context 1] [Service Type Context 2]

- ServiceTypeUpdateMessageBody : [Service Type Context 1] [Service Type Context 2] ... [Service Type Context N] sequence number

- PartitionNotificationMessageBody : [Service Type Context 1] [Service Type Context 2] ... [Service Type Context N] sequence number

- ServiceTypeUpdateReplyMessageBody : sequence number

- LoadOrMoveCostDescription : ftid servicename {S\metricname\metricload\nodeid | P\metricname\nodeid}

- ReportLoad : [LoadOrMoveCostDescription 1] [LoadOrMoveCostDescription 2] .. [LoadOrMoveCostDescription N]

- ResourceUsage : ftid cpuload memoryload

- ResourceUsageReport : [ResourceUsage 1] [ResourceUsage 2] .. [ResourceUsage N

// TODO: add fields over here. For now keeping empty
- NodeUpAckMessageBody:  <is Activated> <Activation Sequence Number> <code major>.<code minor>.<code hotfix>:<cfg>:<instance id> 

A Failover Unit has a more convenient short hand notation
    
    FailoverUnitState ReconfigurationStage PCEpoch/ICEpoch/CCEpoch {Deactivation Info} LocalReplicaId:LocalReplicaInstanceId DownReplicaInstanceId (spvi) Flags (sender:nodeid) [Replica1] [Replica2] 
    
    where sender: is optional     

    Deactivation Info defaults to ccEpoch:0 

    FailoverUnitState = O|C
    where O = Open
        C = Closed
    
    ReconfigurationStage = Phase1_GetLSN|Phase2_Catchup|Phase3_Deactivate|Phase4_Activate|None|Abort_Phase2_Catchup
    
    Flags: - = None
        B = UpdateReplicatorConfiguration
        C = LocalReplicaOpen
        D = LocalReplicaReopenPending
        E = LocalReplicaEndpointUpdatePending
        G = LocalReplicaDroppedReplyPending
        Ha = LocalReplicaClosePending (Abort)
        Hb = LocalReplicaClosePending (ForceAbort)
        Hc = LocalReplicaClosePending (Close)
        Hd = LocalReplicaClosePending (Drop)
        Hf = LocalReplicaClosePending (ForceDelete)        
        Hl = LocalReplicaClosePending (Delete)
        Hn = LocalReplicaClosePending (DeactivateNode)
        Hr = LocalReplicaClosePending (Restart)
        Hv = LocalReplicaClosePending (Deactivate S->N)
        Ho = LocalReplicaClosePending (Obliterate)
        I = LocalReplicaDownMessageReplyPending
        K = LocalReplicaUpMessageReplyPending
        L = LocalReplicaDeleted
        M = Has Service Type Registration
        N = Message Retry Pending
        P = CleanupPending
        R = ReplyToFM
        S = LocalReplicaOpenPending
        T = UpdateServiceDescriptionPending
        U = Replica upload pending
        <digit> = service description update version

A Failover Unit Replica has the notation

    [Replica] [Replica] [Replica] ...

    where Replica = PCRole/ICRole/CCRole ReplicaState IsUp ReplicaMessageStage Flags ReplicaIdAndNodeId [FirstAcknowledgedLSN LastAcknowledgedLSN DeactivationInfo] (Package Version Instance) s:<service endpoint> r:<replicator endpoint>
        The LSN's, PackageVersionInstance and endpoints are optional

    ReplicaRole = N|I|S|P
    where N = None
        I = Idle
        S = Secondary
        P = Primary

    ReplicaState = IC|IB|RD|ID|DD|SB
    where IC = InCreate
        IB = InBuild
        RD = Ready
        ID = InDrop
        DD = Dropped
        SB = StandBy

    IsUp = U|D
    where U = Up
        D = Down

    ReplicaMessageStage = N|RA|RAP
    where N = None
        RA = RAReplyPending
        RAP = RAProxyReplyPending

    IsToBeDeactivated = T
    IsToBeActivated = A
    IsReplicatorRemovePending = R
    IsToBeRestarted = B

    OR where Replica = PCRole/CCRole ReplicaIdAndNodeId and all other are defaults
*/

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReliabilityUnitTest
        {
            namespace StateManagement
            {
                class ServiceTypeReadContext : public ReadContext
                {
                public:
                    ServiceTypeReadContext() { IsHostedInFabricExe = false; }

                    std::wstring ShortName;
                    Reliability::ServiceDescription SD;
                    ServiceModel::ServiceTypeIdentifier ServiceTypeId;
                    ServiceModel::ServicePackageVersionInstance ServicePackageVersionInstance;
                    std::wstring HostId;
                    std::wstring RuntimeId;
                    std::wstring CodePackageName;
                    int64 ServicePackageInstanceId;
                    bool IsHostedInFabricExe;

                    ServiceModel::CodePackageIdentifier GetCodePackageId() const
                    {
                        return ServiceModel::CodePackageIdentifier(ServiceTypeId.ServicePackageId, CodePackageName);
                    }

                    Hosting2::ServiceTypeRegistrationSPtr CreateServiceTypeRegistration() const
                    {
                        return CreateServiceTypeRegistration(ServicePackageVersionInstance);
                    }

                    Hosting2::ServiceTypeRegistrationSPtr CreateServiceTypeRegistration(ServiceModel::ServicePackageActivationContext const & activationContext) const
                    {
                        return CreateServiceTypeRegistration(ServicePackageVersionInstance, activationContext.ActivationGuid);
                    }

                    Hosting2::ServiceTypeRegistrationSPtr CreateServiceTypeRegistration(Common::Guid const & partitionId) const
                    {
                        return CreateServiceTypeRegistration(ServicePackageVersionInstance, partitionId);
                    }

                    Hosting2::ServiceTypeRegistrationSPtr CreateServiceTypeRegistration(
                        ServiceModel::ServicePackageVersionInstance const & versionInstanceOverride) const
                    {
                        return CreateServiceTypeRegistration(versionInstanceOverride, Common::Guid::Empty());
                    }

                    Hosting2::ServiceTypeRegistrationSPtr CreateServiceTypeRegistration(
                        ServiceModel::ServicePackageVersionInstance const & versionInstanceOverride,
                        Common::Guid const & partitionId) const
                    {
                        if (SD.PackageActivationMode == ServiceModel::ServicePackageActivationMode::ExclusiveProcess)
                        {
                            ASSERT_IF(partitionId == Common::Guid::Empty(), "Partition id should not be empty for exclusive");
                            return CreateServiceTypeRegistration(
                                versionInstanceOverride, 
                                ServiceModel::ServicePackageActivationContext(partitionId),
                                partitionId.ToString());
                        }
                        else
                        {
                            return CreateServiceTypeRegistration(
                                versionInstanceOverride, 
                                ServiceModel::ServicePackageActivationContext(),
                                L"");
                        }
                    }

                    Hosting2::ServiceTypeRegistrationSPtr CreateServiceTypeRegistration(
                        ServiceModel::ServicePackageVersionInstance const & versionInstanceOverride, 
                        ServiceModel::ServicePackageActivationContext const & activationContext,
                        std::wstring const & servicePackagePublicActivationId) const
                    {
                        return Hosting2::ServiceTypeRegistration::Test_Create(
                            ServiceTypeId,
                            versionInstanceOverride,
                            activationContext,
                            servicePackagePublicActivationId,
                            HostId,
                            RuntimeId,
                            CodePackageName,
                            ServicePackageInstanceId,
                            IsHostedInFabricExe ? ::GetCurrentProcessId() : ::GetCurrentProcessId() + 1);
                    }
                };

                // This class contains Context information for reading a FT
                class SingleFTReadContext : public ReadContext
                {
                public:
                    SingleFTReadContext()
                        : RA(nullptr)
                    {
                    }

                    std::wstring ShortName;
                    Reliability::FailoverUnitId FUID;
                    Reliability::ConsistencyUnitId CUID;
                    int TargetReplicaSetSize;
                    int MinReplicaSetSize;
                    ServiceTypeReadContext STInfo;
                    ReconfigurationAgent * RA;

                    FailoverUnitDescription GetFailoverUnitDescription() const
                    {
                        return FailoverUnitDescription(FUID, ConsistencyUnitDescription(CUID));
                    }
                };

                // This class contains information for reading multiple FTs in the same object
                class MultipleReadContextContainer : public ReadContext
                {
                public:
                    void AddFTContext(std::wstring const & shortName, SingleFTReadContext const & context)
                    {
                        Add(AllFTContext, shortName, context);
                    }

                    void AddFTContexts(std::vector<SingleFTReadContext const *> const & v)
                    {
                        Add(AllFTContext, v);
                    }

                    void AddSTContexts(std::vector<ServiceTypeReadContext const *> const & v)
                    {
                        Add(AllSDContext, v);
                    }

                    SingleFTReadContext const * GetSingleFTContext(std::wstring const & shortName) const
                    {
                        return Get(AllFTContext, shortName);
                    }

                    ServiceTypeReadContext const * GetSingleSDContext(std::wstring const & shortName) const
                    {
                        return Get(AllSDContext, shortName);
                    }

                private:
                    template<typename T>
                    T const * Get(std::map<std::wstring, T> const & collection, std::wstring const & shortName) const
                    {
                        auto it = collection.find(shortName);
                        if (it == collection.end())
                        {
                            return nullptr;
                        }

                        return &it->second;
                    }

                    template<typename T>
                    void Add(std::map<std::wstring, T> & collection, std::wstring const & shortName, T const & val)
                    {
                        collection[shortName] = val;
                    }

                    template<typename T>
                    void Add(std::map<std::wstring, T> & collection, std::vector<T const *> const & v)
                    {
                        for (auto it : v)
                        {
                            Add(collection, it->ShortName, *it);
                        }
                    }

                    std::map<std::wstring, SingleFTReadContext> AllFTContext;

                    std::map<std::wstring, ServiceTypeReadContext> AllSDContext;
                };           
            }
        }
    }
}
