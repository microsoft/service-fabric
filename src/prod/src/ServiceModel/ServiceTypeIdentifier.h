// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ServiceTypeIdentifier : public Serialization::FabricSerializable
    {
    public:        
        static Common::Global<ServiceTypeIdentifier> FailoverManagerServiceTypeId;
        static Common::Global<ServiceTypeIdentifier> ClusterManagerServiceTypeId;
        static Common::Global<ServiceTypeIdentifier> FileStoreServiceTypeId;
        static Common::Global<ServiceTypeIdentifier> NamingStoreServiceTypeId;
        static Common::Global<ServiceTypeIdentifier> TombStoneServiceTypeId;
        static Common::Global<ServiceTypeIdentifier> RepairManagerServiceTypeId;
        static Common::Global<ServiceTypeIdentifier> NamespaceManagerServiceTypeId;
        static Common::Global<ServiceTypeIdentifier> FaultAnalysisServiceTypeId;
        static Common::Global<ServiceTypeIdentifier> BackupRestoreServiceTypeId;
        static Common::Global<ServiceTypeIdentifier> UpgradeOrchestrationServiceTypeId;
        static Common::Global<ServiceTypeIdentifier> CentralSecretServiceTypeId;
        static Common::Global<ServiceTypeIdentifier> EventStoreServiceTypeId;
        static Common::Global<ServiceTypeIdentifier> GatewayResourceManagerServiceTypeId;

        ServiceTypeIdentifier();
        ServiceTypeIdentifier(ServicePackageIdentifier const & packageIdentifier, std::wstring const & serviceTypeName);
        ServiceTypeIdentifier(ServiceTypeIdentifier const & other);
        ServiceTypeIdentifier(ServiceTypeIdentifier && other);

        __declspec(property(get=get_ApplicationId, put=set_ApplicationId)) ApplicationIdentifier const & ApplicationId;
        inline ApplicationIdentifier const & get_ApplicationId() const { return packageIdentifier_.ApplicationId; };
        void set_ApplicationId(ApplicationIdentifier const & value) { packageIdentifier_.ApplicationId = value; }

        __declspec(property(get=get_ServicePackageId)) ServicePackageIdentifier const & ServicePackageId;
        inline ServicePackageIdentifier const & get_ServicePackageId() const { return packageIdentifier_; };

        __declspec(property(get=get_ServiceTypeName)) std::wstring const & ServiceTypeName;
        inline std::wstring const & get_ServiceTypeName() const { return serviceTypeName_; };

        __declspec(property(get=get_QualifiedServiceTypeName)) std::wstring const QualifiedServiceTypeName;
        inline std::wstring const get_QualifiedServiceTypeName() const { return GetQualifiedServiceTypeName(packageIdentifier_, serviceTypeName_); };

        ServiceTypeIdentifier const & operator = (ServiceTypeIdentifier const & other);
        ServiceTypeIdentifier const & operator = (ServiceTypeIdentifier && other);

        bool operator == (ServiceTypeIdentifier const & other) const;
        bool operator != (ServiceTypeIdentifier const & other) const;

        int compare(ServiceTypeIdentifier const & other) const;
        bool operator < (ServiceTypeIdentifier const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        bool IsSystemServiceType() const;
        bool IsTombStoneServiceType() const;
        bool IsAdhocServiceType() const;

        std::wstring ToString() const;
        static Common::ErrorCode FromString(
            std::wstring const & qualifiedServiceTypeName, 
            __out ServiceTypeIdentifier & serviceTypeIdentifier);

        static std::wstring GetQualifiedServiceTypeName(
            ServicePackageIdentifier const & packageIdentifier, 
            std::wstring const & serviceTypeName);

        static std::wstring GetQualifiedServiceTypeName(
            std::wstring const & applicationId, 
            std::wstring const & servicePackageName, 
            std::wstring const & serviceTypeName);

        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name)
        {   
            traceEvent.AddField<std::wstring>(name + ".identifier");
            return "{0}";
        }

        void FillEventData(Common::TraceEventContext & context) const
        {
            context.WriteCopy<std::wstring>(ToString());
        }

        FABRIC_FIELDS_02(packageIdentifier_, serviceTypeName_);

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(packageIdentifier_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(serviceTypeName_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        ServicePackageIdentifier packageIdentifier_;
        std::wstring serviceTypeName_;

        static Common::GlobalWString Delimiter;
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ServiceTypeIdentifier);
