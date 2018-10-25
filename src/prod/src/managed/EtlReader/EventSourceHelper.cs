// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{
    /// <summary>
    /// This class helps in processing the Events arriving from EventSource stack
    /// </summary>
    public static class EventSourceHelper
    {
        /// <summary>
        /// The event ID for the EventSource manifest emission event.
        /// </summary>
        public const ushort ManifestEventID = 0xFFFE;

        /// <summary>
        /// Checks if the current event contains EventSource DynamicManifest
        /// </summary>
        public static bool CheckForDynamicManifest(EventDescriptor data)
        {
            return data.Id == ManifestEventID && data.Opcode == 0xFE && data.Task == 0xFFFE;
        }
    }
}