// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Strings;
    using System.Xml;
    using System.Xml.XPath;

    public class StringTable
    {
        private Dictionary<string, string> strings;

        public StringTable(IXPathNavigable inputStringTable)
        {
            if (inputStringTable == null)
            {
                throw new ArgumentNullException("inputStringTable");
            }

            XmlElement stringTable = inputStringTable as XmlElement;
            if (stringTable == null)
            {
                throw new ArgumentException(StringResources.ETLReaderError_InvalidXmlElement_InputStringTable);
            }

            this.strings = new Dictionary<string, string>();
            foreach (XmlElement s in stringTable.GetElementsByTagName("string"))
            {
                this.strings.Add(s.GetAttribute("id"), s.GetAttribute("value"));
            }
        }

        public string GetString(string reference)
        {
            if (reference == null)
            {
                throw new ArgumentNullException("reference");
            }

            string prefix = "$(string.";
            if (!reference.StartsWith(prefix, StringComparison.Ordinal))
            {
                return "?";
            }

            string key = reference.Substring(prefix.Length, reference.Length - prefix.Length - 1);
            return this.strings[key];
        }
    }
}