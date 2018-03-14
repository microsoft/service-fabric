// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        // A set which stores at most K elements
        // K=-1 means the set has an unlimited capacity
        template<typename T>
        class BoundedSet
        {
            DENY_COPY(BoundedSet);

        public:
            explicit BoundedSet(int maxElements)
                : maxElements_(maxElements)
            {
                ASSERT_IFNOT(maxElements == -1 || maxElements > 0, "Invalid maxElements {0}", maxElements);
            }

            BoundedSet(BoundedSet && other)
                : maxElements_(other.maxElements_), elements_(move(other.elements_))
            {
            }

            void Add(T const& element)
            {
                if (CanAdd())
                {
                    elements_.push_back(element);
                }
            }

            template<typename Iterator>
            void Add(Iterator begin, Iterator end)
            {
                while (CanAdd() && begin < end)
                {
                    elements_.push_back(*begin);
                    ++begin;
                }
            }

            size_t Size() const
            {
                return elements_.size();
            }

            bool IsEmpty() const
            {
                return elements_.empty();
            }

            bool CanAdd() const
            {
                return maxElements_ == -1 || static_cast<int>(elements_.size()) < maxElements_;
            }

            void Clear()
            {
                elements_.clear();
            }

            typename std::vector<T> const& GetElements() const
            {
                return elements_;
            }

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const&) const;

            void WriteToEtw(uint16 contextSequenceId) const;

        private:
            template<typename U>
            static U const& Dereference(U const& input)
            {
                return input;
            }

            template<typename U>
            static U const& Dereference(U const* input)
            {
                return *input;
            }

            template<typename U>
            static U & Dereference(U * input)
            {
                return *input;
            }

            int maxElements_;
            typename std::vector<T> elements_;
        };
    }
}
