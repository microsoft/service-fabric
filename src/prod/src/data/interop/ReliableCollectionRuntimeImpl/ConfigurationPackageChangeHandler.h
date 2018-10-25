// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Data
{
    namespace Interop
    {

        class ConfigurationPackageChangeHandler final
            : public KObject<ConfigurationPackageChangeHandler>
            , public KShared<ConfigurationPackageChangeHandler>
            , public IFabricConfigurationPackageChangeHandler
            , public Data::Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::RCR>
        {
            K_FORCE_SHARED(ConfigurationPackageChangeHandler)

                K_BEGIN_COM_INTERFACE_LIST(ConfigurationPackageChangeHandler)
                COM_INTERFACE_ITEM(IID_IUnknown, IFabricConfigurationPackageChangeHandler)
                COM_INTERFACE_ITEM(IID_IFabricConfigurationPackageChangeHandler, IFabricConfigurationPackageChangeHandler)
                K_END_COM_INTERFACE_LIST()

        public:
            static HRESULT ConfigurationPackageChangeHandler::Create(
                __in Utilities::PartitionedReplicaId const & traceType,
                __in IFabricStatefulServicePartition* statefulServicePartition,
                __in IFabricPrimaryReplicator* fabricPrimaryReplicator,
                __in wstring const & configPackageName,
                __in wstring const & replicatorSettingsSectionName,
                __in wstring const & replicatorSecuritySectionName,
                __in KAllocator & allocator,
                __out Common::ComPointer<IFabricConfigurationPackageChangeHandler>& result);

            //
            // IFabricConfigurationPackageChangeHandler
            //
            void STDMETHODCALLTYPE OnPackageAdded(
                __in IFabricCodePackageActivationContext *source,
                __in IFabricConfigurationPackage *configPackage);
            void STDMETHODCALLTYPE OnPackageRemoved(
                __in IFabricCodePackageActivationContext *source,
                __in IFabricConfigurationPackage *configPackage);
            void STDMETHODCALLTYPE OnPackageModified(
                __in IFabricCodePackageActivationContext *source,
                __in IFabricConfigurationPackage *previousConfigPackage,
                __in IFabricConfigurationPackage *configPackage);

        private:
            ConfigurationPackageChangeHandler(
                __in Utilities::PartitionedReplicaId const & partitionedReplicId,
                __in IFabricStatefulServicePartition* statefulServicePartition,
                __in IFabricPrimaryReplicator* fabricPrimaryReplicator,
                __in wstring const & configPackageName,
                __in wstring const & replicatorSettingsSectionName,
                __in wstring const & replicatorSecuritySectionName);

            Common::ComPointer<IFabricStatefulServicePartition> statefulServicePartition_;
            Common::ComPointer<IFabricPrimaryReplicator> fabricPrimaryReplicator_;
            wstring configPackageName_;
            wstring replicatorSettingsSectionName_;
            wstring replicatorSecuritySectionName_;
        };
    }
}