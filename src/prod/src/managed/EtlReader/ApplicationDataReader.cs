// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Runtime.InteropServices;
    using System.Text;

    // This code is from Tiho's EventRecordHelper in the debugger
    public unsafe class ApplicationDataReader
    {
        private IntPtr data;
        private uint dataAvailable;
        private bool verifyReads;

        public ApplicationDataReader(IntPtr data)
        {
            this.data = data;
        }

        public ApplicationDataReader(IntPtr data, ushort dataLength) : this(data)
        {
            this.dataAvailable = dataLength;
            this.verifyReads = true;
        }

        private void VerifyRead(int readSize)
        {
            uint dataAvailableAfterRead;
            checked
            {
                // This will throw an OverflowException if we are attempting to
                // read past the end of the buffer
                dataAvailableAfterRead = this.dataAvailable - (uint) readSize;
            }
            this.dataAvailable = dataAvailableAfterRead;
            return;
        }

        private void VerifyAnsiStringRead()
        {
            uint dataAvailableAfterRead = this.dataAvailable;
            IntPtr dataPtrAfterRead = this.data;
            byte ansiChar;
            
            do
            {
                checked
                {
                    // This will throw an OverflowException if we are attempting
                    // to read past the end of the buffer
                    dataAvailableAfterRead = dataAvailableAfterRead - sizeof(byte);
                }

                ansiChar = *((byte*)dataPtrAfterRead);
                dataPtrAfterRead = (IntPtr)(((byte*)dataPtrAfterRead) + sizeof(byte));
            } while (ansiChar != 0);

            this.dataAvailable = dataAvailableAfterRead;
            return;
        }

        private int VerifyUnicodeStringReadAndReturnStringLength()
        {
            uint dataAvailableAfterRead = this.dataAvailable;
            IntPtr dataPtrAfterRead = this.data;
            char unicodeChar;

            int length = 0;

            do
            {
                checked
                {
                    // This will throw an OverflowException if we are attempting
                    // to read past the end of the buffer
                    dataAvailableAfterRead = dataAvailableAfterRead - sizeof(char);
                }

                unicodeChar = *((char*)dataPtrAfterRead);
                ++length;    
                dataPtrAfterRead = (IntPtr)(((byte*)dataPtrAfterRead) + sizeof(char));
            } while (unicodeChar != '\0');

            --length; // length includes null character
            this.dataAvailable = dataAvailableAfterRead;
            return length;
        }

        [SuppressMessage("Microsoft.Naming", "CA1720:IdentifiersShouldNotContainTypeNames", MessageId = "int8", Justification = "This name describes exactly what it does.")]
        public sbyte ReadInt8()
        {
            if (this.verifyReads)
            {
                VerifyRead(sizeof(sbyte));
            }
            sbyte value = *((sbyte*)this.data);
            this.data = (IntPtr)(((byte*)this.data) + sizeof(sbyte));
            return value;
        }

        [SuppressMessage("Microsoft.Naming", "CA1720:IdentifiersShouldNotContainTypeNames", MessageId = "uint8", Justification = "This name describes exactly what it does.")]
        public byte ReadUInt8()
        {
            if (this.verifyReads)
            {
                VerifyRead(sizeof(byte));
            }
            byte value = *((byte*)this.data);
            this.data = (IntPtr)(((byte*)this.data) + sizeof(byte));
            return value;
        }

        public short ReadInt16()
        {
            if (this.verifyReads)
            {
                VerifyRead(sizeof(short));
            }
            short value = *((short*)this.data);
            this.data = (IntPtr)(((byte*)this.data) + sizeof(short));
            return value;
        }

        public ushort ReadUInt16()
        {
            if (this.verifyReads)
            {
                VerifyRead(sizeof(ushort));
            }
            ushort value = *((ushort*)this.data);
            this.data = (IntPtr)(((byte*)this.data) + sizeof(ushort));
            return value;
        }

        public int ReadInt32()
        {
            if (this.verifyReads)
            {
                VerifyRead(sizeof(int));
            }
            int value = *((int*)this.data);
            this.data = (IntPtr)(((byte*)this.data) + sizeof(int));
            return value;
        }

        public uint ReadUInt32()
        {
            if (this.verifyReads)
            {
                VerifyRead(sizeof(uint));
            }
            uint value = *((uint*)this.data);
            this.data = (IntPtr)(((byte*)this.data) + sizeof(uint));
            return value;
        }

        public long ReadInt64()
        {
            if (this.verifyReads)
            {
                VerifyRead(sizeof(long));
            }
            long value = *((long*)this.data);
            this.data = (IntPtr)(((byte*)this.data) + sizeof(long));
            return value;
        }

        public ulong ReadUInt64()
        {
            if (this.verifyReads)
            {
                VerifyRead(sizeof(ulong));
            }
            ulong value = *((ulong*)this.data);
            this.data = (IntPtr)(((byte*)this.data) + sizeof(ulong));
            return value;
        }

        [SuppressMessage("Microsoft.Naming", "CA1720:IdentifiersShouldNotContainTypeNames", MessageId = "float", Justification = "This name describes exactly what it does.")]
        public float ReadFloat()
        {
            if (this.verifyReads)
            {
                VerifyRead(sizeof(float));
            }
            float value = *((float*)this.data);
            this.data = (IntPtr)(((byte*)this.data) + sizeof(float));
            return value;
        }

        public double ReadDouble()
        {
            if (this.verifyReads)
            {
                VerifyRead(sizeof(double));
            }
            double value = *((double*)this.data);
            this.data = (IntPtr)(((byte*)this.data) + sizeof(double));
            return value;
        }

        public bool ReadBoolean()
        {
            const int boolSize = 4;
            if (this.verifyReads)
            {
                VerifyRead(boolSize);
            }
            bool value = *((int*)this.data) != 0;
            this.data = (IntPtr)(((byte*)this.data) + boolSize);
            return value;
        }

        public DateTime ReadFileTime()
        {
            const int fileTimeSize = 8;
            if (this.verifyReads)
            {
                VerifyRead(fileTimeSize);
            }
            long value = *((long*)this.data);
            this.data = (IntPtr)(((byte*)this.data) + fileTimeSize);
            if (value == long.MaxValue)
            {
                return DateTime.MaxValue;
            }

            try
            {
                return DateTime.FromFileTimeUtc(value);
            }
            catch (ArgumentOutOfRangeException)
            {
                // DateTime object was incorrectly used. This is needed to handle bad traces from bug #6590047
                return *(DateTime*) &value;
            }
        }

        public DateTime ReadSystemTime(int length)
        {
            int year = this.ReadInt16();
            int month = this.ReadInt16();
            int dayOfWeek = this.ReadInt16();
            int day = this.ReadInt16();
            int hour = this.ReadInt16();
            int minute = this.ReadInt16();
            int second = this.ReadInt16();
            int milliseconds = this.ReadInt16();
            return new DateTime(year, month, day, hour, minute, second, milliseconds);
        }

        public Guid ReadGuid()
        {
            const int guidSize = 16;
            if (this.verifyReads)
            {
                VerifyRead(guidSize);
            }
            Guid value = new Guid(
                *((int*)this.data),
                *((short*)((byte*)this.data + 4)),
                *((short*)((byte*)this.data + 6)),
                *((byte*)this.data + 8),
                *((byte*)this.data + 9),
                *((byte*)this.data + 10),
                *((byte*)this.data + 11),
                *((byte*)this.data + 12),
                *((byte*)this.data + 13),
                *((byte*)this.data + 14),
                *((byte*)this.data + 15));

            this.data = (IntPtr)(((byte*)this.data) + guidSize);
            return value;
        }

        public byte[] ReadBytes(int length)
        {
            byte[] value = new byte[length];
            if (this.verifyReads)
            {
                VerifyRead(length);
            }
            fixed (byte* pb = value)
            {
                ApplicationDataReader.MemCopy((byte*)this.data, pb, length);
            }

            this.data = (IntPtr)(((byte*)this.data) + length);
            return value;
        }

        public string ReadAnsiString()
        {
            if (this.verifyReads)
            {
                VerifyAnsiStringRead();
            }
            string str = Marshal.PtrToStringAnsi(this.data);
            int length = str.Length + 1;
            this.data = (IntPtr)(((byte*)this.data) + length);
            return str;
        }

        public string ReadUnicodeString()
        {
            if (this.verifyReads)
            {
                int length = VerifyUnicodeStringReadAndReturnStringLength();
                string str = new string((char*)this.data, 0, length);
                this.data = (IntPtr)(((byte*)this.data) + (length + 1) * sizeof(char));
                return str;
            }
            else
            {
                string str = Marshal.PtrToStringUni(this.data);
                int length = (str.Length + 1) * sizeof(char);
                this.data = (IntPtr)(((byte*)this.data) + length);
                return str;
            }
        }

        public string ReadUnicodeStringPrefixLen()
        {
            int length = this.ReadInt16() / 2;
            char[] chars = new char[length];
            for (int i = 0; i < length; i++)
            {
                chars[i] = (char)this.ReadInt16();
            }

            string str = new string(chars);
            return str;
        }

        public ApplicationDataReader SkipBytes(int length)
        {
            if (this.verifyReads)
            {
                VerifyRead(length);
            }
            this.data = (IntPtr)(((byte*)this.data) + length);
            return this;
        }

        public ApplicationDataReader SkipUnicodeString()
        {
            ReadUnicodeString();
            return this;
        }

        public ApplicationDataReader SkipAnsiString()
        {
            if (this.verifyReads)
            {
                VerifyAnsiStringRead();
            }
            string str = Marshal.PtrToStringAnsi(this.data);
            int length = str.Length + 1;
            this.data = (IntPtr)(((byte*)this.data) + length);
            return this;
        }

        private static unsafe void MemCopy(byte* srcPtr, byte* destPtr, int bytesToCopy)
        {
            // AMD64 implementation uses longs instead of ints where possible
            if (bytesToCopy >= 16)
            {
                do
                {
                    if (IntPtr.Size == 8)
                    {
                        ((long*)destPtr)[0] = ((long*)srcPtr)[0];
                        ((long*)destPtr)[1] = ((long*)srcPtr)[1];
                    }
                    else
                    {
                        ((int*)destPtr)[0] = ((int*)srcPtr)[0];
                        ((int*)destPtr)[1] = ((int*)srcPtr)[1];
                        ((int*)destPtr)[2] = ((int*)srcPtr)[2];
                        ((int*)destPtr)[3] = ((int*)srcPtr)[3];
                    }

                    destPtr += 16;
                    srcPtr += 16;
                }
                while ((bytesToCopy -= 16) >= 16);
            }

            // protection against negative len and optimization for len==16*N
            if (bytesToCopy > 0)
            {
                if ((bytesToCopy & 8) != 0)
                {
                    if (IntPtr.Size == 8)
                    {
                        ((long*)destPtr)[0] = ((long*)srcPtr)[0];
                    }
                    else
                    {
                        ((int*)destPtr)[0] = ((int*)srcPtr)[0];
                        ((int*)destPtr)[1] = ((int*)srcPtr)[1];
                    }

                    destPtr += 8;
                    srcPtr += 8;
                }

                if ((bytesToCopy & 4) != 0)
                {
                    ((int*)destPtr)[0] = ((int*)srcPtr)[0];
                    destPtr += 4;
                    srcPtr += 4;
                }

                if ((bytesToCopy & 2) != 0)
                {
                    ((short*)destPtr)[0] = ((short*)srcPtr)[0];
                    destPtr += 2;
                    srcPtr += 2;
                }

                if ((bytesToCopy & 1) != 0)
                {
                    *destPtr++ = *srcPtr++;
                }
            }
        }

    }
}