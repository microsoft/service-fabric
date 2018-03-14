// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define DECLARE_SINGLETON_CONFIG(TConfig)                                                                               \
    public:                                                                                                             \
        static TConfig & GetConfig();                                                                                   \
        static void Test_Reset();                                                                                     \
        virtual ~TConfig();                                                                                             \
    private:                                                                                                            \
        static BOOL CALLBACK InitConfigFunction(PINIT_ONCE, PVOID, PVOID *);                                            \
        static INIT_ONCE initOnceConfig_;                                                                               \
        static std::unique_ptr<TConfig> singletonConfig_;                                                               \
        TConfig();

// Implement singleton pattern using Windows API InitOnceExecuteOnce
#define DEFINE_SINGLETON_CONFIG(TConfig)                                                                                \
    INIT_ONCE TConfig::initOnceConfig_;                                                                                 \
    std::unique_ptr<TConfig> TConfig::singletonConfig_;                                                                 \
                                                                                                                        \
    BOOL CALLBACK TConfig::InitConfigFunction(PINIT_ONCE, PVOID, PVOID *)                                               \
    {                                                                                                                   \
        singletonConfig_ = std::unique_ptr<TConfig>(new TConfig());                                                     \
        return TRUE;                                                                                                    \
    }                                                                                                                   \
                                                                                                                        \
    TConfig & TConfig::GetConfig()                                                                                      \
    {                                                                                                                   \
        PVOID lpContext = NULL;                                                                                         \
        BOOL  bStatus = FALSE;                                                                                          \
                                                                                                                        \
        bStatus = ::InitOnceExecuteOnce(                                                                                \
            &TConfig::initOnceConfig_,                                                                                  \
            TConfig::InitConfigFunction,                                                                                \
            NULL,                                                                                                       \
            &lpContext);                                                                                                \
                                                                                                                        \
        ASSERT_IF(!bStatus, "Failed to initialize {0} singleton", L#TConfig);                       \
        return *TConfig::singletonConfig_;                                                                              \
    }                                                                                                                   \
                                                                                                                        \
    /* This should only be called from a test */                                                                        \
    void TConfig::Test_Reset()                                                                                        \
    {                                                                                                                   \
        TConfig::singletonConfig_ = std::unique_ptr<TConfig>(new TConfig());                                            \
    }
