// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class ISizeEstimator
    {
    public:
        virtual ~ISizeEstimator() {}
        virtual size_t EstimateDynamicSerializationSize() const = 0;
        virtual size_t EstimateSize() const = 0;
    };
}
