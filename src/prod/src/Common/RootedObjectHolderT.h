// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    template <class TRootedObject>
    class RootedObjectHolder
    {
        DENY_COPY_ASSIGNMENT(RootedObjectHolder)

    public:
        RootedObjectHolder(
            __in TRootedObject & rootedObject, 
            ComponentRootSPtr const & root) 
            : root_(root),
            rootedObject_(rootedObject)
        {
        }

        RootedObjectHolder(
            RootedObjectHolder const & other)
            : root_(other.root_),
            rootedObject_(other.rootedObject_)
        {
        }

        RootedObjectHolder(
            RootedObjectHolder && other)
            : root_(move(other.root_)),
            rootedObject_(other.rootedObject_)
        {
        }

        virtual ~RootedObjectHolder() 
        {
        }

        __declspec (property(get=get_RootedObject)) TRootedObject & RootedObject;
        __declspec (property(get=get_RootedObject)) TRootedObject & Value;
        TRootedObject & get_RootedObject() const { return rootedObject_; }

        __declspec (property(get=get_Root)) ComponentRootSPtr const & Root;
         ComponentRootSPtr const & get_Root() const { return root_; }

    private:
        ComponentRootSPtr const root_;
        TRootedObject & rootedObject_;
    };
}
