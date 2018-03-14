// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class StoreDataApplicationIdMap : public Store::StoreData
        {
        public:
            StoreDataApplicationIdMap(ServiceModelApplicationId const &);

            StoreDataApplicationIdMap(ServiceModelApplicationId const &, Common::NamingUri const &);

            __declspec(property(get=get_AppId)) ServiceModelApplicationId const & ApplicationId;
            ServiceModelApplicationId const & get_AppId() const { return appId_; }

            __declspec(property(get=get_Name)) Common::NamingUri const & ApplicationName;
            Common::NamingUri const & get_Name() const { return appName_; }

            virtual ~StoreDataApplicationIdMap() { }
            virtual std::wstring const & get_Type() const;
            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            FABRIC_FIELDS_02(appId_, appName_);

        protected:
            virtual std::wstring ConstructKey() const;

        private:
            ServiceModelApplicationId appId_;
            Common::NamingUri appName_;
        };
    }
}
