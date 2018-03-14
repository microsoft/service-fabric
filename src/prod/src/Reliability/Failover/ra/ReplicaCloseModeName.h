// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReplicaCloseModeName
        {
            enum Enum
            {
                None,

                // Upgrade/RuntimeDown where a close is needed from RAP
                Close,

                // RF Transient for SL/SV
                // Any scenario where RA needs to drop the FT on RAP by calling CR (None)
                Drop,

                // Close due to Deactivate Node
                DeactivateNode,

                // Report Fault Permanentwhere a CR(None)/Abort is needed from RAP
                Abort,

                // ReportFault Transient for SP
                Restart,

                // Delete from FM
                Delete,

                // Deactivate is S->N transition
                Deactivate,

                // Any scenario where RA decides that the replica must be aborted
                // and it is okay to leak state and move on
                // Replica Dropped is required
                ForceAbort,

                // Any scenario where FM requires that the replica must be aborted
                // and it is okay to leak state and move on
                // This is authorized by the FM and the FM has already marked the replica as deleted
                // So no reply needs to be sent to FM
                ForceDelete,

                // Any scenario where the FM has already marked the replica as DD and RA needs to delete it
                // No reply is needed to FM as the FM has already performed the task
                // Only applicable for down replicas with the delete to happen when the replica comes up
                // Example: Deleted service and a deactivate node with a replica of that service comes up
                // RA should mark it as deleted in the replica up reply and delete it when the replica comes up
                QueuedDelete,

                // When the host went down
                AppHostDown,

                // When RA needs to terminate the host and force drop the replica
                Obliterate,

                LastValidEnum = Obliterate
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

            DECLARE_ENUM_STRUCTURED_TRACE(ReplicaCloseModeName);
        }
    }
}


