// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.StateProviders
{
    using System;
    using System.Text;
    using System.Globalization;
    using System.Runtime.InteropServices;
    using System.Fabric.Data.Common;
    using System.Fabric.Data.Btree;
    using System.Fabric.Data.Btree.Interop;

    /// <summary>
    /// Provides key comparison for binary data type.
    /// </summary>
    public class BinaryKeyComparison : IKeyComparable
    {
        /// <summary>
        /// Constructor.
        /// </summary>
        public BinaryKeyComparison()
        {
            this.comparison = new KeyComparisonDescription();
            this.comparison.KeyDataType = (int)DataType.Binary;
            this.comparison.IsFixedLengthKey = false;
            this.comparison.MaximumKeySizeInBytes = NativeBtree.BtreeMaxKeySize;
        }
        /// <summary>
        /// 
        /// </summary>
        KeyComparisonDescription comparison;

        #region IKeyComparable Members

        /// <summary>
        /// 
        /// </summary>
        public KeyComparisonDescription KeyComparison
        {
            get
            {
                return this.comparison;
            }
        }

        #endregion
    }
    /// <summary>
    /// Provides key comparison for string data type (en-US).
    /// </summary>
    public class StringKeyComparisonEnglishUs : IKeyComparable
    {
        /// <summary>
        /// Constructor.
        /// </summary>
        public StringKeyComparisonEnglishUs()
        {
            this.comparison = new KeyComparisonDescription();
            this.comparison.KeyDataType = (int)DataType.String;
            this.comparison.CultureInfo = new CultureInfo("en-US");
            this.comparison.IsFixedLengthKey = false;
            this.comparison.MaximumKeySizeInBytes = NativeBtree.BtreeMaxKeySize;
        }
        /// <summary>
        /// 
        /// </summary>
        KeyComparisonDescription comparison;

        #region IKeyComparable Members

        /// <summary>
        /// 
        /// </summary>
        public KeyComparisonDescription KeyComparison
        {
            get
            {
                return this.comparison;
            }
        }

        #endregion    
    }
    /// <summary>
    /// Provides key comparison for GUID data type.
    /// </summary>
    public class GuidKeyComparison : IKeyComparable
    {
        /// <summary>
        /// Constructor.
        /// </summary>
        public GuidKeyComparison()
        {
            this.comparison = new KeyComparisonDescription();
            this.comparison.KeyDataType = (int)DataType.Guid;
            this.comparison.IsFixedLengthKey = true;
            this.comparison.MaximumKeySizeInBytes = (uint)Marshal.SizeOf(typeof(Guid));
        }
        /// <summary>
        /// 
        /// </summary>
        KeyComparisonDescription comparison;

        #region IKeyComparable Members

        /// <summary>
        /// 
        /// </summary>
        public KeyComparisonDescription KeyComparison
        {
            get
            {
                return this.comparison;
            }
        }

        #endregion
    }
    /// <summary>
    /// Provides key comparison for ulong data type.
    /// </summary>
    public class UInt32KeyComparison : IKeyComparable
    {
        /// <summary>
        /// Constructor.
        /// </summary>
        public UInt32KeyComparison()
        {
            this.comparison = new KeyComparisonDescription();
            this.comparison.KeyDataType = (int)DataType.UInt32;
            this.comparison.IsFixedLengthKey = true;
            this.comparison.MaximumKeySizeInBytes = (uint)Marshal.SizeOf(typeof(uint));
        }
        /// <summary>
        /// 
        /// </summary>
        KeyComparisonDescription comparison;

        #region IKeyComparable Members

        /// <summary>
        /// 
        /// </summary>
        public KeyComparisonDescription KeyComparison
        {
            get
            {
                return this.comparison;
            }
        }

        #endregion
    }
    /// <summary>
    /// Provides key comparison for int data type.
    /// </summary>
    public class Int32KeyComparison : IKeyComparable
    {
        /// <summary>
        /// Constructor.
        /// </summary>
        public Int32KeyComparison()
        {
            this.comparison = new KeyComparisonDescription();
            this.comparison.KeyDataType = (int)DataType.Int32;
            this.comparison.IsFixedLengthKey = true;
            this.comparison.MaximumKeySizeInBytes = (uint)Marshal.SizeOf(typeof(int));
        }
        /// <summary>
        /// 
        /// </summary>
        KeyComparisonDescription comparison;

        #region IKeyComparable Members

        /// <summary>
        /// 
        /// </summary>
        public KeyComparisonDescription KeyComparison
        {
            get
            {
                return this.comparison;
            }
        }

        #endregion    
    }
    /// <summary>
    /// Provides key comparison for datetime data type.
    /// </summary>
    public class DateTimeKeyComparison : IKeyComparable
    {
        /// <summary>
        /// Constructor.
        /// </summary>
        public DateTimeKeyComparison()
        {
            this.comparison = new KeyComparisonDescription();
            this.comparison.KeyDataType = (int)DataType.Int64;
            this.comparison.IsFixedLengthKey = true;
            this.comparison.MaximumKeySizeInBytes = (uint)Marshal.SizeOf(typeof(long));
        }
        /// <summary>
        /// 
        /// </summary>
        KeyComparisonDescription comparison;

        #region IKeyComparable Members

        /// <summary>
        /// 
        /// </summary>
        public KeyComparisonDescription KeyComparison
        {
            get
            {
                return this.comparison;
            }
        }

        #endregion
    }
}