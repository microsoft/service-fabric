// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    namespace RepairAction
    {
        enum Enum
        {
            None = 0,
            DropDatabase = 1,
            TerminateProcess = 2,

            FirstValidEnum = None,
            LastValidEnum = TerminateProcess
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

        DECLARE_ENUM_STRUCTURED_TRACE( RepairAction )
    };
}
