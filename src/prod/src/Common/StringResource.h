// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
#define DECLARE_SINGLETON_RESOURCE( TComponent ) \
    public: \
        TComponent(); \
        virtual ~TComponent(); \
        static TComponent & GetResources(); \
        static void Test_Reset(); \
    private: \
        static BOOL CALLBACK InitSingleton(PINIT_ONCE, PVOID, PVOID *); \
        static INIT_ONCE InitOnce; \
        static Common::Global<TComponent> Singleton; \
        mutable Common::RwLock lock_; \

#define DEFINE_SINGLETON_RESOURCE( TComponent ) \
    INIT_ONCE TComponent::InitOnce; \
    Common::Global<TComponent> TComponent::Singleton; \
    TComponent::TComponent() { } \
    TComponent::~TComponent() { } \
    TComponent & TComponent::GetResources() \
    { \
        PVOID lpContext = NULL; \
        BOOL result = ::InitOnceExecuteOnce(&InitOnce, TComponent::InitSingleton, NULL, &lpContext); \
        ASSERT_IF(!result, "Failed to initialize {0} singleton", L ## #TComponent); \
        return *(Singleton); \
    } \
    BOOL CALLBACK TComponent::InitSingleton(PINIT_ONCE, PVOID, PVOID *) \
    { \
        Singleton = Common::make_global<TComponent>(); \
        return TRUE; \
    } \
    void TComponent::Test_Reset() \
    { \
        Singleton = Common::make_global<TComponent>(); \
    } \

#define DEFINE_STRING_RESOURCE( TComponent, TResource, ComponentName, ResourceName ) \
    public: \
        __declspec(property(get=get_##TResource)) std::wstring const & TResource; \
        std::wstring const & get_##TResource() const \
        { \
            { \
                Common::AcquireReadLock lock(lock_); \
                if (!TResource##_.empty()) { return TResource##_; } \
            } \
            Common::AcquireWriteLock lock(lock_); \
            if (TResource##_.empty()) { TResource##_ = Common::StringResource::Get(IDS_##ComponentName##_##ResourceName); } \
            return TResource##_; \
        } \
    private: \
        mutable std::wstring TResource##_; \

    struct StringResource
    {
        static std::wstring Get(uint);
    };
}
