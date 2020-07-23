// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsReader
{
    using System.Collections.Generic;
    using System.Collections.ObjectModel;

    /// <summary>
    /// Holds the information of a succesfully parsed Uri.
    /// </summary>
    internal class ParsedUri
    {
        public ParsedUri(
            string baseCollection,
            IList<UriCollection> pathCollections,
            string requestCollectionOrMethod,
            IDictionary<string, string> queryParameters)
        {
            this.BaseCollection = baseCollection;
            this.PathCollections = new ReadOnlyCollection<UriCollection>(pathCollections);
            this.RequestCollectionOrMethod = requestCollectionOrMethod;
            this.QueryParameters = new ReadOnlyDictionary<string, string>(queryParameters);
        }

        public string BaseCollection { private set; get; }

        public ReadOnlyCollection<UriCollection> PathCollections { private set; get; }

        public string RequestCollectionOrMethod { private set; get; }

        public ReadOnlyDictionary<string, string> QueryParameters { private set; get; }
    }
}