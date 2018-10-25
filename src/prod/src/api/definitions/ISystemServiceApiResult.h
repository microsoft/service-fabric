//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace Api
{
    class ISystemServiceApiResult
    {
    public:
        virtual ~ISystemServiceApiResult() {};

        virtual std::wstring const & GetResult() = 0;
    };
}
