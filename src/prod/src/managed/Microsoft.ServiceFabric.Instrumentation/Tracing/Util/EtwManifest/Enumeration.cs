// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Test
{
    using System;
    using System.Collections.Generic;
    using System.Text.RegularExpressions;

    /// <summary>
    /// A Enumeration represents an enumerated type and can be used in field definitions.
    /// </summary>
    public sealed class Enumeration
    {
        private SortedDictionary<int, string> values = new SortedDictionary<int, string>();

        public string Name { get; private set; }

        public bool IsBitField { get; private set; }

        public IEnumerable<KeyValuePair<int, string>> Values
        {
            get { return this.values; }
        }

        #region private

        internal Enumeration(string name, bool isBitField)
        {
            this.Name = name;
            this.IsBitField = isBitField;
        }

        internal void Add(int value, string name)
        {
            string otherName;
            if (this.values.TryGetValue(value, out otherName))
            {
                throw new Exception(string.Format("In Enumeration {0} the names {1} and {2} have the same value {3}", this.Name, name, otherName, value));
            }

            this.values[value] = name;
        }

        internal void UpdateStrings(Dictionary<string, string> stringMap)
        {
            var keys = new List<int>(this.values.Keys);
            foreach (var key in keys)
            {
                var value = this.values[key];
                if (EtwProvider.Replace(ref value, stringMap))
                {
                    value = EtwEvent.MakeCamelCase(value);
                    value = Regex.Replace(value, @"[^\w\d_]", "");
                    if (value.Length == 0)
                        value = "_";
                    this.values[key] = value;
                }
            }
        }
        #endregion private
    }
}