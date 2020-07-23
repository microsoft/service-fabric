//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        // 
        // TEMPORARY to support reliable collections demo for //BUILD
        //
        class ReliableCollectionsRef
        : public Common::ISizeEstimator
        , public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
        {
        public:
            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::nameCamelCase, name_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::doNotPersistState, doNotPersistState_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            FABRIC_FIELDS_02(name_, doNotPersistState_);

            bool operator ==(ReliableCollectionsRef const &other) const
            {
                return name_ == other.name_ && doNotPersistState_ == other.doNotPersistState_;
            } 

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(name_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(doNotPersistState_)
            END_DYNAMIC_SIZE_ESTIMATION()
        private:
            std::wstring name_;
            bool doNotPersistState_ { false };

        };
    }
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ModelV2::ReliableCollectionsRef);
