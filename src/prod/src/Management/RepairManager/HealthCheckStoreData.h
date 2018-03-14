// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace RepairManager
    {
        /// <summary>
        /// Structure for persisting repair task health check information in the replicated store.        
        /// </summary>
        class HealthCheckStoreData : public Store::StoreData
        {
        public:
            HealthCheckStoreData();

            HealthCheckStoreData(HealthCheckStoreData &&);

            HealthCheckStoreData const & operator = (HealthCheckStoreData const &);

            virtual ~HealthCheckStoreData();

            /// <summary>
            /// The last time the Service Fabric cluster was unhealthy.
            /// </summary>
            __declspec(property(get = get_LastErrorAt, put = put_LastErrorAt)) Common::DateTime & LastErrorAt;
            Common::DateTime & get_LastErrorAt() { return lastErrorAt_; }
            void put_LastErrorAt(__in Common::DateTime const & newTime);

            /// <summary>
            /// The last time the Service Fabric cluster was healthy.
            /// </summary>
            __declspec(property(get = get_LastHealthyAt, put = put_LastHealthyAt)) Common::DateTime & LastHealthyAt;
            Common::DateTime & get_LastHealthyAt() { return lastHealthyAt_; }
            void put_LastHealthyAt(__in Common::DateTime const & newTime);

            std::wstring const & get_Type() const override;
            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const override;

            FABRIC_FIELDS_02(lastHealthyAt_, lastErrorAt_);

        protected:
            std::wstring ConstructKey() const override;

        private:
            Common::DateTime lastErrorAt_;
            Common::DateTime lastHealthyAt_;
        };
    }
}
