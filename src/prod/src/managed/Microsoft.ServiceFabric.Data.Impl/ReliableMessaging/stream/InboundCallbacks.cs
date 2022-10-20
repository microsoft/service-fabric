// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Messaging.Stream
{
    #region using directives

    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.ReliableMessaging;
    using System.Globalization;
    using System.Linq;
    using Microsoft.ServiceFabric.Data;

    #endregion

    /// <summary>
    /// This class adds stream suppport to route to multiple inbound stream callbacks by stream name prefix best longest match.
    /// The prefix match is enforced as follows.
    /// 1. Longest match
    /// 2. if none found return the default 
    /// or null if default callback is not set.
    /// 
    /// </summary>
    internal class InboundCallbacks
    {
        #region properties

        /// <summary>
        /// Cache of all callbacks indexed by stream name prefix (Uri).
        /// </summary>
        private readonly SortedDictionary<Uri, IInboundStreamCallback> inboundStreamCallbacks;

        /// <summary>
        /// Default callback, or fallback if stream name prefix does not match.
        /// </summary>
        internal IInboundStreamCallback DefaultCallback { get; set; }

        /// <summary>
        /// This should do as this is not a high R/W collection.
        /// </summary>
        private readonly object syncLock = new object();

        #endregion

        #region cstor

        /// <summary>
        /// Default Constructor.
        /// Stores the callbacks to a sorted dictionary that is sorted for fast retrieval and most importantly 
        /// to support finding the Enumerable.FirstOrDefault callback. 
        /// For this, we use the PrefixUriComparer() to sort in descending order, the keys by length and Uri.Compare if the length is the same.
        /// </summary>
        public InboundCallbacks()
        {
            this.inboundStreamCallbacks = new SortedDictionary<Uri, IInboundStreamCallback>(new PrefixUriComparer());
        }

        #endregion

        /// <summary>
        /// Add the callback to the collections.
        /// </summary>
        /// <param name="prefix">Prefix key for the callback</param>
        /// <param name="callback">Inbound stream callback</param>
        internal void AddCallbackByPrefix(Uri prefix, IInboundStreamCallback callback)
        {
            // Args Check
            prefix.ThrowIfNull("Prefix");
            callback.ThrowIfNull("Callback");
            if (!prefix.IsAbsoluteUri)
            {
                throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, SR.Error_InboundCallbacks_PrefixUri, prefix));
            }

            // Add to the inboundStreamCallbacks collection.
            try
            {
                lock (this.syncLock)
                {
                    this.inboundStreamCallbacks.Add(prefix, callback);
                }
            }
            catch (ArgumentException)
            {
                throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, SR.Error_InboundCallbacks_Prefix_AlreadyExists, prefix));
            }
        }

        /// <summary>
        /// Find and return the closet match(Prefix key that matches the stream name) from the callbacks list.
        /// </summary>
        /// <param name="streamName">Stream name</param>
        /// <returns>IInboundStreamCallback if match found, returns default callback or null otherwise</returns>
        internal IInboundStreamCallback GetCallback(Uri streamName)
        {
            Diagnostics.Assert(streamName != null, "Stream Name is null in GetCallback");

            KeyValuePair<Uri, IInboundStreamCallback> callbackFound;
            lock (this.syncLock)
            {
                callbackFound = this.inboundStreamCallbacks.FirstOrDefault(
                    e => streamName.OriginalString.StartsWith(e.Key.OriginalString));
            }

            // Found..
            if (null != callbackFound.Key)
            {
                return callbackFound.Value;
            }

            // Not found, return default.
            return this.DefaultCallback;
        }

        /// <summary>
        /// Clear the collection.
        /// </summary>
        internal void Clear()
        {
            this.DefaultCallback = null;
            lock (this.syncLock)
            {
                this.inboundStreamCallbacks.Clear();
            }
        }
    }

    internal class PrefixUriComparer : IComparer<Uri>
    {
        /// <summary>
        /// The prefix UriComparer is used to sort(keys) the collection in a descending fashion first by
        /// length and then by value. 
        /// Key 1: Prefix
        /// Key 2: Site
        /// Key 3: Pre
        /// Key 4: Dog
        /// ....
        /// </summary>
        /// <returns>
        /// A signed integer that indicates the relative values of <paramref name="x"/> and <paramref name="y"/>, 
        /// 
        /// Value Meaning Less than zero<paramref name="y"/>  is less than <paramref name="x"/>.
        /// Zero<paramref name="x"/> equals <paramref name="y"/>.
        /// Greater than zero<paramref name="y"/> is greater than <paramref name="x"/>.
        /// </returns>
        /// <param name="x">The first object to compare.</param><param name="y">The second object to compare.</param>
        public int Compare(Uri x, Uri y)
        {
            if (y == null)
                return x == null ? 0 : -1;

            // If y is not null... 
            if (x == null)
                // ...and x is null, y is greater.
                return 1;

            // ...and x is not null, compare the  
            // lengths of the two strings. 
            // 
            var retval = y.OriginalString.Length - x.OriginalString.Length;

            if ((retval) != 0)
            {
                // If the strings are not of equal length, 
                // the longer string is greater. 
                // 
                return retval;
            }

            // If the strings are of equal length, 
            // sort them with ordinary string comparison. 
            return Uri.Compare(y, x, UriComponents.AbsoluteUri, UriFormat.UriEscaped, StringComparison.Ordinal);
        }
    }
}