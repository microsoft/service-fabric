// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class StoreData : public Serialization::FabricSerializable
        {
        public:

            StoreData();
            StoreData(StoreData const& other);
            explicit StoreData(int64 operationLSN);
            explicit StoreData(PersistenceState::Enum persistenceState);
            StoreData(int64 operationLSN, PersistenceState::Enum persistenceState);

            __declspec (property(get=get_OperationLSN)) int64 OperationLSN;
            int64 get_OperationLSN() const { return operationLSN_; }

            __declspec (property(get=get_PersistenceState, put=set_PersistenceState)) PersistenceState::Enum PersistenceState;
            PersistenceState::Enum get_PersistenceState() const { return persistenceState_; }
            virtual void set_PersistenceState(PersistenceState::Enum value) { persistenceState_ = value; }

            virtual std::wstring const& GetStoreKey() const = 0;

            virtual void PostRead(::FABRIC_SEQUENCE_NUMBER operationLSN);
            virtual void PostCommit(::FABRIC_SEQUENCE_NUMBER operationLSN);

        private:

            int64 operationLSN_;
            PersistenceState::Enum persistenceState_;
        };
    }
}
