// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include <vector>

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class Placement;
        class NodeEntry;

        // Dynamic bit set that will be empty when created and will grow when elements are added.
        // Main usage is in block lists and for sets of nodes.
        // Difference from NodeSet: no placement is required, and it can be used for any integers and not only nodes.
        class DynamicBitSet
        {
            friend class NodeSet;
            DENY_COPY_ASSIGNMENT(DynamicBitSet);

        public:
            // initialize an empty or complete node set
            DynamicBitSet(size_t Count = 0);

            DynamicBitSet(DynamicBitSet const& other);

            DynamicBitSet(DynamicBitSet && other);

            DynamicBitSet(std::vector<int> const& set);

            DynamicBitSet & operator = (DynamicBitSet && other);

            __declspec (property(get=get_Count)) size_t Count;
            size_t get_Count() const { return elementCount_; }

            __declspec (property(get=get_IsEmpty)) bool IsEmpty;
            bool get_IsEmpty() const { return Count == 0; }

            // Adds a new element to the set.
            void Add(size_t element);

            void Delete(size_t element);

            bool Check(size_t element) const
            {
                if (static_cast<int64>(element) > maxElement_)
                {
                    return false;
                }
                return (bitmapPtr_[element / BitmapElementSize] & 1u << (element % BitmapElementSize)) != 0;
            }

            void Copy(DynamicBitSet const& Other);

            // Clear all elements from the set
            void Clear();

            void ForEach(std::function<void(size_t)> processor) const;

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        private:
            typedef uint32 BitmapElementType;
            const static int BitmapElementSize = sizeof(BitmapElementType) * 8;

            void InternalAdd(size_t element)
            {
                // Does not check if container has enough space!
                ++elementCount_;
                bitmapPtr_[element / BitmapElementSize] |= 1u << (element % BitmapElementSize);
            }

            void Grow(size_t count);

            void InnerClear();

            void InternalDelete(size_t element)
            {
                // Does not check if container has correct size!
                --elementCount_;
                bitmapPtr_[element / BitmapElementSize] &= ~(1u << (element % BitmapElementSize));
            }

            int GetOneCount(BitmapElementType x) const;

            size_t elementCount_;

            int64 maxElement_;
            int bitmapLength_;
            std::vector<BitmapElementType> bitmap_;
            BitmapElementType * bitmapPtr_;
        };
    }
}
