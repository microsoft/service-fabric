// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    template<class TItem>
    class ReferenceArray
    {
    public:
        typedef TItem ItemType;

        explicit ReferenceArray() throw() : count_(0), array_(NULL) { }
        ReferenceArray(size_t count, __in TItem * array) throw() : count_(count), array_(array) { }

        ReferenceArray<TItem> const & operator = (ReferenceArray const & other) throw()
        {
            ASSERT_IFNOT(array_ == NULL, "Cannot initialize ReferenceArray twice");
            ASSERT_IFNOT(other.array_ != NULL, "Initializing from a null ReferenceArray");

            count_ = other.count_;
            array_ = other.array_;
            return *this;
        }

        ReferenceArray<TItem> const & Initialize(size_t count, TItem * array) throw()
        {
            ASSERT_IFNOT(array_ == NULL, "Cannot initialize ReferenceArray twice");
            ASSERT_IFNOT(array != NULL, "Initializing from a null ReferenceArray");

            count_ = count;
            array_ = array;
            return *this;
        }

        bool operator == (ReferenceArray const & other) const throw() { return array_ == other.array_; }
        bool operator != (ReferenceArray const & other) const throw() { return array_ != other.array_; }

        TItem & operator [] (size_t index)
        {
            ASSERT_IFNOT(index < count_, "Invalid index: index={0} count={1}", index, count_);
            ASSERT_IFNOT(array_ != NULL, "Indexing uninitialized ReferenceArray");
            return array_[index];
        }

        size_t GetCount() { return count_; }
        TItem * GetRawArray() { return array_; }

    private:
        size_t count_;
        TItem * array_;

    public: // intellisense ignores all members after a bool operator.
 		    OPERATOR_BOOL { return (array_ != NULL); }
    };
}
