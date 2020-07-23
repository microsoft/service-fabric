// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace CentralSecretService
    {
        class SecretReferencesDescription : public ServiceModel::ClientServerMessageBody
        {
            DEFAULT_COPY_CONSTRUCTOR(SecretReferencesDescription)
            DEFAULT_COPY_ASSIGNMENT(SecretReferencesDescription)

        public:

            SecretReferencesDescription();
            SecretReferencesDescription(std::vector<SecretReference> const & references);

            __declspec(property(get = get_SecretReferences)) std::vector<SecretReference> const & SecretReferences;
            std::vector<SecretReference> const & get_SecretReferences() const { return secretReferences_; }

            Common::ErrorCode FromPublicApi(__in FABRIC_SECRET_REFERENCE_LIST const * referenceList);
            Common::ErrorCode ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_SECRET_REFERENCE_LIST & referenceList) const;
            Common::ErrorCode Validate() const;

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_CHAIN()
                SERIALIZABLE_PROPERTY(L"SecretReferences", secretReferences_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            FABRIC_FIELDS_01(
                secretReferences_)

        private:
            std::vector<SecretReference> secretReferences_;
        };
    }
}