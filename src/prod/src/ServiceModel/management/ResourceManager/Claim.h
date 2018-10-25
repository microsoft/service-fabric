// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ResourceManager
    {
        class Claim : public ServiceModel::ClientServerMessageBody
        {
            DEFAULT_COPY_CONSTRUCTOR(Claim)
                DEFAULT_COPY_ASSIGNMENT(Claim)
        public:

            Claim() = default;
            Claim(ResourceIdentifier const & resourceId, std::vector<ResourceIdentifier> const & consumerIds);

            __declspec (property(get = get_ResourceId)) ResourceIdentifier const & ResourceId;
            ResourceIdentifier const & get_ResourceId() const { return this->resourceId_; }

            __declspec (property(get = get_ConsumerIds)) std::vector<ResourceIdentifier> const & ConsumerIds;
            std::vector<ResourceIdentifier> const & get_ConsumerIds() const { return this->consumerIds_; }

            bool IsEmpty() const;

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_CHAIN()
                SERIALIZABLE_PROPERTY(L"ResourceId", resourceId_)
                SERIALIZABLE_PROPERTY(L"ConsumerIds", consumerIds_)
                END_JSON_SERIALIZABLE_PROPERTIES()

            FABRIC_FIELDS_02(
                resourceId_,
                consumerIds_)

        private:
            ResourceIdentifier resourceId_;
            std::vector<ResourceIdentifier> consumerIds_;
        };
    }
}