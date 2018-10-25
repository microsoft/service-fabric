// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca.EtlConsumerHelper
{
    /// <summary>
    /// This structure represents the index of an event within an ETL file
    /// </summary>
    public struct EventIndex
    {
        /// <summary>
        /// Timestamp of the event.
        /// </summary>
        internal DateTime Timestamp;

        /// <summary>
        /// Value that distinguishes this event from other events in the same ETL
        /// file that also bear the same timestamp.
        /// </summary>
        internal int TimestampDifferentiator;

        internal void Set(DateTime timestamp, int timestampDifferentiator)
        {
            this.Timestamp = timestamp;
            this.TimestampDifferentiator = timestampDifferentiator;
        }

        internal int CompareTo(EventIndex index)
        {
            int dateTimeCompareResult = this.Timestamp.CompareTo(index.Timestamp);
            if (dateTimeCompareResult != 0)
            {
                return dateTimeCompareResult;
            }

            if (this.TimestampDifferentiator < index.TimestampDifferentiator)
            {
                return -1;
            }
            else if (this.TimestampDifferentiator == index.TimestampDifferentiator)
            {
                return 0;
            }
            else
            {
                return 1;
            }
        }
    }
}