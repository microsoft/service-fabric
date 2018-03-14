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
                namespace OperationType
                {
                    enum Enum
                    {
                        Update = 0,
                        Insert = 1,
                        Delete = 2,
                    };

                    void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
                }
            }
        }
    }
}



