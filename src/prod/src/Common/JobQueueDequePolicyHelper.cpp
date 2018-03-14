// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

JobQueueDequePolicyHelper::JobQueueDequePolicyHelper(DequePolicy dequePolicy)
    : dequePolicy_(dequePolicy)
    , isFifo_(dequePolicy != DequePolicy::Lifo)
{
}

JobQueueDequePolicyHelper::~JobQueueDequePolicyHelper()
{
}

void JobQueueDequePolicyHelper::UpdateOnQueueFull()
{
    if (dequePolicy_ == DequePolicy::FifoLifo)
    {
        isFifo_ = false;
    }
}


void JobQueueDequePolicyHelper::UpdateOnQueueEmpty()
{
    if (dequePolicy_ == DequePolicy::FifoLifo)
    {
        isFifo_ = true; 
    }
}

bool JobQueueDequePolicyHelper::ShouldTakeFistItem() const
{
    return isFifo_;
}
