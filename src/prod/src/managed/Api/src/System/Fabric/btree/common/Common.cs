// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.Common
{
    using System;
    using System.Globalization;


    /// <summary>
    /// 
    /// </summary>
    public sealed class KeyComparisonDescription
    {
        /// <summary>
        /// 
        /// </summary>
        public int KeyDataType;

        /// <summary>
        /// 
        /// </summary>
        public CultureInfo CultureInfo;

        /// <summary>
        /// 
        /// </summary>
        public uint MaximumKeySizeInBytes;

        /// <summary>
        /// 
        /// </summary>
        public bool IsFixedLengthKey;
    }

    /// <summary>
    /// 
    /// </summary>
    public interface IKeyComparable
    {
        KeyComparisonDescription KeyComparison { get; }
    }

    /// <summary>
    /// 
    /// </summary>
    public interface IKeyBitConverter<K>
    {
        /// <summary>
        /// 
        /// </summary>
        /// <param name="key"></param>
        /// <returns></returns>
        byte[] ToByteArray(K key);

        /// <summary>
        /// 
        /// </summary>
        /// <param name="bytes"></param>
        /// <returns></returns>
        K FromByteArray(byte[] bytes);
    }

    /// <summary>
    /// 
    /// </summary>
    /// <typeparam name="V"></typeparam>
    public interface IValueBitConverter<V>
    {
        /// <summary>
        /// 
        /// </summary>
        /// <param name="key"></param>
        /// <returns></returns>
        byte[] ToByteArray(V value);

        /// <summary>
        /// 
        /// </summary>
        /// <param name="bytes"></param>
        /// <returns></returns>
        V FromByteArray(byte[] bytes);
    }

    /// <summary>
    /// 
    /// </summary>
    public interface IRedoUndoInformation
    {
        /// <summary>
        /// 
        /// </summary>
        IOperationData Redo { get; }

        /// <summary>
        /// 
        /// </summary>
        IOperationData Undo { get; }
    }

    /// <summary>
    /// 
    /// </summary>
    class RedoUndoInformation : IRedoUndoInformation
    {
        /// <summary>
        /// 
        /// </summary>
        /// <param name="redo"></param>
        /// <param name="undo"></param>
        public RedoUndoInformation(IOperationData redo, IOperationData undo)
        {
            this.redo = redo;
            this.undo = undo;
        }

        #region Instance Members

        /// <summary>
        /// 
        /// </summary>
        IOperationData redo;

        /// <summary>
        /// 
        /// </summary>
        IOperationData undo;
        
        #endregion

        /// <summary>
        /// 
        /// </summary>
        public IOperationData Redo
        {
            get { return this.redo; }
        }

        /// <summary>
        /// 
        /// </summary>
        public IOperationData Undo
        {
            get { return this.undo; }
        }
    }

    /// <summary>
    /// Disposable base class.
    /// </summary>
    abstract class Disposable : IDisposable
    {
        //
        // IDisposable.
        //
        protected bool disposed;

        /// <summary>
        /// 
        /// </summary>
        /// <param name="disposing"></param>
        protected abstract void Dispose(bool disposing);

        /// <summary>
        /// 
        /// </summary>
        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }
    }
}