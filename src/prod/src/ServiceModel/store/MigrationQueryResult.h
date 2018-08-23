// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Store
{
    class MigrationQueryResult 
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        
        MigrationQueryResult()
            : currentPhase_(MigrationPhase::Inactive)
            , state_(MigrationState::Inactive)
            , nextPhase_(MigrationPhase::Inactive)
        {
        }

        MigrationQueryResult(
            MigrationPhase::Enum current, 
            MigrationState::Enum state,
            MigrationPhase::Enum nextPhase) 
            : currentPhase_(current)
            , state_(state)
            , nextPhase_(nextPhase)
        {
        }

        __declspec(property(get = get_CurrentPhase)) MigrationPhase::Enum CurrentPhase;
        MigrationPhase::Enum get_CurrentPhase() const { return currentPhase_; }

        __declspec(property(get = get_State)) MigrationState::Enum State;
        MigrationState::Enum get_State() const { return state_; }

        __declspec(property(get = get_NextPhase)) MigrationPhase::Enum NextPhase;
        MigrationPhase::Enum get_NextPhase() const { return nextPhase_; }

        void ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_KEY_VALUE_STORE_MIGRATION_QUERY_RESULT & publicResult) const;
        
        Common::ErrorCode FromPublicApi(
            __in FABRIC_KEY_VALUE_STORE_MIGRATION_QUERY_RESULT const & publicResult);

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        FABRIC_FIELDS_03( currentPhase_, state_, nextPhase_ )

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::CurrentPhase, currentPhase_)
            SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::State, state_)
            SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::NextPhase, nextPhase_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        MigrationPhase::Enum currentPhase_;
        MigrationState::Enum state_;
        MigrationPhase::Enum nextPhase_;
    };

    using MigrationQueryResultSPtr = std::shared_ptr<MigrationQueryResult>;
}
