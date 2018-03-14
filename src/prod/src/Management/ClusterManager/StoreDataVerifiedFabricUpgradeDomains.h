// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class StoreDataVerifiedFabricUpgradeDomains : public Store::StoreData
        {
        public:
            StoreDataVerifiedFabricUpgradeDomains();
            StoreDataVerifiedFabricUpgradeDomains(std::vector<std::wstring> &&);

            __declspec(property(get=get_UpgradeDomains)) std::vector<std::wstring> & UpgradeDomains;
            std::vector<std::wstring> & get_UpgradeDomains() { return upgradeDomains_; }

            void Clear() { upgradeDomains_.clear(); }

            virtual ~StoreDataVerifiedFabricUpgradeDomains() { }
            virtual std::wstring const & get_Type() const;
            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            FABRIC_FIELDS_01(upgradeDomains_);

        protected:
            virtual std::wstring ConstructKey() const;

        private:
            std::vector<std::wstring> upgradeDomains_;
        };
    }
}
