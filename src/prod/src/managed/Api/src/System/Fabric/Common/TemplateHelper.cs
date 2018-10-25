// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System;
    using System.Collections.Specialized;

    /// <summary>
    /// TemplateHelper - substitute templated content
    /// </summary>
    internal static class TemplateHelper
    {
        /// <summary>
        /// Replaces all occurences of [[VARIABLE]] in templatedData with their associated values in the collection.
        /// Example: If templated data is "this is a [[VARIABLE_1]] [[VARIABLE_2]] message" and the collection contains
        /// {"[[VARIABLE_1]]" -> hello} and {"VARIABLE_2" -> world} this function will return
        /// "this is a hello world message"
        /// </summary>
        /// <param name="templatedData"></param>
        /// <param name="collection"></param>
        /// <returns></returns>
        public static string ApplyTemplate(string templatedData, NameValueCollection collection)
        {
            const string VariablePrefix = "[[";
            const string VariableSuffix = "]]";

            string replacedContent = templatedData;

            foreach (string key in collection.AllKeys)
            {
                if (key.StartsWith(VariablePrefix, StringComparison.Ordinal) && key.EndsWith(VariableSuffix, StringComparison.Ordinal))
                {
                    replacedContent = replacedContent.Replace(key, collection[key]);
                }
                else
                {
                    replacedContent = replacedContent.Replace(VariablePrefix + key + VariableSuffix, collection[key]);
                }
            }

            return replacedContent;
        }
    }
}