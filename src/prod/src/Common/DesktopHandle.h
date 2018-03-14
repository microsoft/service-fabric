// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class DesktopHandle;
    typedef std::unique_ptr<DesktopHandle> DesktopHandleUPtr;
    typedef std::shared_ptr<DesktopHandle> DesktopHandleSPtr;

    class DesktopHandle : public HandleBase<HDESK>
    {
        DENY_COPY(DesktopHandle)

    public:
        explicit DesktopHandle(HDESK hValue);
        virtual ~DesktopHandle();
        static DesktopHandleSPtr CreateSPtr(HDESK hValue);
        static DesktopHandleUPtr CreateUPtr(HDESK hValue);
        static void Cleanup(HDESK hValue);
    };
}
