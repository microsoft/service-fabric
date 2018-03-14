// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class JobHandle;
    typedef std::unique_ptr<JobHandle> JobHandleUPtr;
    typedef std::shared_ptr<JobHandle> JobHandleSPtr;

    class JobHandle : public HandleBase<HANDLE>
    {
        DENY_COPY(JobHandle)

    public:
        explicit JobHandle(HANDLE hValue, bool autoTerminate);
        virtual ~JobHandle();

        __declspec(property(get=get_AutoTerminate)) bool const & AutoTerminate;
        bool const & get_AutoTerminate() const { return this->autoTerminate_; }

        static JobHandleSPtr CreateSPtr(HANDLE hValue, bool autoTerminate = false);
        static JobHandleUPtr CreateUPtr(HANDLE hValue, bool autoTerminate = false);
        static void Cleanup(HANDLE hValue, bool autoTerminate);

    private:
        bool autoTerminate_;
    };
}
