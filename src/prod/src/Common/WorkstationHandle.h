// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class WorkstationHandle;
    typedef std::unique_ptr<WorkstationHandle> WorkstationHandleUPtr;
    typedef std::shared_ptr<WorkstationHandle> WorkstationHandleSPtr;

    class WorkstationHandle : public HandleBase<HWINSTA>
    {
        DENY_COPY(WorkstationHandle)

    public:
        explicit WorkstationHandle(HWINSTA hValue);
        virtual ~WorkstationHandle();
        static WorkstationHandleSPtr CreateSPtr(HWINSTA hValue);
        static WorkstationHandleUPtr CreateUPtr(HWINSTA hValue);
        static void Cleanup(HWINSTA hValue);
    };
}
