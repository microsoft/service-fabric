// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{ 
    template<class THandle>
    class HandleBase
    {
        DENY_COPY(HandleBase)

    protected:
        explicit HandleBase(THandle hValue, THandle invalidValue = NULL)
            : hValue_(hValue)
        {
            ASSERT_IF(hValue == invalidValue, "The specified handle value is invalid.");
        }

    public:
        virtual ~HandleBase()
        {
        }

        __declspec(property(get=get_Value)) THandle Value;
        THandle get_Value() const { return hValue_; }

    private:
        THandle hValue_;
    };
}
