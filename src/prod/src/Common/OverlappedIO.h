// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class OverlappedIo
    {
        DENY_COPY(OverlappedIo);

    public:
        OverlappedIo();
        virtual ~OverlappedIo();

        static void CALLBACK Callback(
            PTP_CALLBACK_INSTANCE instance,
            PVOID context,
            PVOID rawOverlapped,
            ULONG result,
            ULONG_PTR bytesTransferred,
            PTP_IO io);

        LPOVERLAPPED OverlappedPtr();

        void Complete(ErrorCode const & error, ULONG_PTR bytesTransfered);

    private:
        virtual void OnComplete(PTP_CALLBACK_INSTANCE pci, ErrorCode const & error, ULONG_PTR bytesTransfered) = 0;

    private:
        OVERLAPPED overlapped_;
    };

    class ThreadpoolIo
    {
        DENY_COPY(ThreadpoolIo);

    public:
        ThreadpoolIo();
        ~ThreadpoolIo();

        ErrorCode Open(HANDLE handle);
        void Close();
        void Swap(_Inout_ ThreadpoolIo & other);

        bool operator!() const;
        void StartIo();
        void CancelIo();

    private:
        HANDLE handle_;
        PTP_IO io_;
    };
}
