// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    template <class TRootedObject>
    class RootedObjectPointer
    {
    public:
        RootedObjectPointer() throw()
            : p_(NULL), 
            root_()

        {
        }

        RootedObjectPointer(TRootedObject * p, ComponentRootSPtr const & root)
            : p_(p),
            root_(root)
        {
        }
        

        RootedObjectPointer(RootedObjectPointer const & other) throw()
            : p_(other.p_),
            root_(other.root_)
        {
        }

        RootedObjectPointer(RootedObjectPointer && other) throw()
            : p_(other.p_),
            root_(std::move(other.root_))
        {
            other.p_ = NULL;
        }

        TRootedObject * get() const { return p_; }
        
        ComponentRootSPtr const & get_Root() const { return root_; }

        bool operator == (RootedObjectPointer const & other) const throw() 
        { 
            return p_ == other.p_; 
        }

        bool operator != (RootedObjectPointer const & other) const throw() 
        { 
            return p_ != other.p_; 
        }

        TRootedObject * operator -> () const throw() 
        { 
            return p_; 
        }

        RootedObjectPointer const & operator = (RootedObjectPointer const & other) throw()
        {
            if (p_ != other.p_)
            {
                p_ = other.p_;
                root_ = other.root_;
            }

            return *this;
        }

        RootedObjectPointer const & operator = (RootedObjectPointer && other) throw()
        {
            if (p_ != other.p_)
            {
                p_ = other.p_;
                other.p_ = NULL;
                root_ = std::move(other.root_);
            }

            return *this;
        }

    private:
        TRootedObject * p_;
        ComponentRootSPtr root_;
    };
}

