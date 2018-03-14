// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TestHooks
{
    class FaultInjector : public Common::TextTraceComponent<Common::TraceTaskCodes::TestSession>
    {
    public:

        static Common::GlobalWString ReplicatedStoreOpenTag;
        static Common::GlobalWString ReplicatedStoreApplyTag;
        static Common::GlobalWString FabricTest_BeginUpdateEpochTag;
        static Common::GlobalWString FabricTest_EndUpdateEpochTag;
        static Common::GlobalWString FabricTest_GetCopyStateTag;

    public:
        static FaultInjector & GetGlobalFaultInjector();

        FaultInjector();

        void InjectPermanentError(std::wstring const &, Common::ErrorCodeValue::Enum);
        void InjectPermanentError(std::wstring const &, Common::ErrorCodeValue::Enum, std::wstring const & service);
        void InjectPermanentError(std::wstring const &, Common::ErrorCodeValue::Enum, std::wstring const & nodeId, std::wstring const & service);

        void InjectTransientError(std::wstring const &, Common::ErrorCodeValue::Enum, Common::TimeSpan const expiration = Common::TimeSpan::Zero);

        void RemoveError(std::wstring const &);

        bool TryReadError(std::wstring const &, __out Common::ErrorCodeValue::Enum &);

        bool TryReadError(std::wstring const &, std::wstring const & nodeId, std::wstring const & service, __out Common::ErrorCodeValue::Enum &);

        bool TryReadError(std::wstring const &, TestHookContext const &, __out Common::ErrorCodeValue::Enum &);

    private:
        class ErrorEntry;

        void InjectError(
            std::wstring const &, 
            Common::ErrorCodeValue::Enum, 
            bool isTransient, 
            Common::TimeSpan const expiration,
            std::wstring const & nodeId,
            std::wstring const & serviceName);

        void InnerRemoveError(std::wstring const &);

        std::map<std::wstring, std::shared_ptr<ErrorEntry>> errors_;
        Common::RwLock errorsLock_;

    private:
        static BOOL CALLBACK InitFaultInjectorFunction(PINIT_ONCE, PVOID, PVOID *);
        static INIT_ONCE globalInitOnce_;
        static Common::Global<FaultInjector> globalFaultInjector_;
    };
}
