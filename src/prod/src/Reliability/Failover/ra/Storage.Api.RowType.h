// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Storage
        {
            namespace Api
            {
                namespace RowType
                {
                    enum Enum
                    {
                        Invalid,
                        Test,
                        FailoverUnit,
                        LastValidEnum = FailoverUnit
                    };

                    void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

                    DECLARE_ENUM_STRUCTURED_TRACE(RowType);
                }
            }
        }
    }
}
