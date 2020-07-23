// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class FabricBackupRestoreAgentImpl :
        public Common::ComponentRoot,
        Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(FabricBackupRestoreAgentImpl)

    public:
        FabricBackupRestoreAgentImpl(
            Common::ComponentRoot const & parent,
            __in ApplicationHost & host);
        virtual ~FabricBackupRestoreAgentImpl();

        __declspec(property(get=get_Host)) ApplicationHost & Host;
        inline ApplicationHost & get_Host() { return host_;  };

    private:
        Common::ComponentRootSPtr const parent_;
        ApplicationHost & host_;
    };
}
