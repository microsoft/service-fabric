// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{

    class FabricTestCommandsTracker
    {
    public:
        typedef std::function<bool(void)> CommandCallback;

    public:
        FabricTestCommandsTracker();

        bool RunAsync(CommandCallback const &);
        bool WaitForAll(Common::TimeSpan const & timeout);
        bool IsEmpty() const;

    private:
        class CommandHandle
        {
        public:
            CommandHandle();

            bool WaitForResult(Common::TimeSpan const & timeout);
            void SetResult(bool result);

        private:
            Common::ManualResetEvent wait_;
            bool result_;
        };

        std::vector<std::shared_ptr<CommandHandle>> handles_;
        mutable Common::ExclusiveLock handlesLock_;
    };
}
