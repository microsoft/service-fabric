// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.Log.Interop
{
    using Microsoft.ServiceFabric.Data;
    using System.Diagnostics;
    using System.Runtime.CompilerServices;

    /// <summary>
    /// Utility class to ease access to KIoBuffer (segmented) contents
    ///
    ///      This class implements a stream abstraction over the contents of a KIoBuffer instance.
    ///      It maintains a current location (in terms of offset) into the contents. There are 
    ///      primitives for advancing the cursor (Skip), positioning it (PostionTo), and fetching
    ///      the current position (GetPosition).
    ///
    ///      The contents can be interated over via the Iterate() method and a caller supplied callback
    ///      (InterationCallback). In addition, there are methods for fetching and storing into and from 
    ///      the stream (Pull and Put). In the cases of Pull and Put, both raw bytes and strong type-safe 
    ///      access is supported all UInt* value types.
    ///
    ///      To support the corner cases where raw access to the underlying buffer contents is desired,
    ///      GetBufferPointer() and GetBufferPointerRange() may be used.
    ///
    ///      There are also Reset() and Reuse() methods.
    /// 
    /// </summary>
    internal unsafe struct KIoBufferStream
    {
        /// <summary>
        /// Construct a KIoBufferStream - throwing an ArgumentOutOfRangeException if InitialOffset
        /// had an invalid value.
        /// </summary>
        public
            KIoBufferStream(NativeLog.IKIoBuffer IoBuffer = null, UInt32 InitialOffset = 0, UInt32 TotalSizeLimit = 0)
        {
            this._Buffer = null;
            this._ElementsDescArray = null;
            this._ElementsDescs = null;
            this._MoveInteratorState = null;
            this._CurrentElementBufferBase = null;
            this._Position = 0;
            this._TotalSize = 0;
            this._NumberOfElements = 0;
            this._CurrentElement = UInt32.MaxValue;
            this._CurrentElementOffset = 0;
            this._CurrentElementSize = 0;
            this._IsInitialized = false;

            if ((IoBuffer != null) && (!this.Reuse(IoBuffer, InitialOffset, TotalSizeLimit)))
            {
                throw new ArgumentOutOfRangeException(SR.Error_InitialOffset);
            }
        }

        /// <summary>
        /// Disassocate a KIoBufferStream with a KIoBuffer
        /// </summary>
        public void
            Reset()
        {
            this._Buffer = null;
            this._ElementsDescArray = null;
            this._ElementsDescs = null;
            this._TotalSize = 0;
            this._NumberOfElements = 0;
            this.InternalClear();
            this._IsInitialized = true;
        }

        /// <summary>
        /// Associate a KIoBufferStream with a KIoBuffer (and optionally set its cursor to InitialOffset - defaulted to 0)
        /// </summary>
        /// <returns>false if the is a range error becasue of the value of InitialOffset</returns>
        public bool
            Reuse(NativeLog.IKIoBuffer IoBuffer, UInt32 InitialOffset = 0, UInt32 TotalSizeLimit = 0)
        {
            this._IsInitialized = true;

            IoBuffer.GetElements(out this._NumberOfElements, out this._ElementsDescArray);
            {
                void* arrayBase;
                this._ElementsDescArray.GetArrayBase(out arrayBase);
                this._ElementsDescs = (NativeLog.KIOBUFFER_ELEMENT_DESCRIPTOR*) arrayBase;
            }

            if (this._NumberOfElements == 1)
            {
                // Optimize for single buffer case
                this._Buffer = IoBuffer;
                this._CurrentElement = 0;
                this._CurrentElementOffset = InitialOffset;
                this._CurrentElementSize = this._ElementsDescs[0].Size;
                this._CurrentElementBufferBase = this._ElementsDescs[0].ElementBaseAddress;
                this._Position = InitialOffset;
                this._TotalSize = this._CurrentElementSize;
                if (TotalSizeLimit > 0)
                {
                    this._TotalSize = Math.Min(this._TotalSize, TotalSizeLimit);
                    this._CurrentElementSize = this._TotalSize;
                }
                return (InitialOffset < this._CurrentElementSize);
            }

            this._Buffer = IoBuffer;
            IoBuffer.QuerySize(out this._TotalSize);
            if (TotalSizeLimit > 0)
            {
                this._TotalSize = Math.Min(this._TotalSize, TotalSizeLimit);
            }
            this.InternalClear();
            return this.PositionTo(InitialOffset);
        }

        /// <summary>
        /// Position the cursor of a KIoBufferStream to a specific Offset in the assocated KIoBuffer.
        /// </summary>
        /// <returns>false if the location is out of range for the KIoBuffer OR there is not an assocated KIoBuffer.</returns>
        public bool
            PositionTo(UInt32 Offset)
        {
            if ((this._Buffer != null) && (this._TotalSize >= Offset))
            {
                this._CurrentElement = 0;
                this._CurrentElementOffset = 0;
                this._CurrentElementSize = Math.Min(this._ElementsDescs[0].Size, this._TotalSize);
                this._CurrentElementBufferBase = this._ElementsDescs[0].ElementBaseAddress;
                this._Position = 0;
                return this.Skip(Offset);
            }

            return false;
        }

        /// <summary>
        /// Return the current position of a KIoBufferStream
        /// </summary>
        public UInt32
            GetPosition()
        {
            return this._Position;
        }

        /// <summary>
        /// User provided callback delegate to the Iterate() method. 
        /// 
        /// For each continous fragment of memory interated over for a given Iterate() call,
        ///  this caller provided delegate is invoked.
        /// </summary>
        /// <param name="IoBufferFragment">pointer to the base of the continous memory fragment.</param>
        /// <param name="FragmentSize">size of the current memory fragment</param>
        /// <returns>!0 - Aborts the current Iterate() and the status is returned to the Iterate() caller.</returns>
        public delegate Int32
            InterationCallback(byte* IoBufferFragment, ref UInt32 FragmentSize);

        /// <summary>
        /// Iterate over the underlying KIoBuffer contents from the current position to the
        /// interval provided (InterationSize) invoking a caller provided delegate for each
        /// continous memory fragment within that interval.
        /// </summary>
        /// <param name="Callback">User callback to invoke for each fragment.</param>
        /// <param name="InterationSize">Limit on the interation interval [currectPos, currectPos + InterationSize -1]</param>
        /// <returns>
        ///      0           - Interval interated over - Current Position += InterationSize.
        ///      -1          - Range error OR no KIoBuffer assocated with the KIoBufferStream.
        ///      {any failure from Callback invocations}
        /// </returns>
        public Int32
            Iterate(InterationCallback Callback, UInt32 InterationSize)
        {
            if (InterationSize > 0)
            {
                if ((this._CurrentElement == UInt32.MaxValue) ||
                    ((this._Position + InterationSize) > this._TotalSize) ||
                    !this._IsInitialized)
                {
                    return -1; // EOS
                }

                while (true)
                {
                    {
                        UInt32 fragmentSize;

                        Debug.Assert(this._CurrentElementSize >= this._CurrentElementOffset);
                        fragmentSize = Math.Min(InterationSize, (this._CurrentElementSize - this._CurrentElementOffset));
                        this._Position += fragmentSize;

                        if (fragmentSize > 0)
                        {
                            if (Callback != null)
                            {
                                var status = Callback((this._CurrentElementBufferBase + this._CurrentElementOffset), ref fragmentSize);
                                if (status != 0)
                                {
                                    return status;
                                }
                            }

                            this._CurrentElementOffset += fragmentSize;
                            InterationSize -= fragmentSize;
                        }
                    }

                    if ((InterationSize > 0) || ((this._CurrentElementOffset == this._CurrentElementSize) && this._TotalSize > this._Position))
                    {
                        this._CurrentElement++;
                        this._CurrentElementOffset = 0;
                        if (this._CurrentElement >= this._NumberOfElements)
                        {
                            return -1; // EOS
                        }

                        this._CurrentElementSize = Math.Min(this._ElementsDescs[this._CurrentElement].Size, (this._TotalSize - this._Position));
                        this._CurrentElementBufferBase = this._ElementsDescs[this._CurrentElement].ElementBaseAddress;
                    }
                    else break;
                }
            }

            return 0;
        }


        /// <summary>
        /// Advance the KIoBufferStream cursor by a specifc (LengthToSkip) amoun
        /// </summary>
        /// <param name="LengthToSkip"></param>
        /// <returns>false - Out of range OR no KIoBuffer assocated with the KIoBufferStream.</returns>
        public bool
            Skip(UInt32 LengthToSkip)
        {
            if (!this._IsInitialized)
                return false;

            return this.Iterate(null, LengthToSkip) == 0;
        }

        /// <summary>
        /// Pull a block of memory of a given size (ResultSize) from the stream; copying it to a given 
        /// location (ResultPtr).
        /// </summary>
        /// <returns>true  - The desired contents are retrieved from Current Location and stored at *ResultPtr 
        ///                  and Current Position += ResultSize.
        ///          false - Out of range OR no KIoBuffer assocated with the KIoBufferStream.</returns>
        public bool
            Pull(byte* ResultPtr, UInt32 ResultSize)
        {
            if (!this._IsInitialized)
                return false;

            this._MoveInteratorState = ResultPtr;

            InterationCallback moveFromBufferIterator = this.MoveFromBufferIterator;
            return this.Iterate(moveFromBufferIterator, ResultSize) == 0;
        }

        /// <summary>
        /// Pull an instance of type-UInt* from the stream; copying it to a given location (Results)
        /// </summary>
        /// <returns>true  - The desired contents are retrieved from Current Location and stored at *ResultPtr 
        ///                  and Current Position += sizeof(Results).
        ///          false - Out of range OR no KIoBuffer assocated with the KIoBufferStream.</returns>
        [MethodImpl(MethodImplOptions.NoInlining)]
        public bool 
        Pull(out byte Results)
        {
            if (!this._IsInitialized)
            {
                Results = 0;
                return false;
            }

            fixed (byte* p = &Results)
            {
                *p = *(this._CurrentElementBufferBase + this._CurrentElementOffset);
            }
            return this.Iterate(null, sizeof(byte)) == 0;
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        public bool 
        Pull(out UInt16 Results)
        {
            if (!this._IsInitialized)
            {
                Results = 0;
                return false;
            }

            fixed (UInt16* p = &Results)
            {
                if ((this._CurrentElementOffset + sizeof(UInt16)) <= this._CurrentElementSize)
                {
                    // UInt16 bytes contained in current element's buffer
                    *p = *((UInt16*) (this._CurrentElementBufferBase + this._CurrentElementOffset));
                    return this.Iterate(null, sizeof(UInt16)) == 0;
                }

                return this.Pull((byte*) p, sizeof(UInt16));
            }
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        public bool 
        Pull(out UInt32 Results)
        {
            if (!this._IsInitialized)
            {
                Results = 0;
                return false;
            }

            fixed (UInt32* p = &Results)
            {
                if ((this._CurrentElementOffset + sizeof(UInt32)) <= this._CurrentElementSize)
                {
                    // UInt32 bytes contained in current element's buffer
                    *p = *((UInt32*) (this._CurrentElementBufferBase + this._CurrentElementOffset));
                    return this.Iterate(null, sizeof(UInt32)) == 0;
                }

                return this.Pull((byte*) p, sizeof(UInt32));
            }
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        public bool 
        Pull(out UInt64 Results)
        {
            if (!this._IsInitialized)
            {
                Results = 0;
                return false;
            }

            fixed (UInt64* p = &Results)
            {
                if ((this._CurrentElementOffset + sizeof(UInt64)) <= this._CurrentElementSize)
                {
                    // UInt64 bytes contained in current element's buffer
                    *p = *((UInt64*) (this._CurrentElementBufferBase + this._CurrentElementOffset));
                    return this.Iterate(null, sizeof(UInt64)) == 0;
                }

                return this.Pull((byte*) p, sizeof(UInt64));
            }
        }

        /// <summary>
        /// Store a block of memory of a given size (Size) into the stream; copying it from a given 
        /// location (SrcPtr)
        /// </summary>
        /// <returns>true Src is stored at Current Position and Current Position += Size; 
        /// Out of range OR no KIoBuffer assocated with the KIoBufferStream</returns>
        public bool
            Put(byte* SrcPtr, UInt32 Size)
        {
            if (!this._IsInitialized)
                return false;

            this._MoveInteratorState = SrcPtr;

            InterationCallback moveToBufferIterator = this.MoveToBufferIterator;
            return this.Iterate(moveToBufferIterator, Size) == 0;
        }

        /// <summary>
        /// Store an instance of type-UInt* into the stream; copying it from a given location (Src).
        /// </summary>
        /// <returns>true Src is stored at Current Position and Current Position += sizeof(UInt*); 
        /// Out of range OR no KIoBuffer assocated with the KIoBufferStream</returns>
        public bool
            Put(byte Src)
        {
            if (!this._IsInitialized)
                return false;

            this._CurrentElementBufferBase[this._CurrentElementOffset] = Src;
            return this.Iterate(null, sizeof(byte)) == 0;
        }

        public bool
            Put(UInt16 Src)
        {
            if (!this._IsInitialized)
                return false;

            if ((this._CurrentElementOffset + sizeof(UInt16)) <= this._CurrentElementSize)
            {
                // T bytes contained in current element's buffer
                *((UInt16*) (this._CurrentElementBufferBase + this._CurrentElementOffset)) = Src;
                return this.Iterate(null, sizeof(UInt16)) == 0;
            }

            return this.Put((byte*) &Src, sizeof(UInt16));
        }

        public bool
            Put(UInt32 Src)
        {
            if (!this._IsInitialized)
                return false;

            if ((this._CurrentElementOffset + sizeof(UInt32)) <= this._CurrentElementSize)
            {
                // T bytes contained in current element's buffer
                *((UInt32*) (this._CurrentElementBufferBase + this._CurrentElementOffset)) = Src;
                return this.Iterate(null, sizeof(UInt32)) == 0;
            }

            return this.Put((byte*) &Src, sizeof(UInt32));
        }

        public bool
            Put(UInt64 Src)
        {
            if (!this._IsInitialized)
                return false;

            if ((this._CurrentElementOffset + sizeof(UInt64)) <= this._CurrentElementSize)
            {
                // T bytes contained in current element's buffer
                *((UInt64*) (this._CurrentElementBufferBase + this._CurrentElementOffset)) = Src;
                return this.Iterate(null, sizeof(UInt64)) == 0;
            }

            return this.Put((byte*) &Src, sizeof(UInt64));
        }

        /// <summary>
        /// Get access to the memory in a KIoBufferStream's current position.
        /// </summary>
        /// <returns>null - No KIoBuffer assocated with the KIoBufferStream</returns>
        public byte*
            GetBufferPointer()
        {
            if (!this._IsInitialized)
                return null;

            return this._CurrentElementBufferBase + this._CurrentElementOffset;
        }

        /// <summary>
        /// Return the limit (size) of the underlying memory at a KIoBufferStream's current position.
        /// </summary>
        /// <returns>0 - No KIoBuffer assocated with the KIoBufferStream.</returns>
        public UInt32
            GetBufferPointerRange()
        {
            if (!this._IsInitialized)
                return 0;

            return this._CurrentElementSize - this._CurrentElementOffset;
        }

        public UInt32
            GetLength()
        {
            if (!this._IsInitialized)
                return 0;

            return this._TotalSize;
        }

        //** Public static methods

        /// <summary>
        /// Utility function to copy a block of a given size (Size) to a KIoBuffer (IoBuffer) at
        ///  a specified offset (DestOffset) to a provided memory location (SrcPtr)
        /// </summary>
        /// <returns>false if out of range detected</returns>
        public static bool
            CopyTo(
            NativeLog.IKIoBuffer IoBuffer,
            UInt32 DestOffset,
            UInt32 Size,
            byte* SrcPtr)
        {
            // Compute current destination location
            var dest = new KIoBufferStream(IoBuffer, DestOffset);
            // NOTE: ctor will fast path for single element KIoBuffers
            //       and code is inlined here

            if (dest._TotalSize < (DestOffset + Size))
            {
                return false; // Out of range request
            }

            if (dest.GetBufferPointerRange() >= Size)
            {
                // Optimize for moving totally within current element
                Memcopy(SrcPtr, dest._CurrentElementBufferBase + dest._CurrentElementOffset, Size);
                return true;
            }

            // Will be crossing element boundary
            return dest.Put(SrcPtr, Size);
        }


        /// <summary>
        ///  Utility function to copy a block of a given size (Size) from a KIoBuffer (IoBuffer) at
        ///  a specified offset (SrcOffset) to a provided memory location (ResultPtr)
        /// </summary>
        /// <returns>false if out of range detected</returns>
        public static bool
            CopyFrom(
            NativeLog.IKIoBuffer IoBuffer,
            UInt32 SrcOffset,
            UInt32 Size,
            byte* ResultPtr)
        {
            // Compute current source location
            var src = new KIoBufferStream(IoBuffer, SrcOffset);
            // NOTE: ctor will fast path for single element KIoBuffers
            //       and code is inlined here

            if (src._TotalSize < (SrcOffset + Size))
            {
                return false; // Out of range request
            }

            if (src.GetBufferPointerRange() >= Size)
            {
                // Optimize for moving totally within current element
                Memcopy(src._CurrentElementBufferBase + src._CurrentElementOffset, ResultPtr, Size);
                return true;
            }

            // Will be crossing element boundary
            return src.Pull(ResultPtr, Size);
        }

        //** Private methods
        private void
            InternalClear()
        {
            this._CurrentElement = UInt32.MaxValue;
            this._CurrentElementBufferBase = null;
            this._CurrentElementOffset = 0;
            this._CurrentElementOffset = 0;
            this._CurrentElementSize = 0;
        }

        private Int32
            MoveFromBufferIterator(byte* IoBufferFragment, ref UInt32 FragmentSize)
        {
            Memcopy(IoBufferFragment, this._MoveInteratorState, FragmentSize);

            this._MoveInteratorState += FragmentSize;
            return 0;
        }

        private Int32
            MoveToBufferIterator(byte* IoBufferFragment, ref UInt32 FragmentSize)
        {
            Memcopy(this._MoveInteratorState, IoBufferFragment, FragmentSize);

            this._MoveInteratorState += FragmentSize;
            return 0;
        }

        //* Private static methods
        private static void
            Memcopy(byte* Src, byte* Dest, UInt32 Length)
        {
            // The best observed throughput has been about 2GB/sec or 500ps/byte without calling CopyMemory. Further we
            // see a native call/return overhead of about 70nsecs/call. This is assuming a memory bandwidth of about 27GB/sec
            // our a mem-to-mem copy rate of about 13.5GB/sec. This means that it takes about 140 byte transfer to break even 
            // above that and we should call CopyMemory.
            if (Length > 140)
            {
                // At least break even overhead
                NativeLog.CopyMemory(Src, Dest, Length);
                return;
            }

            // BUG: richhas, xxxxxx, it would be great to have access to a memcpy intrinsic - this 
            //                       code could just as much damage.

            // The best observed throughput has been about 2GB/sec or 500ps/byte. This means that it takes about 
            // 140 byte transfer to break even 
            while (Length >= sizeof(UInt64))
            {
                *((UInt64*) Dest) = *((UInt64*) Src);
                Src += sizeof(UInt64);
                Dest += sizeof(UInt64);
                if ((Length -= sizeof(UInt64)) == 0)
                    return;
            }

            while (Length > 0)
            {
                *Dest = *Src;
                Src += 1;
                Dest += 1;
                Length--;
            }
        }


        private NativeLog.IKIoBuffer _Buffer;
        private NativeLog.IKArray _ElementsDescArray;
        private UInt32 _TotalSize;
        private UInt32 _NumberOfElements;
        private UInt32 _CurrentElement;
        private UInt32 _CurrentElementOffset;
        private UInt32 _CurrentElementSize;
        private byte* _CurrentElementBufferBase;
        private UInt32 _Position;
        private byte* _MoveInteratorState;
        private bool _IsInitialized;
        private NativeLog.KIOBUFFER_ELEMENT_DESCRIPTOR* _ElementsDescs;
    }
}