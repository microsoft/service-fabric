// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    template <class TComInterface>
    class ComComponentRoot : public ComponentRoot
    {
        DENY_COPY_ASSIGNMENT(ComComponentRoot)

    public:
        ComComponentRoot(ComPointer<TComInterface> const & root)
            : root_(root)
        {
        }
    
        ComComponentRoot(ComPointer<TComInterface> && root)
            : root_(move(root))
        {
        }
    
    private:
        ComPointer<TComInterface> const root_;
    };
}
