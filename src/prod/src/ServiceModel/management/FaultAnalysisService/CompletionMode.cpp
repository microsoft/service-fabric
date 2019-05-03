// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Management
{
    namespace FaultAnalysisService
    {
        namespace CompletionMode
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & val)
            {
                switch (val)
                {
                case DoNotVerify:
                    w << L"DoNotVerify";
                    return;
                case Verify:
                    w << L"Verify";
                    return;
                default:
                    w << Common::wformatString("Unknown CompletionMode value {0}", static_cast<int>(val));
                }
            }

            Enum FromPublicApi(FABRIC_COMPLETION_MODE const & mode)
            {
                switch (mode)
                {
                case FABRIC_COMPLETION_MODE_DO_NOT_VERIFY:
                    return DoNotVerify;

                case FABRIC_COMPLETION_MODE_VERIFY:
                    return Verify;
                default:
                    return None;
                }
            }

            FABRIC_COMPLETION_MODE ToPublicApi(Enum const & mode)
            {
                switch (mode)
                {
                case DoNotVerify:
                    return FABRIC_COMPLETION_MODE_DO_NOT_VERIFY;

                case Verify:
                    return FABRIC_COMPLETION_MODE_VERIFY;

                default:
                    return FABRIC_COMPLETION_MODE_INVALID;
                }
            }
        }
    }
}
