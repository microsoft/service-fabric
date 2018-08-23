// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Fabric.Common.Serialization;
    using System.Globalization;
    using System.Numerics;
    using System.Runtime.InteropServices;
    using System.Text.RegularExpressions;

    /// <summary>
    /// <para>Class to encapsulate a node ID.</para>
    /// </summary>
    [StructLayout(LayoutKind.Explicit, Size = 16)]
    public class NodeId
    {
        private const int CharsPerKey = 16;

        private static readonly NodeId OneId = new NodeId(0, 1);
        private static readonly NodeId ZeroId = new NodeId(0, 0);
        private static readonly NodeId ZeroMinusOneId = new NodeId(ulong.MaxValue, ulong.MaxValue);

        [FieldOffset(0)]
        private readonly ulong high;

        [FieldOffset(8)]
        private readonly ulong low;

        /// <summary>
        /// <para>Initializes a new <see cref="System.Fabric.NodeId" /> object, with the specified high and low order components.</para>
        /// </summary>
        /// <param name="high">
        /// <para>The high order component of the <see cref="System.Fabric.NodeId" /> object.</para>
        /// </param>
        /// <param name="low">
        /// <para>The low order component of the <see cref="System.Fabric.NodeId" /> object.</para>
        /// </param>
        public NodeId(BigInteger high, BigInteger low)
        {
            this.low = (ulong)low;
            this.high = (ulong)high;
        }

        internal NodeId(NodeId other)
        {
            this.high = other.high;
            this.low = other.low;
        }

        private NodeId()
        {
            this.low = 0;
            this.high = 0;
        }

        /// <summary>
        /// <para>The low order component of the <see cref="System.Fabric.NodeId" /> object.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Numerics.BigInteger" />.</para>
        /// </value>
        [JsonCustomization(IsIgnored = true)]
        public BigInteger Low
        {
            get
            {
                return this.low;
            }
        }

        /// <summary>
        /// <para>The high order component of the <see cref="System.Fabric.NodeId" /> object.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Numerics.BigInteger" />.</para>
        /// </value>
        [JsonCustomization(IsIgnored = true)]
        public BigInteger High
        {
            get
            {
                return this.high;
            }
        }

        internal static NodeId One
        {
            get { return NodeId.OneId; }
        }

        internal static NodeId Zero
        {
            get { return NodeId.ZeroId; }
        }

        internal static NodeId MaxValue
        {
            get { return NodeId.ZeroMinusOneId; }
        }

        internal static NodeId MinValue
        {
            get { return NodeId.ZeroId; }
        }

        // Native and Rest structure has this property.
        internal string Id
        {
            get
            {
                return this.ToString();
            }
        }

        /// <summary>
        /// <para>Converts the string representation of a node ID to its <see cref="System.Fabric.NodeId" /> object equivalent. A return value indicates whether the operation succeeded.</para>
        /// </summary>
        /// <param name="from">
        /// <para>A string containing the node ID to convert.</para>
        /// </param>
        /// <param name="parsedNodeId">
        /// <para>When this method returns, contains a new <see cref="System.Fabric.NodeId" /> object equivalent to the node ID contained in <paramref name="from" />, 
        /// if the conversion succeeded, or <languageKeyword>null</languageKeyword> if the conversion failed. This parameter is passed uninitialized.</para>
        /// </param>
        /// <returns>
        /// <returns>A boolean indicating if the parse was successful</returns>
        /// </returns>
        public static bool TryParse(string from, out NodeId parsedNodeId)
        {
            parsedNodeId = null;
            ulong low = 0;
            ulong high = 0;

            int lengthHighPart = 0;
            int lengthBigIntegerString = NodeId.CharsPerKey;

            if (from.Length > lengthBigIntegerString * 2)
            {
                return false;
            }
            else if (from.Length > lengthBigIntegerString)
            {
                lengthHighPart = from.Length - lengthBigIntegerString;
                if (!ulong.TryParse(from.Substring(0, lengthHighPart), NumberStyles.HexNumber, CultureInfo.InvariantCulture, out high))
                {
                    return false;
                }
            }

            if (!ulong.TryParse(from.Substring(lengthHighPart), NumberStyles.HexNumber, CultureInfo.InvariantCulture, out low))
            {
                return false;
            }

            parsedNodeId = new NodeId(new BigInteger(high), new BigInteger(low));
            return true;
        }

        /// <summary>
        /// <para>Determines whether two <see cref="System.Fabric.NodeId" /> objects have the same value.</para>
        /// </summary>
        /// <param name="value1">
        /// <para>A <see cref="System.Fabric.NodeId" /> object to compare with <paramref name="value2" />.</para>
        /// </param>
        /// <param name="value2">
        /// <para>A <see cref="System.Fabric.NodeId" /> object to compare with <paramref name="value1" />.</para>
        /// </param>
        /// <returns>
        /// <para>Returns a <see cref="System.Boolean" /> value that is <languageKeyword>true</languageKeyword> if the objects are equivalent;
        /// otherwise <languageKeyword>false</languageKeyword>.</para>
        /// </returns>
        public static bool operator ==(NodeId value1, NodeId value2)
        {
            if(ReferenceEquals(value1, value2))
            {
                return true;
            }

            if (ReferenceEquals(value1, null) || ReferenceEquals(value2, null))
            {
                return false;
            }

            return (value1.low == value2.low) && (value1.high == value2.high);
        }

        /// <summary>
        /// <para>Determines whether two <see cref="System.Fabric.NodeId" /> objects have different values.</para>
        /// </summary>
        /// <param name="value1">
        /// <para>A <see cref="System.Fabric.NodeId" /> object to compare with <paramref name="value2" />.</para>
        /// </param>
        /// <param name="value2">
        /// <para>A <see cref="System.Fabric.NodeId" /> object to compare with <paramref name="value1" />.</para>
        /// </param>
        /// <returns>
        /// <para>Returns a <see cref="System.Boolean" /> value that is <languageKeyword>true</languageKeyword> if the objects have different values; 
        /// otherwise <languageKeyword>false</languageKeyword>.</para>
        /// </returns>
        public static bool operator !=(NodeId value1, NodeId value2)
        {
            return !(value1 == value2);
        }

        /// <summary>
        /// <para>Returns the hash code for this <see cref="System.Fabric.NodeId" /> object.</para>
        /// </summary>
        /// <returns>
        /// <para>A 32-bit signed integer hash code.</para>
        /// </returns>
        public override int GetHashCode()
        {
            return (this.high ^ this.low).GetHashCode();
        }

        /// <summary>
        /// <para>Indicates whether this <see cref="System.Fabric.NodeId" /> object and the specified object are equal.</para>
        /// </summary>
        /// <param name="obj">
        /// <para>The object to compare with the current <see cref="System.Fabric.NodeId" />.</para>
        /// </param>
        /// <returns>
        /// <para>Returns a <see cref="System.Boolean" /> value that is <languageKeyword>true</languageKeyword> 
        /// if the objects are the same type and represent the same value; otherwise <languageKeyword>false</languageKeyword>.</para>
        /// </returns>
        public override bool Equals(object obj)
        {
            return (obj is NodeId) && (this == ((NodeId)obj));
        }

        /// <summary>
        /// <para>Creates and returns a string representation of the current node ID.</para>
        /// </summary>
        /// <returns>
        /// <para>Returns <see cref="System.String" />.</para>
        /// </returns>
        public override string ToString()
        {
            if (this.High == 0L)
            {
                return this.Low.ToString("x", CultureInfo.InvariantCulture);
            }

            return string.Format(CultureInfo.InvariantCulture, "{0:x" + "}{1:x" + NodeId.CharsPerKey + "}", this.high, this.low);
        }

        internal static string ToNodeName(string nodeId)
        {
            return string.Format(CultureInfo.InvariantCulture, "nodeid:{0}", nodeId);
        }

        internal static string ToNodeName(NodeId nodeId)
        {
            return ToNodeName(nodeId.ToString());
        }

        /// <summary>
        /// Creates a new <code>WinfabricNodeId</code> from a hexadecimal string. The string must be of the format from ToString.
        /// </summary>
        /// <param name="hexValue">hexadecimal representation of the NodeId</param>
        /// <returns><code>WinFabricNodeId</code> generated from the string</returns>
        /// <exception cref="ArgumentException">thrown when the argument is an invalid hexadecimal representation</exception>
        /// <exception cref="FormatException">thrown when the argument is invalid hexadecimal</exception>
        internal static NodeId ConvertFromHexString(string hexValue)
        {
            if (string.IsNullOrEmpty(hexValue))
            {
                throw new ArgumentNullException(@"value cannot be null or empty", "hexValue");
            }

            if (!Regex.IsMatch(hexValue, @"\A\b[0-9a-fA-F]+\b\Z"))
            {
                throw new ArgumentException(@"not a hex string", "hexValue");
            }

            if (hexValue.Length > NodeId.CharsPerKey * 2)
            {
                throw new ArgumentException(string.Format(@"unexpected string length: {0}", hexValue.Length.ToString()), "hexValue");
            }

            string lowKeyStr = "0";
            string highKeyStr = "0";

            int lengthOfLow = Math.Min(hexValue.Length, NodeId.CharsPerKey);
            lowKeyStr = hexValue.Substring(hexValue.Length - lengthOfLow, lengthOfLow);
            ulong lowKey = ulong.Parse(lowKeyStr, NumberStyles.HexNumber, CultureInfo.InvariantCulture);

            int lengthOfHigh = hexValue.Length - lengthOfLow;
            if (lengthOfHigh > 0)
            {
                highKeyStr = hexValue.Substring(0, lengthOfHigh);
            }

            ulong highKey = ulong.Parse(highKeyStr, NumberStyles.HexNumber, CultureInfo.InvariantCulture);

            return new NodeId(highKey, lowKey);
        }
    }
}