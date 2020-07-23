// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"
#include <memory>

#define DENY_COPY_ASSIGNMENT( class_name ) private: class_name& operator = ( class_name const& );
#define DENY_COPY_CONSTRUCTOR( class_name ) private: class_name( class_name const& );

#define DENY_COPY( class_name ) DENY_COPY_ASSIGNMENT( class_name ); DENY_COPY_CONSTRUCTOR( class_name )

namespace TracePoints
{
    template <class T, class t0>
    std::unique_ptr<T> make_unique(t0 && a0)
    {
        return std::unique_ptr<T>(new T(std::forward<t0>(a0)));
    }
}
