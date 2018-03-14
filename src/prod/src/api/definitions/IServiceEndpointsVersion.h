// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IServiceEndpointsVersion
    {
    public:
        virtual ~IServiceEndpointsVersion() {};

        virtual LONG Compare(IServiceEndpointsVersion const & other) const = 0; 

        virtual Reliability::GenerationNumber GetGeneration() const = 0; 

        virtual int64 GetVersion() const = 0; 
    };
}
