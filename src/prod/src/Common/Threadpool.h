// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common 
{
    typedef std::function<void(void)> ThreadpoolCallback; 

    class Threadpool
    {
        public:
            Threadpool();
            ~Threadpool();

            static void Post(ThreadpoolCallback const &);
            static void Post(ThreadpoolCallback const &, Common::TimeSpan delay, bool allowSyncCall = false);

            static Common::Global<Threadpool> CreateCustomPool(
                DWORD minThread = 0 /*0 means using default*/,
                DWORD maxThread = 0 /*0 means using default*/);

            void Submit(ThreadpoolCallback const &);
            void Submit(ThreadpoolCallback const &, Common::TimeSpan delay, bool allowSyncCall = false);

            bool SetThreadMin(DWORD min);
            void SetThreadMax(DWORD max);

            PTP_POOL PoolPtr();
            PTP_CALLBACK_ENVIRON CallbackEnvPtr();

            __forceinline static volatile LONG & ActiveCallbackCount() { return activeCallbackCount_; }

#ifndef PLATFORM_UNIX
            template<class T>
            static void TimerCallback(PTP_CALLBACK_INSTANCE pci, void* callbackParameter, PTP_TIMER pTpTimer)
            {
                BEGIN_THREADPOOL_CALLBACK()
                    T::Callback(pci, callbackParameter, pTpTimer);
                END_THREADPOOL_CALLBACK()
            }

            template<class T>
            static void WaitCallback(
                PTP_CALLBACK_INSTANCE pci, 
                void* callbackParameter, 
                PTP_WAIT pTpWait,
                TP_WAIT_RESULT tpWaitResult)
            {
                BEGIN_THREADPOOL_CALLBACK()
                    T::Callback(pci, callbackParameter, pTpWait, tpWaitResult);
                END_THREADPOOL_CALLBACK()
            }

            template<class T>
            static void IoCallback(
                PTP_CALLBACK_INSTANCE instance,
                PVOID context,
                PVOID overlapped,
                ULONG result,
                ULONG_PTR bytesTransferred,
                PTP_IO tpIO)
            {
                BEGIN_THREADPOOL_CALLBACK()
                    T::Callback(instance, context, overlapped, result, bytesTransferred, tpIO);
                END_THREADPOOL_CALLBACK()
            }
#endif

            template<class T>
            static void WorkCallback(
                PTP_CALLBACK_INSTANCE instance,
                PVOID state,
                PTP_WORK pTpWork)
            {
                BEGIN_THREADPOOL_CALLBACK()
                    T::Callback(instance, state, pTpWork);
                END_THREADPOOL_CALLBACK()
            }

        private:
            void InitializeCustomPool();

            static void SubmitInternal(PTP_CALLBACK_ENVIRON pcbe, ThreadpoolCallback const &);
            static void SubmitInternal(PTP_CALLBACK_ENVIRON pcbe, ThreadpoolCallback const &, Common::TimeSpan delay, bool allowSyncCall);

            class WorkItem;
            class TimerSimple;

        private:
            PTP_POOL ptpPool_;
            TP_CALLBACK_ENVIRON cbe_;
            PTP_CALLBACK_ENVIRON pcbe_;

            static volatile LONG activeCallbackCount_;
    };
}

