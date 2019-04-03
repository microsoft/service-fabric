// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsReader
{
    using System;
    using System.Collections.Generic;

    /// <summary>
    /// Parser for Request's Uri.
    /// </summary>
    internal class RequestParser
    {
        public static ParsedUri ParseUri(string suffixUri)
        {
            var splitSuffixUri = suffixUri.Split(new[] { ReaderConstants.QueryStringDelimiter }, 2);
            var temp = splitSuffixUri[0].Split(ReaderConstants.FragmentDelimiter);
            if (temp.Length > 1)
            {
                splitSuffixUri[0] = temp[0];
            }

            var suffixUriList = splitSuffixUri[0].Trim(ReaderConstants.SegmentDelimiter).Split(ReaderConstants.SegmentDelimiter);
            if (suffixUriList.Length < 2)
            {
                throw new ArgumentException("Invalid request, Uri suffix:{0}", suffixUri);
            }

            var baseCollection = suffixUriList[0];
            var pathCollections = new List<UriCollection>();
            string lastSegmentName = null;
            for (var i = 1; i < suffixUriList.Length; ++i)
            {
                var currentSegment = suffixUriList[i];
                if (i < suffixUriList.Length - 1)
                {
                    var nextSegment = suffixUriList[i + 1];
                    if (nextSegment == ReaderConstants.MetadataSegment)
                    {
                        if (lastSegmentName == null)
                        {
                            throw new ArgumentException("Invalid request, Uri suffix:{0}", suffixUri);
                        }

                        var collection = new UriCollection(lastSegmentName);
                        collection.SetValueOfCollectionKey(currentSegment);
                        pathCollections.Add(collection);
                        lastSegmentName = null;
                        i += 1;
                        continue;
                    }
                }

                if (lastSegmentName != null)
                {
                    pathCollections.Add(new UriCollection(lastSegmentName));
                }

                lastSegmentName = currentSegment;
            }

            var requestCollectionOrMethod = lastSegmentName;
            var queryParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            if (splitSuffixUri.Length > 1)
            {
                var parameters = splitSuffixUri[1].Split(
                    new[] { ReaderConstants.And },
                    StringSplitOptions.RemoveEmptyEntries);

                foreach (var parameter in parameters)
                {
                    var splitParameter = parameter.Split(new[] { ReaderConstants.Equal }, 2);
                    if (splitParameter.Length > 1 && !queryParameters.ContainsKey(splitParameter[0]))
                    {
                        queryParameters.Add(splitParameter[0], splitParameter[1]);
                    }
                }
            }

            return new ParsedUri(
                baseCollection, 
                pathCollections, 
                requestCollectionOrMethod,
                queryParameters);
        }

        public static QueryParametersWrapper ParseQueryParameters(IReadOnlyDictionary<string, string> parameters)
        {
            return new QueryParametersWrapper(parameters);
        }
    }
}