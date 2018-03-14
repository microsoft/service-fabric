// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class StoreDataVerifiedUpgradeDomains : public Store::StoreData
        {
        public:
            StoreDataVerifiedUpgradeDomains();
            explicit StoreDataVerifiedUpgradeDomains(ServiceModelApplicationId const &);
            StoreDataVerifiedUpgradeDomains(ServiceModelApplicationId const &, std::vector<std::wstring> &&);

            __declspec(property(get=get_ApplicationId)) ServiceModelApplicationId const & ApplicationId;
            ServiceModelApplicationId const & get_ApplicationId() const { return applicationId_; }

            __declspec(property(get=get_UpgradeDomains)) std::vector<std::wstring> & UpgradeDomains;
            std::vector<std::wstring> & get_UpgradeDomains() { return upgradeDomains_; }

            void Clear() { upgradeDomains_.clear(); }

            virtual ~StoreDataVerifiedUpgradeDomains() { }
            virtual std::wstring const & get_Type() const;
            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            FABRIC_FIELDS_02(applicationId_, upgradeDomains_);

        protected:
            virtual std::wstring ConstructKey() const;

        private:
            ServiceModelApplicationId applicationId_;
            std::vector<std::wstring> upgradeDomains_;
        };
    }
}
