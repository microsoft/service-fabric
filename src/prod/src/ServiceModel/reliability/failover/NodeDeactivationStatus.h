// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace NodeDeactivationStatus
    {
        // The order is important and show progression from the start of deactivation
        // to the complete of activation.
        enum Enum
        {
            DeactivationSafetyCheckInProgress = 0,
            DeactivationSafetyCheckComplete = 1,
            DeactivationComplete = 2,
            ActivationInProgress = 3,
            None = 4,
        };

        FABRIC_NODE_DEACTIVATION_STATUS ToPublic(Enum deactivationStatus);

        void WriteToTextWriter(Common::TextWriter & w, Enum const& value);

        Enum Merge(Enum const &, Enum const &);

        DECLARE_ENUM_STRUCTURED_TRACE(NodeDeactivationStatus);
    }
}

DEFINE_AS_BLITTABLE(Reliability::NodeDeactivationStatus::Enum);
