//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace Client
{
    class CallSystemServiceResult
        : public Api::ISystemServiceApiResult
        , public Common::ComponentRoot
    {
        DENY_COPY(CallSystemServiceResult);

    public:
        CallSystemServiceResult(
            __in std::wstring && result);

        std::wstring const & GetResult();

    private:
        std::wstring result_;
    };
}
