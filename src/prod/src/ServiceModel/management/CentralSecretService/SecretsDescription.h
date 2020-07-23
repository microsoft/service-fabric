// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace CentralSecretService
    {
        class SecretsDescription : public ServiceModel::ClientServerMessageBody
        {
            DEFAULT_COPY_CONSTRUCTOR(SecretsDescription)
            DEFAULT_COPY_ASSIGNMENT(SecretsDescription)
            
        public:
            SecretsDescription();
            SecretsDescription(std::vector<Secret> const & secrets);

            __declspec(property(get = get_Secrets)) std::vector<Secret> const & Secrets;
            std::vector<Secret> const & get_Secrets() const { return secrets_; }

            Common::ErrorCode FromPublicApi(__in FABRIC_SECRET_LIST const * secretList);
            Common::ErrorCode ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_SECRET_LIST & secretList) const;
            Common::ErrorCode Validate() const;

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_CHAIN()
                SERIALIZABLE_PROPERTY(L"Secrets", secrets_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            FABRIC_FIELDS_01(
                secrets_)
        private:
            std::vector<Secret> secrets_;
        };
    }
}
