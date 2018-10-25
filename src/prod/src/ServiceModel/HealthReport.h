// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class HealthReport
        : public HealthInformation
    {
        DEFAULT_COPY_CONSTRUCTOR(HealthReport)
    public:
        HealthReport();
        HealthReport(
            EntityHealthInformationSPtr && entityInformation,
            std::wstring const & sourceId,
            std::wstring const & property,
            Common::TimeSpan const & timeToLive,
            FABRIC_HEALTH_STATE state,
            std::wstring const & description,
            FABRIC_SEQUENCE_NUMBER sequenceNumber,
            bool removeWhenExpired,
            AttributeList && attributes);

        HealthReport(
            EntityHealthInformationSPtr && entityInformation,
            std::wstring && sourceId,
            std::wstring && property,
            Common::TimeSpan const & timeToLive,
            FABRIC_HEALTH_STATE state,
            std::wstring const & description,
            FABRIC_SEQUENCE_NUMBER sequenceNumber,
            bool removeWhenExpired,
            AttributeList && attributes);

        HealthReport(
            EntityHealthInformationSPtr && entityInformation,
            HealthInformation && healthInfo,
            AttributeList && attributes);

        HealthReport(HealthReport && other) = default;
        HealthReport & operator = (HealthReport && other) = default;

        __declspec(property(get=get_Kind)) FABRIC_HEALTH_REPORT_KIND Kind;
        FABRIC_HEALTH_REPORT_KIND get_Kind() const { CheckEntityExists(); return entityInformation_->Kind; }

        __declspec(property(get=get_EntityInformation)) EntityHealthInformationSPtr const & EntityInformation;
        EntityHealthInformationSPtr const & get_EntityInformation() const { return entityInformation_; }
                
        __declspec(property(get=get_Attributes,put=set_Attributes)) AttributeList const & AttributeInfo;
        AttributeList const & get_Attributes() const { return attributeList_; }
        void set_Attributes(AttributeList const & attributes) { attributeList_ = attributes; }
        
        __declspec(property(get=get_EntityPropertyId)) std::wstring const & EntityPropertyId;
        std::wstring const & get_EntityPropertyId() const;

        __declspec(property(get = get_ReportPriority)) Priority::Enum ReportPriority;
        Priority::Enum get_ReportPriority() const;

        static Priority::Enum GetPriority(FABRIC_HEALTH_REPORT_KIND kind, std::wstring const & sourceId, std::wstring const & property);
        static Common::StringCollection GetAuthoritySources(FABRIC_HEALTH_REPORT_KIND kind);
        
        static HealthReport CreateHealthInformationForDelete(
            EntityHealthInformationSPtr && entityInformation,
            std::wstring && sourceId,
            FABRIC_SEQUENCE_NUMBER sequenceNumber);

        static HealthReport CreateSystemHealthReport(
            Common::SystemHealthReportCode::Enum reportCode,
            EntityHealthInformationSPtr && entityInformation,
            std::wstring const & extraDescription,
            AttributeList && attributeList);    

        static HealthReport CreateSystemHealthReport(
            Common::SystemHealthReportCode::Enum reportCode,
            EntityHealthInformationSPtr && entityInformation,
            std::wstring const & extraDescription,
            FABRIC_SEQUENCE_NUMBER sequenceNumber,
            AttributeList && attributeList);    
        
        static HealthReport CreateSystemHealthReport(
            Common::SystemHealthReportCode::Enum reportCode,
            EntityHealthInformationSPtr && entityInformation,
            std::wstring const & extraDescription,
            FABRIC_SEQUENCE_NUMBER sequenceNumber,
            Common::TimeSpan const timeToLive,
            AttributeList && attributeList);  

        static HealthReport CreateSystemHealthReport(
            Common::SystemHealthReportCode::Enum reportCode,
            EntityHealthInformationSPtr && entityInformation,
            std::wstring const & dynamicProperty,
            std::wstring const & extraDescription,
            AttributeList && attributeList);  

        static HealthReport CreateSystemHealthReport(
            Common::SystemHealthReportCode::Enum reportCode,
            EntityHealthInformationSPtr && entityInformation,
            std::wstring const & dynamicProperty,
            std::wstring const & extraDescription,
            Common::TimeSpan const timeToLive,
            AttributeList && attributeList);  

        static HealthReport CreateSystemHealthReport(
            Common::SystemHealthReportCode::Enum reportCode,
            EntityHealthInformationSPtr && entityInformation,
            std::wstring const & dynamicProperty,
            std::wstring const & extraDescription,
            FABRIC_SEQUENCE_NUMBER sequenceNumber,
            AttributeList && attributeList);  

        static HealthReport CreateSystemHealthReport(
            Common::SystemHealthReportCode::Enum reportCode,
            EntityHealthInformationSPtr && entityInformation,
            std::wstring const & dynamicProperty,
            std::wstring const & description,
            FABRIC_SEQUENCE_NUMBER sequenceNumber,
            Common::TimeSpan const timeToLive,
            AttributeList && attributeList);  

        static HealthReport CreateSystemRemoveHealthReport(
            Common::SystemHealthReportCode::Enum reportCode,
            EntityHealthInformationSPtr && entityInformation,
            std::wstring const & dynamicProperty);  

        static HealthReport CreateSystemRemoveHealthReport(
            Common::SystemHealthReportCode::Enum reportCode,
            EntityHealthInformationSPtr && entityInformation,
            std::wstring const & dynamicProperty,
            FABRIC_SEQUENCE_NUMBER sequenceNumber);  

        static HealthReport CreateSystemDeleteEntityHealthReport(
            Common::SystemHealthReportCode::Enum reportCode,
            EntityHealthInformationSPtr && entityInformation);

        static HealthReport CreateSystemDeleteEntityHealthReport(
            Common::SystemHealthReportCode::Enum reportCode,
            EntityHealthInformationSPtr && entityInformation,
            FABRIC_SEQUENCE_NUMBER sequenceNumber);

        static HealthReport GenerateInstanceHealthReport(
            ServiceModel::HealthInformation && healthInformation,
            Common::Guid &partitionId,
            FABRIC_REPLICA_ID & instanceId);

        static HealthReport GenerateReplicaHealthReport(
            ServiceModel::HealthInformation && healthInformation,
            Common::Guid &partitionId,
            FABRIC_REPLICA_ID & replicaId);

        static HealthReport GeneratePartitionHealthReport(
            ServiceModel::HealthInformation && healthInformation,
            Common::Guid &partitionId);

        static HealthReport GenerateApplicationHealthReport(
            ServiceModel::HealthInformation && healthInformation,
            std::wstring const & applicationName);

        static HealthReport GenerateDeployedApplicationHealthReport(
            ServiceModel::HealthInformation && healthInformation,
            std::wstring const & applicationName,
            Common::LargeInteger const & nodeId,
            std::wstring const & nodeName);

        static HealthReport GenerateDeployedServicePackageHealthReport(
            ServiceModel::HealthInformation && healthInformation,
            std::wstring const & applicationName,
            std::wstring const & serviceManifestName,
            std::wstring const & servicePackageActivationId,
            Common::LargeInteger const & nodeId,
            std::wstring const & nodeName);

        void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_HEALTH_REPORT & healthReport) const;

        Common::ErrorCode FromPublicApi(FABRIC_HEALTH_REPORT const & healthReport);

        Common::ErrorCode FromPublicApi(
            FABRIC_HEALTH_REPORT const & healthReport, 
            FABRIC_STRING_PAIR_LIST const & attributeList);

        FABRIC_FIELDS_10(entityInformation_, sourceId_, property_, timeToLive_, state_, description_, sequenceNumber_, sourceUtcTimestamp_, attributeList_, removeWhenExpired_);
        
        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(entityPropertyId_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(entityInformation_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(attributeList_)
        END_DYNAMIC_SIZE_ESTIMATION()

        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
        void FillEventData(Common::TraceEventContext & context) const;
        
    private:

        void CheckEntityExists() const;

        static Common::GlobalWString Delimiter;
        static Common::Global<Common::TimeSpan> DefaultDeleteTTL;

        EntityHealthInformationSPtr entityInformation_;
        AttributeList attributeList_;

        mutable std::wstring entityPropertyId_;
        // Caches priority to avoid string comparisons
        mutable Priority::Enum priority_;
    };

    typedef std::unique_ptr<HealthReport> HealthReportUPtr;
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::HealthReport);
