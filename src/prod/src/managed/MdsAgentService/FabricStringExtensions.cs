// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricMdsAgentSvc
{
    using System.Collections.Generic;
    using System.Linq;

    /// <summary>
    /// Contains the extension methods used in this project.
    /// </summary>
    internal static class FabricStringExtensions
    {
        internal static string FixDisallowedCharacters(this string inputString)
        {
            if (string.IsNullOrEmpty(inputString))
            {
                return inputString;
            }

            // Replace all disallowed characters with the underscore character.
            IEnumerable<char> disallowedChars = inputString.Where(c =>
            {
                return !((c == '_') ||
                         ((c >= 'a') && (c <= 'z')) ||
                         ((c >= 'A') && (c <= 'Z')) ||
                         ((c >= '0') && (c <= '9')));
            });

            string outputString = inputString;
            foreach (char disallowedChar in disallowedChars)
            {
                outputString = outputString.Replace(disallowedChar, '_');
            }

            return outputString;
        }
    }
}