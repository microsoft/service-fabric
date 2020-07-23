// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

using namespace Reliability::FailoverManagerComponent;

DEFINE_USER_ARRAY_UTILITY(Management::NetworkInventoryManager::NetworkDefinition);

namespace Management
{
    namespace NetworkInventoryManager
    {
        class NIMNetworkDefinitionStoreData : public StoreData
        {
        public:
            NIMNetworkDefinitionStoreData();

            NIMNetworkDefinitionStoreData(NetworkDefinitionSPtr networkDefinition);

            static std::wstring const& GetStoreType();
            virtual std::wstring const& GetStoreKey() const;

            // Id for the node allocation store key.
            __declspec(property(get = get_Id, put = set_Id)) std::wstring Id;
            std::wstring get_Id() const { return this->id_; };
            void set_Id(const std::wstring & value) { this->id_ = value; }

            __declspec(property(get = get_NetworkDefinition, put = set_NetworkDefinition)) NetworkDefinitionSPtr NetworkDefinition;
            NetworkDefinitionSPtr get_NetworkDefinition() const { return this->networkDefinition_; };
            void set_NetworkDefinition(NetworkDefinitionSPtr & value) { this->networkDefinition_ = value; }

            __declspec(property(get=get_SequenceNumber, put = set_SequenceNumber)) int64 SequenceNumber;
            int64 get_SequenceNumber() const { return sequenceNumber_; }
            void set_SequenceNumber(int64 value) { this->sequenceNumber_ = value; }

            __declspec(property(get = get_AssociatedApplications, put = set_AssociatedApplications)) Common::StringCollection & AssociatedApplications;
            Common::StringCollection & get_AssociatedApplications() { return this->associatedApplications_; };

            FABRIC_FIELDS_05(id_, lastUpdateTime_, networkDefinition_, sequenceNumber_, associatedApplications_);

            void PostUpdate(Common::DateTime updatedTime);

            void PostRead(int64 operationLSN)
            {
                operationLSN = 0;
                StoreData::PostRead(operationLSN);
            }

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        private:
            std::wstring id_;
            Common::DateTime lastUpdateTime_;
            int64 sequenceNumber_;

            NetworkDefinitionSPtr networkDefinition_;
            Common::StringCollection associatedApplications_;
        };

        typedef shared_ptr<NIMNetworkDefinitionStoreData> NIMNetworkDefinitionStoreDataSPtr;
    }
}


