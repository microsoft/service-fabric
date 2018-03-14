// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class Constants
        {
        public:
            // -------------------------------
            // General
            // -------------------------------
            
            static Common::GlobalWString TokenDelimeter;
            static FABRIC_INSTANCE_ID UnusedInstanceValue;
 
            // -------------------------------
            // Store Data Keys
            // -------------------------------

            static Common::GlobalWString StoreType_ReplicaHealthEvent;
            static Common::GlobalWString StoreType_NodeHealthEvent;
            static Common::GlobalWString StoreType_PartitionHealthEvent;
            static Common::GlobalWString StoreType_ServiceHealthEvent;
            static Common::GlobalWString StoreType_ApplicationHealthEvent;
            static Common::GlobalWString StoreType_DeployedApplicationHealthEvent;
            static Common::GlobalWString StoreType_DeployedServicePackageHealthEvent;
            static Common::GlobalWString StoreType_ClusterHealthEvent;

            static Common::GlobalWString StoreType_ReplicaHealthAttributes;
            static Common::GlobalWString StoreType_NodeHealthAttributes;
            static Common::GlobalWString StoreType_PartitionHealthAttributes;
            static Common::GlobalWString StoreType_ServiceHealthAttributes;
            static Common::GlobalWString StoreType_ApplicationHealthAttributes;
            static Common::GlobalWString StoreType_DeployedApplicationHealthAttributes;
            static Common::GlobalWString StoreType_DeployedServicePackageHealthAttributes;
            static Common::GlobalWString StoreType_ClusterHealthAttributes;

            static Common::GlobalWString StoreType_ReplicaSequenceStream;
            static Common::GlobalWString StoreType_NodeSequenceStream;
            static Common::GlobalWString StoreType_PartitionSequenceStream;
            static Common::GlobalWString StoreType_ServiceSequenceStream;
            static Common::GlobalWString StoreType_ApplicationSequenceStream;
            static Common::GlobalWString StoreType_DeployedApplicationSequenceStream;
            static Common::GlobalWString StoreType_DeployedServicePackageSequenceStream;
            static Common::GlobalWString StoreType_ClusterSequenceStream;

            static Common::GlobalWString StoreType_ReplicaTypeString;
            static Common::GlobalWString StoreType_NodeTypeString;
            static Common::GlobalWString StoreType_PartitionTypeString;
            static Common::GlobalWString StoreType_ServiceTypeString;
            static Common::GlobalWString StoreType_ApplicationTypeString;
            static Common::GlobalWString StoreType_DeployedApplicationTypeString;
            static Common::GlobalWString StoreType_DeployedServicePackageTypeString;
            static Common::GlobalWString StoreType_ClusterTypeString;
            
            // -------------------------------
            // Job Queue and Job Items identifiers
            // -------------------------------

            static Common::GlobalWString ProcessReportJobItemId;
            static Common::GlobalWString DeleteEntityJobItemId;
            static Common::GlobalWString CleanupEntityJobItemId;
            static Common::GlobalWString CleanupEntityExpiredTransientEventsJobItemId;
            static Common::GlobalWString ProcessSequenceStreamJobItemId;
            static Common::GlobalWString CheckInMemoryEntityDataJobItemId;

            // -------------------------------
            // Test parameters
            // -------------------------------
            static double HealthEntityCacheLoadStoreDataDelayInSeconds;
            static void Test_SetHealthEntityCacheLoadStoreDataDelayInSeconds(double delay) { HealthEntityCacheLoadStoreDataDelayInSeconds = delay; }
        };

