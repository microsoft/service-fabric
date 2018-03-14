// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class RootedObject
    {
        DENY_COPY_ASSIGNMENT(RootedObject)

    public:
        explicit RootedObject(ComponentRoot const & root) : root_(root) {}
        virtual ~RootedObject() = 0 {}

        __declspec (property(get=get_Root)) ComponentRoot const & Root;
        ComponentRoot const & get_Root() const { return root_; }

    private:
        ComponentRoot const & root_;
    };
}
