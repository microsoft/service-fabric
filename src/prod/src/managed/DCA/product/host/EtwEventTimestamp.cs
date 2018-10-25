// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;

    // Structure that represents the timestamp of an ETW event. 
    internal struct EtwEventTimestamp
    {
        // Timestamp provided for the ETW event by the OS
        internal DateTime Timestamp { get; set; }

        // Sequence number of ETW event among multiple ETW events
        // having the same OS timestamp
        internal int Differentiator { get; set; }

        internal int CompareTo(EtwEventTimestamp otherTimestamp)
        {
            int dateTimeCompareResult = this.Timestamp.CompareTo(otherTimestamp.Timestamp);
            if (dateTimeCompareResult != 0)
            {
                return dateTimeCompareResult;
            }

            if (this.Differentiator < otherTimestamp.Differentiator)
            {
                return -1;
            }
            else if (this.Differentiator == otherTimestamp.Differentiator)
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