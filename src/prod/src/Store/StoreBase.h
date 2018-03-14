// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class StoreBase;
    typedef std::shared_ptr<StoreBase> StoreBaseSPtr;
    typedef std::unique_ptr<StoreBase> StoreBaseUPtr;

    class StoreBase : public Common::FabricComponent
    {
        DENY_COPY(StoreBase);

    protected:
        StoreBase()
        {
        }

        virtual Common::ErrorCode OnOpen() override
        {
            return Common::ErrorCode::Success();
        }

        virtual Common::ErrorCode OnClose() override
        {
            return Common::ErrorCode::Success();
        }

        virtual void OnAbort() override
        {
        }
    };
}
