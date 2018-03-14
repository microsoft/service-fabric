// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    template <class TRootedObject>
    class RootedObjectWeakHolder
    {
        DENY_COPY_ASSIGNMENT(RootedObjectWeakHolder)

    public:
        RootedObjectWeakHolder(
            RootedObjectHolder<TRootedObject> const & rootedObjectHolder)
            : root_(rootedObjectHolder.Root),
            rootedObject_(rootedObjectHolder.RootedObject)
        {
        }

        RootedObjectWeakHolder(
            __in TRootedObject & rootedObject, 
            ComponentRootSPtr const & root) 
            : root_(root),
            rootedObject_(rootedObject)
        {
        }

        RootedObjectWeakHolder(
            RootedObjectWeakHolder const & other)
            : root_(other.root_),
            rootedObject_(other.rootedObject_)
        {
        }

        RootedObjectWeakHolder(
            RootedObjectWeakHolder && other)
            : root_(move(other.root_)),
            rootedObject_(other.rootedObject_)
        {
        }

        virtual ~RootedObjectWeakHolder() 
        {
        }

        std::unique_ptr<RootedObjectHolder<TRootedObject>> lock() const
        {
            auto root = root_.lock();
            if (root)
            {
                return make_unique<RootedObjectHolder<TRootedObject>>(rootedObject_, root);
            }
            else
            {
                return nullptr;
            }
        }

    private:
        ComponentRootWPtr const root_;
        TRootedObject & rootedObject_;
    };
}
