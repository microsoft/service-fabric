// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability
{
    using System.Collections.Generic;
    using System.Linq;
    using System.Text.RegularExpressions;
    using TraceDiagnostic.Store;

    /// <summary>
    /// Helper methods and fields for Regex manipulations
    /// </summary>
    internal static class RegexHelper
    {
        private static readonly RegexOptions RegexOption =    RegexOptions.Compiled
                                                            | RegexOptions.CultureInvariant
                                                            | RegexOptions.Singleline;
        /// <summary>
        /// Dictionary of Regexes for various events
        /// </summary>
        internal static readonly Dictionary<string, List<Regex>> EventRegexes = new Dictionary<string, List<Regex>>()
        {
            { EventTypes.ApiError, new List<Regex>()
                {
                    new Regex(@"^(?:.*?,){4}(?<TaskName>.*?)\..*?API (?<operation>.*?) for FT (?<PartitionKey>.*?) on node (?<nodeId>.*?) with Replica Id (?<replicaId>.*?) and Replica Instance Id (?<replicaInstanceId>.*?) has returned (?<error>.*)$", RegexOption),
                    new Regex(@".*", RegexOption)
                } 
            },
            { EventTypes.ApiSlow, new List<Regex>()
                {
                    new Regex(@"^(?:.*?,){4}(?<TaskName>.*?)\..*?API (?<operation>.*?) for FT (?<PartitionKey>.*?) on node (?<nodeId>.*?) with Replica Id (?<replicaId>.*?) and Replica Instance Id (?<replicaInstanceId>.*?) has exceeded the threshold$", RegexOption),
                    new Regex(@".*", RegexOption)
                }
            },
            { EventTypes.HostedServiceUnexpectedTermination, new List<Regex>()
                {
                    new Regex(@"^(?:.*?,){4}(?<TaskName>.*?)\..*?@(?<PartitionKey>.*?),.*?terminated unexpectedly with code (?<ReturnCode>.*?) and process name (?<ProcessName>.*)$", RegexOption),
                    new Regex(@".*", RegexOption)
                } 
            },
            {
                EventTypes.ProcessUnexpectedTermination, new List<Regex>()
                {
                    new Regex(@"^(?:.*?,){4}(?<TaskName>.*?)\..*?@(?<PartitionKey>.*?),ServiceHostProcess: (?<ProcessName>\S*?)\s.*?terminated unexpectedly with exit code (?<ReturnCode>\S*?)\s.*$", RegexOption),
                    new Regex(@".*", RegexOption)
                }
            }
        };

        /// <summary>
        /// Retrieves all named captures when a given regex matches a given input string
        /// The behavior differs from usual Regex match, for when the match is unsuccessful
        /// this method - instead of throwing exception - returns the entire input string
        /// </summary>
        /// <param name="regex">Regex to apply</param>
        /// <param name="input">String to match</param>
        /// <returns>All named captures</returns>
        internal static Dictionary<string, string> MatchNamedCaptures(this Regex regex, string input)
        {
            var match = regex.Match(input);
            if (match.Success)
            {
                GroupCollection groups = match.Groups;
                string[] groupNames = regex.GetGroupNames();

                return groupNames
                    .Where(groupName => groups[groupName].Captures.Count > 0)
                    .ToDictionary(groupName => groupName, groupName => groups[groupName].Value);
            }

            return new Dictionary<string, string>() { { "EntireInput", input } };
        }

        /// <summary>
        /// Retrieves all named captures when any of a given list of regexes, matches a given input string
        /// The behavior differs from usual Regex match, for when the match is unsuccessful
        /// this method - instead of throwing exception - returns the entire input string
        /// </summary>
        /// <param name="regexes">List of Regexes to apply</param>
        /// <param name="input">String to match</param>
        /// <returns>All named captures</returns>
        internal static Dictionary<string, string> MatchNamedCaptures(this List<Regex> regexes, string input)
        {
            foreach (var regex in regexes)
            {
                var captures = regex.MatchNamedCaptures(input);
                if (captures.Count > 1)
                {
                    return captures;
                }
            }

            return new Dictionary<string, string>() { { "EntireInput", input } };
        }
    }
}


#pragma warning restore 1591