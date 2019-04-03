// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Reader
{
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;
    using System;

    /// <summary>
    /// Property level filters
    /// </summary>
    public class PropertyFilter
    {
        public object Value { get; }

        public string Name { get; }

        public DataType DataType { get; }

        public PropertyFilter(string propertyName, string value)
        {
            Assert.IsNotNull(propertyName, "propertyName != null");
            Assert.IsNotNull(value, "value != null");

            this.Name = propertyName;
            this.Value = value;
            this.DataType = DataType.String;
        }

        public PropertyFilter(string propertyName, Guid value)
        {
            Assert.IsNotNull(propertyName, "propertyName != null");
            Assert.AssertIsTrue(value != Guid.Empty, "value != empty Guid");

            this.Name = propertyName;
            this.Value = value;
            this.DataType = DataType.Guid;
        }

        public PropertyFilter(string propertyName, int value)
        {
            Assert.IsNotNull(propertyName, "propertyName != null");

            this.Name = propertyName;
            this.Value = value;
            this.DataType = DataType.Int32;
        }

        public PropertyFilter(string propertyName, long value)
        {
            Assert.IsNotNull(propertyName, "propertyName != null");

            this.Name = propertyName;
            this.Value = value;
            this.DataType = DataType.Int64;
        }

        public Guid GetGuidValue()
        {
            if (this.DataType != DataType.Guid)
            {
                throw new InvalidCastException("Value Type is not Guid");
            }

            return (Guid)this.Value;
        }

        public string GetStringValue()
        {
            if (this.DataType != DataType.String)
            {
                throw new InvalidCastException("Value Type is not String");
            }

            return (string)this.Value;
        }

        public int GetIntValue()
        {
            if (this.DataType != DataType.Int32)
            {
                throw new InvalidCastException("Value Type is not Int");
            }

            return (int)this.Value;
        }

        public Int64 GetIn64Value()
        {
            if (this.DataType != DataType.Int64)
            {
                throw new InvalidCastException("Value Type is not Int");
            }

            return (long)this.Value;
        }

        public override bool Equals(object obj)
        {
            var other = obj as PropertyFilter;
            if (other == null)
            {
                return false;
            }

            if (this.Name != other.Name || this.DataType != other.DataType)
            {
                return false;
            }

            if (this.DataType == DataType.Int64)
            {
                return this.GetIn64Value() == other.GetIn64Value();
            }
            else if (this.DataType == DataType.Guid)
            {
                return this.GetGuidValue() == other.GetGuidValue();
            }
            else if (this.DataType == DataType.String)
            {
                return this.GetStringValue() == other.GetStringValue();
            }
            else if (this.DataType == DataType.Int32)
            {
                return this.GetIntValue() == other.GetIntValue();
            }
            else
            {
                return false;
            }
        }

        public override int GetHashCode()
        {
            unchecked
            {
                int hash = 17;
                hash = (hash * 23) + this.Name.GetHashCode();
                hash = (hash * 23) + this.DataType.ToString().GetHashCode();
                hash = (hash * 23) + this.Value.GetHashCode();
                return hash;
            }
        }
    }
}