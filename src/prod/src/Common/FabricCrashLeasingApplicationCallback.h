//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once
#include "Common/RwLock.h"

namespace Common
{
    // Encapsulates the leasing application callback
    class FabricCrashLeasingApplicationCallback
    {        
    public:
        typedef void(*CallbackPtr)(void);

        FabricCrashLeasingApplicationCallback(CallbackPtr callback = nullptr) :
            callback_(callback)
        {
        }

        void Set(CallbackPtr callback)
        {
            AcquireExclusiveLock grab(lock_);
            callback_ = callback;
        }

        void Invoke()
        {
            CallbackPtr inner = nullptr;

            {
                Common::AcquireExclusiveLock grab(lock_);
                inner = callback_;
            }

            if (inner != nullptr)
            {
                inner();
            }            
        }

    private:
        Common::ExclusiveLock lock_;
        CallbackPtr callback_;
    };
}