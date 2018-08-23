// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Text;
    using System.Xml;
    using System.Xml.XPath;

    public class ValueMaps
    {
        private Dictionary<string, MapDefinition> valueMaps;

        public ValueMaps(IXPathNavigable inputMapElements, StringTable strings)
        {
            if (strings == null)
            {
                throw new ArgumentNullException("strings");
            }

            XmlElement mapElements = inputMapElements as XmlElement;
            if (mapElements != null)
            {
                this.valueMaps = new Dictionary<string, MapDefinition>();
                
                foreach (XmlElement valueMap in mapElements.GetElementsByTagName("valueMap"))
                {
                    string mapName = valueMap.GetAttribute("name");
                    MapDefinition mapDef = new ValueMapDefinition(valueMap, strings);

                    this.valueMaps.Add(mapName, mapDef);
                }

                foreach (XmlElement valueMap in mapElements.GetElementsByTagName("bitMap"))
                {
                    string mapName = valueMap.GetAttribute("name");
                    MapDefinition mapDef = new BitMapDefinition(valueMap, strings);

                    this.valueMaps.Add(mapName, mapDef);
                }
            }
        }

        public bool TryGetValue(string mapName, uint key, out string value)
        {
            value = null;

            MapDefinition mapDef = null;
            if (this.valueMaps.TryGetValue(mapName, out mapDef))
            {
                return mapDef.TryGetValue(key, out value);
            }

            return false;
        }

        private abstract class MapDefinition
        {
            public abstract bool TryGetValue(uint key, out string value);

            protected abstract void Add(uint key, string value);

            protected void ParseMap(XmlElement valueMapElement, StringTable strings)
            {
                if (valueMapElement == null)
                {
                    throw new ArgumentNullException("valueMapElement");
                }

                if (strings == null)
                {
                    throw new ArgumentNullException("strings");
                }

                foreach (XmlElement map in valueMapElement.GetElementsByTagName("map"))
                {
                    uint valueKey;
                    string attributeValue = map.GetAttribute("value");

                    if (attributeValue.StartsWith("0x", StringComparison.CurrentCultureIgnoreCase))
                    {
                        attributeValue = attributeValue.Substring(2); 
                        valueKey = UInt32.Parse(attributeValue, NumberStyles.HexNumber, CultureInfo.CurrentCulture);
                    } 
                    else
                    {
                        valueKey = UInt32.Parse(attributeValue, System.Globalization.CultureInfo.InvariantCulture);
                    }

                    string message = map.GetAttribute("message");
                    string value = strings.GetString(message);

                    this.Add(valueKey, value);
                }
            }
        }

        private class ValueMapDefinition : MapDefinition
        {
            private readonly Dictionary<uint, string> valueMap = new Dictionary<uint, string>();

            public ValueMapDefinition(XmlElement valueMapElement, StringTable strings)
            {
                this.ParseMap(valueMapElement, strings);
            }

            public override bool TryGetValue(uint key, out string value)
            {
                return this.valueMap.TryGetValue(key, out value);
            }

            protected override void Add(uint key, string value)
            {
                this.valueMap.Add(key, value);
            }
        }

        private class BitMapDefinition : MapDefinition
        {
            private readonly List<Tuple<uint, string>> bitMap = new List<Tuple<uint, string>>();

            public BitMapDefinition(XmlElement valueMapElement, StringTable strings)
            {
                this.ParseMap(valueMapElement, strings);
            }

            public override bool TryGetValue(uint key, out string value)
            {
                StringBuilder builder = null;
                uint matchMask = 0;

                for (int i = 0; i < this.bitMap.Count; ++i)
                {
                    uint bit = this.bitMap[i].Item1;
                    string bitName = this.bitMap[i].Item2;

                    if (key == bit)
                    {
                        // Handle the zero case, and avoid allocating when a single bit is set
                        value = bitName;
                        return true;
                    }

                    if ((key & bit) != 0)
                    {
                        PrepareForAppend(ref builder);
                        builder.Append(bitName);
                        matchMask |= bit;
                    }
                }

                uint unmatched = key & ~matchMask;
                if (unmatched != 0)
                {
                    // Some bits were not matched by the bitmap
                    PrepareForAppend(ref builder);
                    builder.AppendFormat("0x{0:X}", unmatched);
                }

                if (builder == null)
                {
                    // Nothing matched the bitmap
                    value = null;
                    return false;
                }

                value = builder.ToString();
                return true;
            }

            protected override void Add(uint key, string value)
            {
                this.bitMap.Add(Tuple.Create(key, value));
            }

            private static void PrepareForAppend(ref StringBuilder builder)
            {
                if (builder == null)
                {
                    builder = new StringBuilder(128);
                }
                else
                {
                    builder.Append('|');
                }
            }
        }
    }
}