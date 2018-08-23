// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class JobItemContextBase
        {
            DENY_COPY(JobItemContextBase);

        public:
            static const bool HasCorrelatedTrace = false;

            JobItemContextBase()
            {
            }

            JobItemContextBase(JobItemContextBase &&)
            {
            }

            virtual ~JobItemContextBase()
            {
            }
        };       

        class NodeUpdateServiceJobItemContext
        {
        public:
            static const bool HasCorrelatedTrace = false;

            NodeUpdateServiceJobItemContext()
            : wasUpdated_(false)
            {
            }

            NodeUpdateServiceJobItemContext(NodeUpdateServiceJobItemContext && other)
            : wasUpdated_(std::move(other.wasUpdated_))
            {
            }

            __declspec(property(get=get_WasUpdated)) bool WasUpdated;
            bool get_WasUpdated() const { return wasUpdated_; }

            void MarkUpdated()
            {
                wasUpdated_ = true;
            }

        private:
            bool wasUpdated_;
        };

        class QueryInput
        {
            DENY_COPY(QueryInput);

        public:
            QueryInput(std::wstring && applicationName, std::wstring && serviceManifestName)
            : applicationName_(std::move(applicationName)),
              serviceManifestName_(std::move(serviceManifestName))
            {
                isAdHoc_ = (Common::StringUtility::Compare(applicationName_, Common::NamingUri::RootNamingUriString) == 0);
            }

            QueryInput(QueryInput && other)
            : applicationName_(std::move(other.applicationName_)),
              serviceManifestName_(std::move(other.serviceManifestName_))
            {
                isAdHoc_ = (Common::StringUtility::Compare(applicationName_, Common::NamingUri::RootNamingUriString) == 0);
            }

            __declspec(property(get=get_ApplicationName)) std::wstring const & ApplicationName;
            std::wstring const & get_ApplicationName() const { return applicationName_; }

            __declspec(property(get=get_ServiceManifestName)) std::wstring const & ServiceManifestName;
            std::wstring const & get_ServiceManifestName() const { return serviceManifestName_; }

            __declspec(property(get=get_IsFilteredByServiceManifestName)) bool IsFilteredByServiceManifestName;
            bool get_IsFilteredByServiceManifestName() const { return !serviceManifestName_.empty(); }

            __declspec(property(get=get_IsAdHoc)) bool IsAdHoc;
            bool get_IsAdHoc() const { return isAdHoc_; }

        private:
            std::wstring const applicationName_;
            std::wstring const serviceManifestName_;
            bool isAdHoc_;
        };

        class QueryJobItemContext
        {
            DENY_COPY(QueryJobItemContext);

        public:
            static const bool HasCorrelatedTrace = false;

            QueryJobItemContext()
            : isValid_(false)
            {
            }

            QueryJobItemContext(QueryJobItemContext && other)
            : isValid_(std::move(other.isValid_)),
              result_(std::move(other.result_))
            {
            }

            __declspec(property(get=get_IsValid)) bool IsValid;
            bool get_IsValid() const { return isValid_; }

            void AddResultToFinalOutput(std::vector<ServiceModel::DeployedServiceReplicaQueryResult> & finalOutput)
            {
                ASSERT_IF(!isValid_, "Result must be valid to add to final output");
                finalOutput.push_back(std::move(result_));
            }

            void SetResult(ServiceModel::DeployedServiceReplicaQueryResult && result)
            {
                result_ = std::move(result);
                isValid_ = true;
            }

        private:
            bool isValid_;
            ServiceModel::DeployedServiceReplicaQueryResult result_;
        };

        class AppHostClosedInput
        {
            DENY_COPY(AppHostClosedInput);

        public:
            AppHostClosedInput(Common::ActivityDescription const & activityDescription, std::wstring const & hostId)
            : activityDescription_(activityDescription)
            , hostId_(hostId)
            {
            }

            AppHostClosedInput(AppHostClosedInput && other)
            : activityDescription_(std::move(other.activityDescription_))
            , hostId_(std::move(other.hostId_))
            {
            }

            __declspec(property(get = get_ActivityDescription)) Common::ActivityDescription const & ActivityDescription;
            Common::ActivityDescription const & get_ActivityDescription() const { return activityDescription_; }

            __declspec(property(get=get_HostId)) std::wstring const & HostId;
            std::wstring const & get_HostId() const { return hostId_; }

        private:
            Common::ActivityDescription activityDescription_;
            std::wstring const hostId_;
        };

        class RuntimeClosedInput
        {
            DENY_COPY(RuntimeClosedInput);

        public:
            RuntimeClosedInput(std::wstring const & hostId, std::wstring const & runtimeId)
            : hostId_(hostId),
              runtimeId_(runtimeId)
            {
            }

            RuntimeClosedInput(RuntimeClosedInput && other)
            : hostId_(std::move(other.hostId_)),
              runtimeId_(std::move(other.runtimeId_))
            {
            }

            __declspec(property(get=get_HostId)) std::wstring const & HostId;
            std::wstring const & get_HostId() const { return hostId_; }

            __declspec(property(get=get_RuntimeId)) std::wstring const & RuntimeId;
            std::wstring const & get_RuntimeId() const { return runtimeId_; }

        private:
            std::wstring const hostId_;
            std::wstring const runtimeId_;
        };

        class ServiceTypeRegisteredInput
        {
            DENY_COPY(ServiceTypeRegisteredInput);

        public:
            ServiceTypeRegisteredInput(Hosting2::ServiceTypeRegistrationSPtr const & registration)
            : registration_(registration)
            {
            }

            ServiceTypeRegisteredInput(ServiceTypeRegisteredInput && other)
            : registration_(std::move(other.registration_))
            {
            }

            __declspec(property(get=get_Registration)) Hosting2::ServiceTypeRegistrationSPtr const & Registration;
            Hosting2::ServiceTypeRegistrationSPtr const & get_Registration() const { return registration_; }

        private:
            Hosting2::ServiceTypeRegistrationSPtr const registration_;
        };

        class ReplicaUpReplyJobItemContext 
        {
            DENY_COPY(ReplicaUpReplyJobItemContext);

        public:
            static const bool HasCorrelatedTrace = true;
            static const bool HasTraceMetadata = false;

            ReplicaUpReplyJobItemContext(FailoverUnitInfo && ftInfo, bool isInDroppedList)
            : ftInfo_(std::move(ftInfo)),
              isInDroppedList_(isInDroppedList)
            {
            }

            ReplicaUpReplyJobItemContext(ReplicaUpReplyJobItemContext && other)
            : ftInfo_(std::move(other.ftInfo_)),
              isInDroppedList_(std::move(other.isInDroppedList_))
            {
            }

            __declspec(property(get=get_FTInfo)) FailoverUnitInfo const & FTInfo;
            FailoverUnitInfo const & get_FTInfo() const { return ftInfo_; }

            __declspec(property(get=get_IsInDroppedList)) bool IsInDroppedList;
            bool get_IsInDroppedList() const { return isInDroppedList_; }

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            void Trace(
                std::string const & entityId, 
                Federation::NodeInstance const & nodeInstance, 
                std::wstring const & activityId) const;

            void WriteToEtw(uint16 contextSequenceId) const;

        private:
            FailoverUnitInfo ftInfo_;
            bool const isInDroppedList_;
        };

        class ApplicationUpgradeEnumerateFTsJobItemContext
        {
            DENY_COPY(ApplicationUpgradeEnumerateFTsJobItemContext);

        public:
            static const bool HasCorrelatedTrace = false;

            ApplicationUpgradeEnumerateFTsJobItemContext()
            : isAffectedByUpgrade_(false),
              isDeletedByUpgrade_(false),
              servicePackageRolloutVersion_(ServiceModel::RolloutVersion::Invalid)
            {
            }

            ApplicationUpgradeEnumerateFTsJobItemContext(ApplicationUpgradeEnumerateFTsJobItemContext && other)
            : isAffectedByUpgrade_(std::move(other.isAffectedByUpgrade_)),
              isDeletedByUpgrade_(std::move(other.isDeletedByUpgrade_)),
              servicePackageName_(std::move(other.servicePackageName_)),
              servicePackageRolloutVersion_(std::move(other.servicePackageRolloutVersion_))
            {
            }

            void MarkAsPartOfUpgrade(std::wstring const & servicePackageName, ServiceModel::RolloutVersion const & servicePackageRolloutVersion)
            {
                ASSERT_IF(isAffectedByUpgrade_ || isDeletedByUpgrade_, "These methods can only be called once");
                isAffectedByUpgrade_ = true;

                servicePackageName_ = servicePackageName;
                servicePackageRolloutVersion_ = servicePackageRolloutVersion;
            }

            void MarkAsDeletedByUpgrade()
            {
                ASSERT_IF(isAffectedByUpgrade_ || isDeletedByUpgrade_, "These methods can only be called once");
                isDeletedByUpgrade_ = true;
            }

            void AddToListIfAffectedByUpgrade(
                Infrastructure::EntityEntryBaseSPtr const & ftId, 
                Infrastructure::EntityEntryBaseList & ftsAffectedByUpgrade,
                Infrastructure::EntityEntryBaseList & ftsBeingDeletedByUpgrade,
                std::map<std::wstring, ServiceModel::RolloutVersion> & affectedPackages) const
            {
                ASSERT_IF(isAffectedByUpgrade_ && isDeletedByUpgrade_, "Only one can be set");
                if (isDeletedByUpgrade_)
                {
                    ftsBeingDeletedByUpgrade.push_back(ftId);
                    return;
                }

                if (isAffectedByUpgrade_)
                {
                    ASSERT_IF(servicePackageName_.empty(), "servicePackageName_ cannot be empty");
                    ASSERT_IF(servicePackageRolloutVersion_ == ServiceModel::RolloutVersion::Invalid, "Rollout version must be set");

                    ftsAffectedByUpgrade.push_back(ftId);

                    auto it = affectedPackages.find(servicePackageName_);
                    if (it != affectedPackages.end())
                    {
                        return;
                    }
                    
                    affectedPackages[servicePackageName_] = servicePackageRolloutVersion_;
                }
            }

        private:
            bool isDeletedByUpgrade_;
            bool isAffectedByUpgrade_;
            std::wstring servicePackageName_;
            ServiceModel::RolloutVersion servicePackageRolloutVersion_;
        };
        
        class ApplicationUpgradeUpdateVersionJobItemContext 
        {
            DENY_COPY(ApplicationUpgradeUpdateVersionJobItemContext);
        public:
            static const bool HasCorrelatedTrace = false;

            ApplicationUpgradeUpdateVersionJobItemContext()
            : wasClosedAsPartOfUpgrade_(false)
            {                
            }

            ApplicationUpgradeUpdateVersionJobItemContext(ApplicationUpgradeUpdateVersionJobItemContext && other)
            : wasClosedAsPartOfUpgrade_(std::move(other.wasClosedAsPartOfUpgrade_))
            {
            }

            __declspec(property(get=get_WasClosedAsPartOfUpgrade)) bool WasClosedAsPartOfUpgrade;
            bool get_WasClosedAsPartOfUpgrade() const { return wasClosedAsPartOfUpgrade_; }

            void MarkAsClosedForUpgrade()
            {
                wasClosedAsPartOfUpgrade_ = true;
            }

        private:
            bool wasClosedAsPartOfUpgrade_;
        };

        class ApplicationUpgradeReplicaDownCompletionCheckJobItemContext
        {
            DENY_COPY(ApplicationUpgradeReplicaDownCompletionCheckJobItemContext);

        public:
            static const bool HasCorrelatedTrace = false;

            ApplicationUpgradeReplicaDownCompletionCheckJobItemContext()
            : isReplicaDownReplyReceived_(false)
            {
            }

            ApplicationUpgradeReplicaDownCompletionCheckJobItemContext(ApplicationUpgradeReplicaDownCompletionCheckJobItemContext && other)
            : isReplicaDownReplyReceived_(std::move(other.isReplicaDownReplyReceived_))
            {
            }

            __declspec(property(get=get_IsReplyReceived)) bool IsReplyReceived;
            bool get_IsReplyReceived() const { return isReplicaDownReplyReceived_; }

            void MarkAsReplyReceived()
            {
                ASSERT_IF(isReplicaDownReplyReceived_, "Reply cannot be received twice");
                isReplicaDownReplyReceived_ = true;
            }

            void AddToListIfReplyNotReceived(
                Reliability::FailoverUnitId const & ftId, 
                std::vector<Reliability::FailoverUnitId> & list) const
            {
                if (!isReplicaDownReplyReceived_)
                {
                    list.push_back(ftId);
                }
            }

        private:
            bool isReplicaDownReplyReceived_;
        };

        class GetLfumJobItemContext
        {
            DENY_COPY(GetLfumJobItemContext);

        public:
            static const bool HasCorrelatedTrace = false;

            GetLfumJobItemContext()
            : hasConfiguration_(false)
            {
            }

            GetLfumJobItemContext(GetLfumJobItemContext && other)
            : configuration_(std::move(other.configuration_)),
              hasConfiguration_(std::move(other.hasConfiguration_))
            {
            }

            void SetConfiguration(FailoverUnitInfo && configuration)
            {
                ASSERT_IF(hasConfiguration_, "Cannot set configuration twice");
                configuration_ = std::move(configuration);
                hasConfiguration_ = true;
            }

            void AddToList(std::vector<FailoverUnitInfo> & ftInfos)
            {
                if (hasConfiguration_)
                {
                    ftInfos.push_back(std::move(configuration_));
                }
            }

        private:
            bool hasConfiguration_;
            FailoverUnitInfo configuration_;
        };

        class GetLfumInput
        {
            DENY_COPY(GetLfumInput);

        public:
            GetLfumInput(Reliability::GenerationHeader const & header, bool anyReplicaFound)
            : generationHeader_(header),
              anyReplicaFound_(anyReplicaFound)
            {
            }

            GetLfumInput(GetLfumInput && other)
            : generationHeader_(std::move(other.generationHeader_)),
              anyReplicaFound_(std::move(other.anyReplicaFound_))
            {
            }

            __declspec(property(get=get_GenerationHeader)) Reliability::GenerationHeader const & GenerationHeader;
            Reliability::GenerationHeader const & get_GenerationHeader() const { return generationHeader_; }

            __declspec(property(get=get_AnyReplicaFound)) bool AnyReplicaFound;
            bool get_AnyReplicaFound() const { return anyReplicaFound_; }

        private:
            Reliability::GenerationHeader const generationHeader_;
            bool const anyReplicaFound_;            
        };

        class GenerationUpdateJobItemContext
        {
            DENY_COPY(GenerationUpdateJobItemContext);

        public:
            static const bool HasCorrelatedTrace = false;

            GenerationUpdateJobItemContext(Reliability::Epoch const & fmServiceEpoch, Reliability::GenerationHeader const & generationHeader)
            : fmServiceEpoch_(fmServiceEpoch),
              generationHeader_(generationHeader)
            {
            }

            GenerationUpdateJobItemContext(GenerationUpdateJobItemContext && other)
            : fmServiceEpoch_(std::move(other.fmServiceEpoch_)),
              generationHeader_(std::move(other.generationHeader_))
            {
            }

            __declspec(property(get=get_GenerationHeader)) Reliability::GenerationHeader const & GenerationHeader;
            Reliability::GenerationHeader const & get_GenerationHeader() const { return generationHeader_; }

            __declspec(property(get=get_FMServiceEpoch)) Reliability::Epoch const & FMServiceEpoch;
            Reliability::Epoch const & get_FMServiceEpoch() const { return fmServiceEpoch_; }

        private:
            Reliability::Epoch const fmServiceEpoch_;
            Reliability::GenerationHeader const generationHeader_;
        };

        class NodeUpAckInput
        {
            DENY_COPY(NodeUpAckInput);

        public:
            NodeUpAckInput(NodeUpAckMessageBody && body)
            : body_(std::move(body))
            {
            }

            NodeUpAckInput(NodeUpAckInput && other)
            : body_(std::move(other.body_))
            {
            }

            __declspec(property(get=get_Body)) NodeUpAckMessageBody const & Body;
            NodeUpAckMessageBody const & get_Body() const { return body_; }

        private:
            NodeUpAckMessageBody body_;
        };

        class HostQueryJobItemContext
        {
            DENY_COPY(HostQueryJobItemContext);

        public:
            static const bool HasCorrelatedTrace = false;

            HostQueryJobItemContext(
                int64 const & replicaId,
                Federation::RequestReceiverContextUPtr && requestContext)
                : replicaId_(replicaId),
                  requestContext_(std::move(requestContext))
            {
            }

            HostQueryJobItemContext(HostQueryJobItemContext && other)
                : replicaId_(other.replicaId_)
                , requestContext_(std::move(other.requestContext_))
            {
            }

            __declspec(property(get=get_RequestContext)) std::shared_ptr<Federation::RequestReceiverContext> & RequestContext;
            std::shared_ptr<Federation::RequestReceiverContext> & get_RequestContext() { return requestContext_; }

            __declspec(property(get=get_ReplicaId)) int64 const & ReplicaId;
            int64 const & get_ReplicaId() const { return replicaId_; }

        private:
            int64 const replicaId_;
            std::shared_ptr<Federation::RequestReceiverContext> requestContext_;
        };

        class ClientReportFaultJobItemContext
        {
            DENY_COPY(ClientReportFaultJobItemContext);

        public:
            static const bool HasCorrelatedTrace = false;

            ClientReportFaultJobItemContext(
                ReportFaultRequestMessageBody && body, 
                Federation::RequestReceiverContextUPtr && context, 
                Common::ActivityId const & activityId)
            : body_(std::move(body)),
              context_(std::move(context)),
              activityId_(activityId)
            {
            }

            ClientReportFaultJobItemContext(ClientReportFaultJobItemContext && other)
            : body_(std::move(other.body_)),
              context_(std::move(other.context_)),
              activityId_(std::move(other.activityId_))
            {
            }

            __declspec(property(get=get_LocalReplicaId)) int64 LocalReplicaId;
            int64 get_LocalReplicaId() const { return body_.ReplicaId; }

            __declspec(property(get=get_FaultType)) FaultType::Enum FaultType;
            FaultType::Enum get_FaultType() const { return body_.FaultType; }

            __declspec(property(get = get_ActivityDescription)) Common::ActivityDescription const & ActivityDescription;
            Common::ActivityDescription const & get_ActivityDescription() const { return body_.ActivityDescription; }

            __declspec(property(get=get_ActivityId)) Common::ActivityId const & ActivityId;
            Common::ActivityId const & get_ActivityId() const { return activityId_; }
    
            __declspec(property(get=get_RequestContext)) Federation::RequestReceiverContextUPtr & RequestContext;
            Federation::RequestReceiverContextUPtr & get_RequestContext() { return context_; }

            __declspec(property(get = get_IsForce)) bool IsForce;
            bool get_IsForce() const { return body_.IsForce; }

        private:
            ReportFaultRequestMessageBody body_;
            Federation::RequestReceiverContextUPtr context_;
            Common::ActivityId activityId_;
        };

        class CheckReplicaCloseProgressJobItemContext
        {
            DENY_COPY(CheckReplicaCloseProgressJobItemContext);
        public:
            static const bool HasCorrelatedTrace = false;

            CheckReplicaCloseProgressJobItemContext() :
                isClosed_(true)
            {
            }

            CheckReplicaCloseProgressJobItemContext(CheckReplicaCloseProgressJobItemContext && other) :
                isClosed_(std::move(other.isClosed_)),
                registration_(std::move(other.registration_))
            {
            }

            __declspec(property(get = get_IsClosed)) bool IsClosed;
            bool get_IsClosed() const
            {
                return isClosed_;
            }

            __declspec(property(get = get_Registration)) Hosting2::ServiceTypeRegistrationSPtr Registration;
            Hosting2::ServiceTypeRegistrationSPtr get_Registration() const
            {
                return registration_;
            }

            void MarkAsStillOpen(Hosting2::ServiceTypeRegistrationSPtr const & registration)
            {
                isClosed_ = false;
                registration_ = registration;
            }

        private:
            bool isClosed_;
            Hosting2::ServiceTypeRegistrationSPtr registration_;
        };

        class UpdateEntryOnStorageLoadInput
        {
            DENY_COPY(UpdateEntryOnStorageLoadInput);

        public:
            UpdateEntryOnStorageLoadInput(Reliability::NodeUpOperationFactory & factory)
                : factory_(&factory)
            {
            }

            UpdateEntryOnStorageLoadInput(UpdateEntryOnStorageLoadInput && other)
                : factory_(std::move(other.factory_))
            {                
            }

            __declspec(property(get = get_NodeUpOperationFactory)) Reliability::NodeUpOperationFactory & NodeUpOperationFactory;
            Reliability::NodeUpOperationFactory & get_NodeUpOperationFactory() const { return *factory_; } 

        private:
            Reliability::NodeUpOperationFactory * factory_;
        };
    }
}
