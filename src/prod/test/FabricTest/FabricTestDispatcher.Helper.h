// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    typedef std::function<bool(Common::StringCollection const &)> CommandHandler;

    /// <summary>
    /// This is just a helper class to FabricDispatcher.
    /// It was created because FabricDispatcher was getting too big and causing error C1128
    /// (See: https://msdn.microsoft.com/en-us/library/8578y171(v=vs.110).aspx.)
    /// TODO - refactor other command categories of FabricDispatcher into this class.
    /// See bug: 7442612
    /// </summary>
    class FabricTestDispatcherHelper
    {
		DENY_COPY(FabricTestDispatcherHelper)
    public:

        /// <summary>
        /// Adds commands related to RepairManager (RM)
        /// </summary>
		static void AddRepairCommands(__in TestFabricClient & fabricClient, __inout std::map<std::wstring, CommandHandler> & commandHandlers);
    };
}
