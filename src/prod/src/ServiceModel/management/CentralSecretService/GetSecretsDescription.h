// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace CentralSecretService
    {
        class GetSecretsDescription
            : public SecretReferencesDescription
        {
        public:
            GetSecretsDescription();
            GetSecretsDescription(std::vector<SecretReference> const & secretReferences, bool includeValue);
            ~GetSecretsDescription();

            __declspec(property(get = get_IncludeValue)) bool const & IncludeValue;
            bool const & get_IncludeValue() const { return includeValue_; }

            Common::ErrorCode FromPublicApi(__in FABRIC_SECRET_REFERENCE_LIST const * secretReferenceList, __in BOOLEAN includeValue);
            Common::ErrorCode ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_SECRET_REFERENCE_LIST & secretReferenceList, __out BOOLEAN & includeValue) const;
            
            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_CHAIN()
                SERIALIZABLE_PROPERTY(L"IncludeValue", includeValue_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            FABRIC_FIELDS_01(
                includeValue_)
        private:
            bool includeValue_;
        };
    }
}