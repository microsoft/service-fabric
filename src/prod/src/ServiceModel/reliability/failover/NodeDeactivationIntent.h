// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace NodeDeactivationIntent
    {
        enum Enum
        {
            None = 0,
            Pause = 1,
            Restart = 2,
            RemoveData = 3,
            RemoveNode = 4,
        };

        Common::ErrorCode FromPublic(FABRIC_NODE_DEACTIVATION_INTENT pulbicDeactivationIntent, __out Enum & deactivationIntent);

        FABRIC_NODE_DEACTIVATION_INTENT ToPublic(Enum deactivationIntent);

        void WriteToTextWriter(Common::TextWriter & w, Enum const& value);

        DECLARE_ENUM_STRUCTURED_TRACE( NodeDeactivationIntent )
    }
}

DEFINE_AS_BLITTABLE(Reliability::NodeDeactivationIntent::Enum);
