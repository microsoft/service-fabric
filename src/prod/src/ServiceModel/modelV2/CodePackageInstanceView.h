//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        class InstanceState
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
            , public Common::ISizeEstimator
        {
        public:
            InstanceState() = default;
            InstanceState(
                std::wstring const & state,
                DWORD const exitCode);

            __declspec(property(get=get_state)) std::wstring const & state;
            std::wstring const & get_state() const { return state_; }

            __declspec(property(get=get_exitCode)) DWORD exitCode;
            DWORD get_exitCode() const { return exitCode_; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::state, state_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::exitCode, exitCode_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(state_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(exitCode_)
            END_DYNAMIC_SIZE_ESTIMATION()

            FABRIC_FIELDS_02(
                state_,
                exitCode_);

        private:
            std::wstring state_;
            DWORD exitCode_;
        };
 
        class CodePackageInstanceView
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
        {
        public:
            CodePackageInstanceView() = default;
            CodePackageInstanceView(
                int restartCount,
                InstanceState const & currentState,
                InstanceState const & previousState);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::restartCount, restartCount_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::currentState, currentState_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::previousState, previousState_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            FABRIC_FIELDS_03(restartCount_, currentState_, previousState_);

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(restartCount_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(currentState_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(previousState_)
            END_DYNAMIC_SIZE_ESTIMATION()

        private:
            int restartCount_;
            InstanceState currentState_;
            InstanceState previousState_;
        };
    }
}

