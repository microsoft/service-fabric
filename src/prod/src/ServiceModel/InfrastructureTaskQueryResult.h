// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace ClusterManager
    {
        class InfrastructureTaskDescription;

        namespace InfrastructureTaskState
        {
            enum Enum : int;
        }
    }
}

namespace ServiceModel
{
    class InfrastructureTaskQueryResult : public Serialization::FabricSerializable
    {
    public:
        InfrastructureTaskQueryResult();
        InfrastructureTaskQueryResult(
            std::shared_ptr<Management::ClusterManager::InfrastructureTaskDescription> && description,
            Management::ClusterManager::InfrastructureTaskState::Enum state);

        void ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_INFRASTRUCTURE_TASK_QUERY_RESULT_ITEM & result) const ;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const&) const;
        std::wstring ToString() const;

        Common::ErrorCode FromPublicApi(__in FABRIC_INFRASTRUCTURE_TASK_QUERY_RESULT_ITEM const& item);

        __declspec(property(get=get_Description)) Management::ClusterManager::InfrastructureTaskDescription const& Description;
        Management::ClusterManager::InfrastructureTaskDescription const& get_Description() const { return *description_; }

        __declspec(property(get=get_State)) Management::ClusterManager::InfrastructureTaskState::Enum State;
        Management::ClusterManager::InfrastructureTaskState::Enum get_State() const { return state_; }

        FABRIC_FIELDS_02(description_, state_);

    private:
        // shared_ptr to allow forward declaration
        std::shared_ptr<Management::ClusterManager::InfrastructureTaskDescription> description_;
        Management::ClusterManager::InfrastructureTaskState::Enum state_;
    };
}
