// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class FabricComponent
    {
        DENY_COPY(FabricComponent)

    public:
        __declspec(property(get=get_FabricComponentState)) FabricComponentState State;
        inline FabricComponentState const & get_FabricComponentState() const { return state_; }

        bool IsOpened();

        Common::ErrorCode Open();
        
        Common::ErrorCode Close();

        void Abort();
        bool TryAbort();

        virtual ~FabricComponent(void);

    protected:
        FabricComponent(void);

        virtual Common::ErrorCode OnOpen() = 0;

        virtual Common::ErrorCode OnClose() = 0;

        virtual void OnAbort() = 0;

        void ThrowIfCreatedOrOpening();

        bool IsOpeningOrOpened();

        Common::RwLock & get_StateLock();

    private:
        FabricComponentState state_;
    };
}
