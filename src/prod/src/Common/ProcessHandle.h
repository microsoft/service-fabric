// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class ProcessHandle;
    typedef std::unique_ptr<ProcessHandle> ProcessHandleUPtr;
    typedef std::shared_ptr<ProcessHandle> ProcessHandleSPtr;

    class ProcessHandle : public HandleBase<HANDLE>
    {
        DENY_COPY(ProcessHandle)

    public:
        explicit ProcessHandle(HANDLE hValue, bool autoTerminate);
        virtual ~ProcessHandle();

        __declspec(property(get=get_AutoTerminate)) bool const & AutoTerminate;
        bool const & get_AutoTerminate() const { return this->autoTerminate_; }

        static ProcessHandleSPtr CreateSPtr(HANDLE hValue, bool autoTerminate = false);
        static ProcessHandleUPtr CreateUPtr(HANDLE hValue, bool autoTerminate = false);
        static void Cleanup(HANDLE hValue, bool autoTerminate);

    private:
        bool autoTerminate_;
    };
}
