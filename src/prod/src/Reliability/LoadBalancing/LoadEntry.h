// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class LoadEntry
        {
            DENY_COPY_ASSIGNMENT(LoadEntry);

        public:
            LoadEntry();

            explicit LoadEntry(size_t metricCount, int64 value = 0);

            explicit LoadEntry(std::vector<int64> && values);

            LoadEntry(LoadEntry const& other);

            LoadEntry(LoadEntry && other);

            LoadEntry & operator = (LoadEntry && other);

            LoadEntry & operator +=(LoadEntry const & other);
            LoadEntry & operator -=(LoadEntry const & other);

            bool operator >= (LoadEntry const & other ) const;
            bool operator <= (LoadEntry const & other ) const;
            bool operator == (LoadEntry const & other ) const;
            bool operator != (LoadEntry const & other ) const;

            __declspec (property(get=get_Length)) size_t Length;
            size_t get_Length() const { return values_.size(); }

            __declspec (property(get=get_Values)) std::vector<int64> const& Values;
            std::vector<int64> const& get_Values() const { return values_; }

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const&) const;
            std::wstring ToString() const;

            void AddLoad(size_t index, int64 diff)
            {
                ASSERT_IFNOT(index < values_.size(), "Metric index {0} out of bound {1}", index, values_.size());
                AddLoadValue(values_[index], diff, index);
            }

            int64 TryAddLoad(size_t index, int64 diff) const
            {
                ASSERT_IFNOT(index < values_.size(), "Metric index {0} out of bound {1}", index, values_.size());
                int64 load = values_[index];
                AddLoadValue(load, diff, index);
                return load;
            }

            void Set(size_t index, int64 value)
            {
                ASSERT_IFNOT(index < values_.size(), "Metric index {0} out of bound {1}", index, values_.size());
                values_[index] = value;
            }

            void clear()
            {
                ASSERT_IF(values_.empty(), "LoadEntry value is empty");
                memset(values_.data(), 0, values_.size());
            }

            void Resize(size_t metricCount)
            {
                ASSERT_IF(!values_.empty(), "LoadEntry value should be empty before Resize");
                values_.resize(metricCount);
                clear();
            }

        private:
            static void AddLoadValue(int64 & lhs, int64 rhs, size_t index)
            {
                // check for int64 arithmetic overflows
                if (lhs >= 0 && rhs >= 0)
                {
                    ASSERT_IF(lhs + rhs < 0, "Load at index {0} overflows with {1} + {2}", index, lhs, rhs);
                }
                else if (lhs < 0 && rhs < 0)
                {
                    ASSERT_IF(lhs + rhs >= 0, "Load at index {0} overflows with {1} + {2}", index, lhs, rhs);
                }

                lhs += rhs;
            }

            std::vector<int64> values_;
        };
    }
}
