// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class StoreDataGlobalInstanceCounter : public Store::StoreData
        {
        public:
            StoreDataGlobalInstanceCounter();

            __declspec(property(get=get_Value)) uint64 Value;
            uint64 get_Value() const { return value_; }

            virtual ~StoreDataGlobalInstanceCounter() { }
            virtual std::wstring const & get_Type() const;
            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            Common::ErrorCode GetNextApplicationId(
                Store::StoreTransaction const &, 
                ServiceModelTypeName const &, 
                ServiceModelVersion const &, 
                __out ServiceModelApplicationId &);

            Common::ErrorCode UpdateNextApplicationId(
                Store::StoreTransaction const &);

            FABRIC_FIELDS_01(value_);
            
        protected:
            virtual std::wstring ConstructKey() const;

        private:
            uint64 value_;
        };
    }
}

