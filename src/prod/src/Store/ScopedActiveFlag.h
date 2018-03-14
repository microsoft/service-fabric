// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class ScopedActiveFlag
    {
        DENY_COPY(ScopedActiveFlag);

    private:
        ScopedActiveFlag(__in Common::atomic_bool & activeFlag)
            : isFirst_(!activeFlag.exchange(true))
            , activeFlag_(activeFlag)
        {
        }

    public:

        virtual ~ScopedActiveFlag()
        {
            if (isFirst_)
            {
                activeFlag_.store(false);
            }
        }

        static bool TryAcquire(__in Common::atomic_bool & activeFlag, __out std::shared_ptr<ScopedActiveFlag> & scopedFlag)
        {
            scopedFlag = std::shared_ptr<ScopedActiveFlag>(new ScopedActiveFlag(activeFlag));

            return scopedFlag->isFirst_;
        }

    private:
        bool isFirst_;
        Common::atomic_bool & activeFlag_;
    };
}
