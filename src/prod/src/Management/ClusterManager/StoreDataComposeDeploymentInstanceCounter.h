// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class StoreDataComposeDeploymentInstanceCounter : public Store::StoreData
        {
        public:
            StoreDataComposeDeploymentInstanceCounter();

            __declspec(property(get=get_Value)) uint64 Value;
            uint64 get_Value() const { return value_; }

            virtual ~StoreDataComposeDeploymentInstanceCounter() { }
            virtual std::wstring const & get_Type() const;
            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            Common::ErrorCode GetTypeNameAndVersion(
                Store::StoreTransaction const &,
                __out ServiceModelTypeName&,
                __out ServiceModelVersion &);

            static Common::ErrorCode GetVersionNumber(ServiceModelVersion const &version, uint64 *versionNumber);
            static ServiceModelVersion GenerateServiceModelVersion(uint64 number);

            FABRIC_FIELDS_01(value_);

        protected:
            virtual std::wstring ConstructKey() const;

        private:

            uint64 value_;
        };
    }
}


