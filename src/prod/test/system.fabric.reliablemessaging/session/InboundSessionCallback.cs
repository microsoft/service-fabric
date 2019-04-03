// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.ReliableSession.Test
{
    using System.Fabric.ReliableMessaging.Session;

    /// <summary>
    /// Callback offering a session.
   /// </summary>
    class InboundSessionCallback : ReliableSessionTest, IReliableInboundSessionCallback
    {
        readonly PartitionRecord myPartitionRecord;

        public InboundSessionCallback(PartitionRecord partitionRec)
        {
            this.myPartitionRecord = partitionRec;
        }
        /// <summary>
        /// Callback offering a session.
        /// Use the instance number (in URI segment - 3) to assign the callback session to the right global partition object.
        /// Three overloads for singleton, numbered and named partition types
        /// </summary>
        /// <returns> true to grant permission and false to deny</returns>
        public bool AcceptInboundSession(
            Uri sourceServiceInstanceName, IReliableMessagingSession session)
        {
            // singleton partition types
            // Key = ServiceInstanceName + PartitionID (in the case of singletone == 0).
            string key = sourceServiceInstanceName + ":" + "0";
            if (!this.myPartitionRecord.InboundSessions.ContainsKey(key))
            {
                this.myPartitionRecord.InboundSessions.Add(key, session);
            }
            return true;

        }

        /// <summary>
        /// Callback offering a session.
        /// Use the instance number (in URI segment - 3) to assign the callback session to the right global partition object.
        /// Three overloads for singleton, numbered and named partition types
        /// </summary>
        /// <param name="sourceServiceInstanceName"></param>
        /// <param name="partitionKey">Numbered partition types</param>
        /// <param name="session"></param>
        /// <returns> true to grant permission and false to deny</returns>
        public bool AcceptInboundSession(
            Uri sourceServiceInstanceName, IntegerPartitionKeyRange partitionKey, IReliableMessagingSession session)
        {
            string key = sourceServiceInstanceName + ":" + partitionKey.IntegerKeyLow + ":" + partitionKey.IntegerKeyHigh;
            if (!this.myPartitionRecord.InboundSessions.ContainsKey(key))
            {
                this.myPartitionRecord.InboundSessions.Add(key, session);
            }
            return true;
        }

        /// <summary>
        /// Callback offering a session.
        /// Use the instance number (in URI segment - 3) to assign the callback session to the right global partition object.
        /// Three overloads for singleton, numbered and named partition types
        /// </summary>
        /// <param name="sourceServiceInstanceName"></param>
        /// <param name="partitionKey">Named partition types</param>
        /// <param name="session"></param>
        /// <returns> true to grant permission and false to deny</returns>
        public bool AcceptInboundSession(
            Uri sourceServiceInstanceName, string partitionKey, IReliableMessagingSession session)
        {
            string key = sourceServiceInstanceName + ":" + partitionKey;
            if (!this.myPartitionRecord.InboundSessions.ContainsKey(key))
            {
                this.myPartitionRecord.InboundSessions.Add(key, session);
            }
            return true;
        }
    }
}