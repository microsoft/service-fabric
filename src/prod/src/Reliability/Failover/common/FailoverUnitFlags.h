// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverUnitFlags
    {
        struct Flags
        {
        public:
            enum Enum
            {
                None = 0x00,
                Stateful = 0x01,
                Persisted = 0x02,
                NoData = 0x04,
                ToBeDeleted = 0x08,
                SwappingPrimary = 0x10,
                Orphaned = 0x20,
                Upgrading = 0x40
            };

            Flags(
                bool stateful = false,
                bool persisted = false,
                bool noData = false,
                bool toBeDeleted = false,
                bool swapPrimary = false,
                bool orphaned = false,
                bool upgrading = false);

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const;

            __declspec (property(get = get_Value)) int Value;
            int get_Value() const { return value_; }

            bool IsStateful() const { return ((value_ & Stateful) != 0); }

            bool HasPersistedState() const { return ((value_ & Persisted) != 0); }

            bool IsNoData() const { return ((value_ & NoData) != 0); }
            void SetNoData(bool noData) { noData ? value_ |= NoData : value_ &= (~NoData); }

            bool IsToBeDeleted() const { return ((value_ & ToBeDeleted) != 0); }
            void SetToBeDeleted() { value_ |= ToBeDeleted; }

            bool IsSwappingPrimary() const { return ((value_ & SwappingPrimary) != 0); }
            void SetSwappingPrimary(bool swappingPrimary) { swappingPrimary ? value_ |= SwappingPrimary : value_ &= (~SwappingPrimary); }

            bool IsOrphaned() const { return ((value_ & Orphaned) != 0); }
            void SetOrphaned() { value_ |= Orphaned; }

            bool IsUpgrading() const { return ((value_ & Upgrading) != 0); }
            void SetUpgrading(bool upgrading) { upgrading ? value_ |= Upgrading : value_ &= (~Upgrading); }

            FABRIC_PRIMITIVE_FIELDS_01(value_);

        private:

            int value_;
        };
    }
}
