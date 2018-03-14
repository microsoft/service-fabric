// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    template<class TItem>
    class ReferencePointer
    {
    public:
        typedef TItem ItemType;

        explicit ReferencePointer() throw() : p_(NULL) { }
        explicit ReferencePointer(__in TItem * item) throw() : p_(item) { }

        ReferencePointer<TItem> const & operator = (ReferencePointer const & other) throw()
        {
            ASSERT_IFNOT(p_ == NULL, "Cannot initialize ReferencePointer twice");
            ASSERT_IFNOT(other.p_ != NULL, "Initializing from a null ReferencePointer");

            p_ = other.p_;
            return *this;
        }

        ReferencePointer<TItem> const & operator = (TItem * item) throw()
        {
            ASSERT_IFNOT(p_ == NULL, "Cannot initialize ReferencePointer twice");
            ASSERT_IFNOT(item != NULL, "Initializing from a null ReferencePointer");

            p_ = item;
            return *this;
        }

        bool operator == (ReferencePointer const & other) const throw() { return p_ == other.p_; }
        bool operator != (ReferencePointer const & other) const throw() { return p_ != other.p_; }

        TItem * operator -> () const throw()
        {
            ASSERT_IFNOT(p_ != NULL, "Dereferncing uninitialized ReferencePointer");
            return p_;
        }

        TItem & operator * () const throw()
        {
            ASSERT_IFNOT(p_ != NULL, "Dereferncing uninitialized ReferencePointer");
            return *p_;
        }

        void Reset()
        {
            p_ = NULL;
        }

        TItem * GetRawPointer() { return p_; }

    private:
        TItem * p_;

    public: // intellisense ignores all members after a bool operator.
        OPERATOR_BOOL { return (p_ != NULL); }
    };
}