#define GET_HM_RC( ResourceName ) \
    Common::StringResource::Get( IDS_HM_##ResourceName )

#define TEMPLATE_SPECIALIZATION_ENTITY_ID(CLASS_NAME) \
    template class CLASS_NAME<NodeHealthId>;          \
    template class CLASS_NAME<ClusterHealthId>;       \
    template class CLASS_NAME<ReplicaHealthId>;       \
    template class CLASS_NAME<PartitionHealthId>;     \
    template class CLASS_NAME<ServiceHealthId>;       \
    template class CLASS_NAME<ApplicationHealthId>;   \
    template class CLASS_NAME<DeployedApplicationHealthId>;      \
    template class CLASS_NAME<DeployedServicePackageHealthId>;   \


#define TEMPLATE_SPECIALIZATION_ENTITY(CLASS_NAME)    \
    template class CLASS_NAME<NodeEntity>;            \
    template class CLASS_NAME<ReplicaEntity>;         \
    template class CLASS_NAME<ClusterEntity>;         \
    template class CLASS_NAME<PartitionEntity>;       \
    template class CLASS_NAME<ServiceEntity>;         \
    template class CLASS_NAME<ApplicationEntity>;     \
    template class CLASS_NAME<DeployedApplicationEntity>;        \
    template class CLASS_NAME<DeployedServicePackageEntity>;     \


#define TEMPLATE_STORE_DATA_GET_TYPE(CLASS_NAME,TYPE_EXTENSION) \
    template<> std::wstring const & CLASS_NAME<NodeHealthId>::get_Type() const { return Constants::StoreType_Node##TYPE_EXTENSION; } \
    template<> std::wstring const & CLASS_NAME<ClusterHealthId>::get_Type() const { return Constants::StoreType_Cluster##TYPE_EXTENSION; } \
    template<> std::wstring const & CLASS_NAME<ReplicaHealthId>::get_Type() const { return Constants::StoreType_Replica##TYPE_EXTENSION; } \
    template<> std::wstring const & CLASS_NAME<PartitionHealthId>::get_Type() const { return Constants::StoreType_Partition##TYPE_EXTENSION; } \
    template<> std::wstring const & CLASS_NAME<ServiceHealthId>::get_Type() const { return Constants::StoreType_Service##TYPE_EXTENSION; } \
    template<> std::wstring const & CLASS_NAME<ApplicationHealthId>::get_Type() const { return Constants::StoreType_Application##TYPE_EXTENSION; } \
    template<> std::wstring const & CLASS_NAME<DeployedApplicationHealthId>::get_Type() const { return Constants::StoreType_DeployedApplication##TYPE_EXTENSION; } \
    template<> std::wstring const & CLASS_NAME<DeployedServicePackageHealthId>::get_Type() const { return Constants::StoreType_DeployedServicePackage##TYPE_EXTENSION; } \


#define TEMPLATE_STORE_DATA_GET_TYPE_STRING(CLASS_NAME) \
    template<> std::wstring const & CLASS_NAME<NodeHealthId>::GetEntityTypeString() { return Constants::StoreType_NodeTypeString; } \
    template<> std::wstring const & CLASS_NAME<ClusterHealthId>::GetEntityTypeString() { return Constants::StoreType_ClusterTypeString; } \
    template<> std::wstring const & CLASS_NAME<PartitionHealthId>::GetEntityTypeString() { return Constants::StoreType_PartitionTypeString; } \
    template<> std::wstring const & CLASS_NAME<ReplicaHealthId>::GetEntityTypeString() { return Constants::StoreType_ReplicaTypeString; } \
    template<> std::wstring const & CLASS_NAME<ServiceHealthId>::GetEntityTypeString() { return Constants::StoreType_ServiceTypeString; } \
    template<> std::wstring const & CLASS_NAME<ApplicationHealthId>::GetEntityTypeString() { return Constants::StoreType_ApplicationTypeString; } \
    template<> std::wstring const & CLASS_NAME<DeployedApplicationHealthId>::GetEntityTypeString() { return Constants::StoreType_DeployedApplicationTypeString; } \
    template<> std::wstring const & CLASS_NAME<DeployedServicePackageHealthId>::GetEntityTypeString() { return Constants::StoreType_DeployedServicePackageTypeString; } \


    }
}
