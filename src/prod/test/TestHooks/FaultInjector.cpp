// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

namespace TestHooks
{
    StringLiteral const TraceComponent("FaultInjector");

    GlobalWString FaultInjector::ReplicatedStoreOpenTag = make_global<wstring>(L"REStore.Open");
    GlobalWString FaultInjector::ReplicatedStoreApplyTag = make_global<wstring>(L"REStore.Apply");
    GlobalWString FaultInjector::FabricTest_BeginUpdateEpochTag = make_global<wstring>(L"provider.beginupdateepoch");
    GlobalWString FaultInjector::FabricTest_EndUpdateEpochTag = make_global<wstring>(L"provider.endupdateepoch");
    GlobalWString FaultInjector::FabricTest_GetCopyStateTag = make_global<wstring>(L"provider.getcopystate");

    INIT_ONCE FaultInjector::globalInitOnce_ = INIT_ONCE_STATIC_INIT;
    Global<FaultInjector> FaultInjector::globalFaultInjector_;

    class FaultInjector::ErrorEntry
    {
    public:
        ErrorEntry(
            ErrorCodeValue::Enum error, 
            bool isTransient, 
            TimeSpan const expiration,
            wstring const & nodeId,
            wstring const & service)
            : error_(error)
            , isTransient_(isTransient)
            , expiration_(DateTime::Now() + expiration)
            , nodeId_(nodeId)
            , service_(service)
        {
        }

        __declspec(property(get=get_Error)) ErrorCodeValue::Enum Error;
        __declspec(property(get=get_IsTransient)) bool IsTransient;
        __declspec(property(get=get_IsExpired)) bool IsExpired;

        ErrorCodeValue::Enum get_Error() const { return error_; }
        bool get_IsTransient() const { return isTransient_; }
        bool get_IsExpired() const { return (DateTime::Now() > expiration_); }

        bool Match(wstring const & nodeId, wstring const & service)
        {
            if (!nodeId_.empty() && 
                !nodeId.empty() && 
                nodeId_ != nodeId)
            {
                return false;
            }

            if (!service_.empty() && 
                !service.empty() &&
                service_ != service)
            {
                return false;
            }

            return true;
        }

        void WriteTo(__in TextWriter & w, FormatOptions const &) const
        {
            w.Write("[error={0} transient={1} exp={2} node='{3}' service='{4}']",
                error_,
                isTransient_,
                expiration_,
                nodeId_,
                service_);
        }

    private:
        ErrorCodeValue::Enum error_;
        bool isTransient_;
        DateTime expiration_;

        wstring nodeId_;
        wstring service_;
    };

    FaultInjector::FaultInjector()
        : errors_()
        , errorsLock_()
    {
    }

    FaultInjector & FaultInjector::GetGlobalFaultInjector()
    {
        PVOID lpContext = NULL;
        BOOL result = ::InitOnceExecuteOnce(
            &globalInitOnce_, 
            InitFaultInjectorFunction,
            NULL,
            &lpContext);
        ASSERT_IF(!result, "Failed to initialize global FaultInjector");

        return *globalFaultInjector_;
    }

    void FaultInjector::InjectPermanentError(
        wstring const & key,
        ErrorCodeValue::Enum error)
    {
        InjectError(key, error, false, TimeSpan::Zero, L"", L"");
    }

    void FaultInjector::InjectPermanentError(
        wstring const & key,
        ErrorCodeValue::Enum error,
        wstring const & service)
    {
        InjectError(key, error, false, TimeSpan::Zero, L"", service);
    }

    void FaultInjector::InjectPermanentError(
        wstring const & key,
        ErrorCodeValue::Enum error,
        wstring const & nodeId,
        wstring const & service)
    {
        InjectError(key, error, false, TimeSpan::Zero, nodeId, service);
    }

    void FaultInjector::InjectTransientError(
        wstring const & key,
        ErrorCodeValue::Enum error,
        TimeSpan const expiration)
    {
        InjectError(key, error, true, expiration, L"", L"");
    }

    void FaultInjector::InjectError(
        wstring const & key,
        ErrorCodeValue::Enum error,
        bool isTransient,
        TimeSpan const expiration,
        wstring const & nodeId,
        wstring const & serviceName)
    {
        AcquireWriteLock lock(errorsLock_);

        auto entry = make_shared<ErrorEntry>(error, isTransient, expiration, nodeId, serviceName);

        WriteWarning(
            TraceComponent,
            "Injecting fault: tag='{0}' fault='{1}'",
            key,
            *entry);

        errors_.erase(key);
        errors_.insert(make_pair(key, move(entry)));
    }

    void FaultInjector::RemoveError(
        wstring const & key)
    {
        AcquireWriteLock lock(errorsLock_);

        InnerRemoveError(key);
    }

    void FaultInjector::InnerRemoveError(
        wstring const & key)
    {
        errors_.erase(key);

        WriteInfo(
            TraceComponent,
            "Removed fault: tag='{0}'",
            key);
    }

    bool FaultInjector::TryReadError(
        wstring const & key,
        __out ErrorCodeValue::Enum & error)
    {
        return this->TryReadError(key, L"", L"", error);
    }

    bool FaultInjector::TryReadError(
        wstring const & key,
        TestHookContext const & testHookContext,
        __out ErrorCodeValue::Enum & error)
    {
        return this->TryReadError(
            key, 
            testHookContext.NodeId.ToString(),
            testHookContext.ServiceName,
            error);
    }

    bool FaultInjector::TryReadError(
        wstring const & key,
        wstring const & nodeId,
        wstring const & service,
        __out ErrorCodeValue::Enum & error)
    {
        bool shouldRemove = false;
        {
            AcquireReadLock lock(errorsLock_);

            auto findIt = errors_.find(key);
            if (findIt == errors_.end())
            {
                WriteNoise(
                    TraceComponent,
                    "No faults found: tag='{0}'",
                    key);
                
                return false;
            }

            auto const & entry = findIt->second;

            if (!entry->Match(nodeId, service))
            {
                WriteNoise(
                    TraceComponent,
                    "No faults found: tag='{0}' nodeId='{1}' service='{2}' entry='{3}'",
                    key,
                    nodeId,
                    service,
                    *entry);
                
                return false;
            }

            error = entry->Error;

            WriteWarning(
                TraceComponent,
                "Read fault: tag='{0}' entry='{1}'",
                key,
                *entry);

            if (entry->IsTransient && entry->IsExpired)
            {
                shouldRemove = true;
            }
        }

        if (shouldRemove)
        {
            AcquireWriteLock lock(errorsLock_);

            auto findIt = errors_.find(key);

            if (findIt != errors_.end())
            {
                InnerRemoveError(key);
            }
        }

        return true;
    }

    BOOL CALLBACK FaultInjector::InitFaultInjectorFunction(PINIT_ONCE, PVOID, PVOID *)
    {
        globalFaultInjector_ = make_global<FaultInjector>();

        return TRUE;
    }
}
