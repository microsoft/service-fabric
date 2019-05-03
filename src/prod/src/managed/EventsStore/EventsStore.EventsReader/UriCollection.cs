// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsReader
{
    using System;

    /// <summary>
    /// Represents a single collection segment of an Uri.
    /// </summary>
    internal class UriCollection
    {
        public UriCollection(string collectionName)
        {
            this.Name = collectionName;
        }

        public string Name { private set; get; }

        public Tuple<string, string> KeyValuePair { private set; get; }

        public void SetValueOfCollectionKey(string value)
        {
            this.KeyValuePair = new Tuple<string, string>(
                GetCollectionKey(this.Name),
                value);
        }

        public string GetValueOfCollectionKey()
        {
            if (this.HasKeySet())
            {
                return this.KeyValuePair.Item2;
            }

            return null;
        }

        public bool HasKeySet()
        {
            return this.KeyValuePair != null;
        }

        private static string GetCollectionKey(string name)
        {
            switch (name.ToLower())
            {
                case ReaderConstants.Nodes: return ReaderConstants.NodeName;
                case ReaderConstants.Applications: return ReaderConstants.ApplicationId;
                case ReaderConstants.Services: return ReaderConstants.ServiceId;
                case ReaderConstants.Partitions: return ReaderConstants.PartitionId;
                case ReaderConstants.Replicas: return ReaderConstants.ReplicaId;
                case ReaderConstants.CorrelatedEvents: return ReaderConstants.EventInstanceId;
                default:
                    throw new ArgumentException(
                        "Unsupported key-based collection {0}",
                        name);
            }
        }
    }
}