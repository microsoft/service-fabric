// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.ReliableSession.Test
{
    using System.Fabric.ReliableMessaging.Session;
    using System.Fabric.Test;

    /// <summary>
    /// Logging partition session aborts callbacks
    /// </summary>
    class SessionAbortedCallBack : IReliableSessionAbortedCallback
    {
       /// <summary>
       /// Abort callback for a singleton service
       /// </summary>
       /// <param name="isInbound">Is Inbound/Outbound Session</param>
       /// <param name="sourceServiceInstanceName">Service instance name</param>
       /// <param name="session">Aborted session</param>
        public void SessionAbortedByPartner(
            bool isInbound, Uri sourceServiceInstanceName, IReliableMessagingSession session)
        {
            string sessionKind = isInbound ? "Inbound" : "Outbound";
            string partnerKind = isInbound ? "Source" : "Target";
            LogHelper.Log("{0} Session Aborted for Singleton {1} {2}", sessionKind, partnerKind, sourceServiceInstanceName);
        }

        /// <summary>
        /// Abort callback for a range partition service
        /// </summary>
        /// <param name="isInbound">Is Inbound/Outbound Session</param>
        /// <param name="sourceServiceInstanceName">Service instance name</param>
        /// <param name="partitionKey">Partition key range</param>
        /// <param name="session">Aborted session</param>
        public void SessionAbortedByPartner(
            bool isInbound, Uri sourceServiceInstanceName, IntegerPartitionKeyRange partitionKey, IReliableMessagingSession session)
        {
            string sessionKind = isInbound ? "Inbound" : "Outbound";
            string partnerKind = isInbound ? "Source" : "Target";
            LogHelper.Log("{0} Session Aborted for Int64 Partition {1} {2}:{3}-{4}", sessionKind, partnerKind, sourceServiceInstanceName, partitionKey.IntegerKeyLow, partitionKey.IntegerKeyHigh);
        }

        /// <summary>
        /// Abort callback for a named partition service
        /// </summary>
        /// <param name="isInbound">Is Inbound/Outbound Session</param>
        /// <param name="sourceServiceInstanceName">Service instance name</param>
        /// <param name="partitionKey">Named partition key</param>
        /// <param name="session">Aborted session</param>
        public void SessionAbortedByPartner(
            bool isInbound, Uri sourceServiceInstanceName, string partitionKey, IReliableMessagingSession session)
        {
            string sessionKind = isInbound ? "Inbound" : "Outbound";
            string partnerKind = isInbound ? "Source" : "Target";
            LogHelper.Log("{0} Session Aborted for String Partition {1} {2}:{3}", sessionKind, partnerKind, sourceServiceInstanceName, partitionKey);
        }
    }
}