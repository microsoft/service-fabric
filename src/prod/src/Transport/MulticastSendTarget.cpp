// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Transport
{
    MulticastSendTarget::~MulticastSendTarget()
    {
    }

    ISendTarget::SPtr const & MulticastSendTarget::operator[](size_t index)
    {
        return targets_[index];
    }

    size_t MulticastSendTarget::size() const
    {
        return targets_.size();
    }
}
