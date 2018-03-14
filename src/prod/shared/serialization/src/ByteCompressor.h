// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <math.h>

#define MAX_COMPRESSION_SIZE(typeSize)     ((typeSize * 8 + 6) / 7)   /*ceiling of (typeSize * 8 / 7)*/
#define MASK_7BIT     0x7F     /*0b 0111 1111 */
#define MASK_MOREDATA 0x80     /*0b 1000 0000 */
#define MASK_NEGATIVE 0x40     /*0b 0100 0000 */

class ByteCompressor
{
public:

    template <class T>
    static ULONG CompressValue(T value, UCHAR * buffer, bool isSigned);
    
    template <class T>
    static NTSTATUS ReadAndUncompress(Serialization::FabricSerializableStream* stream, __out T& value, bool signedRead);
};

template <class T>
ULONG ByteCompressor::CompressValue(T value, __out_bcount(MAX_COMPRESSION_SIZE(sizeof(T))) UCHAR * buffer, bool isSigned)
{
    const ULONG Size = MAX_COMPRESSION_SIZE(sizeof(T));
    ULONG index = Size - 1;

    if (value == 0)
    {
        buffer[index] = 0;
        return 0;
    }

    T target = 0;
    if (isSigned && value<0)
    {
        target = ~target;
    }
    UCHAR signBit = (UCHAR)(target & MASK_NEGATIVE);

    T temp = value;   
    bool isFirst = true;    

    for (;;)
    { 
        UCHAR b = (MASK_7BIT & temp); // grab

        if (!isFirst)
        {
            b |= MASK_MOREDATA;
        }
        
        buffer[index] = b;            
        isFirst = false;
        --index;          

        temp >>= 7;

        //done for unsigned value, or unsigned value if sign bit has been compressed.
        if ((temp == target) && (!isSigned || (b & MASK_NEGATIVE) == signBit)) 
        {
            break;
        }                    
    }

    return Size - index -1 ;
}           

template <class T>
NTSTATUS ByteCompressor::ReadAndUncompress(Serialization::FabricSerializableStream* stream, __out T& value, bool signedRead)
{       
    NTSTATUS status = STATUS_SUCCESS;
    const ULONG maxSize = MAX_COMPRESSION_SIZE(sizeof(T));
                   
    UCHAR byteValue = 0;
    status = stream->ReadRawBytes(1, &byteValue);
    if (NT_ERROR(status))
    {
        return status;
    }
  
    value = 0;
    if (signedRead && (byteValue & MASK_NEGATIVE) != 0)
    {
        value = ~value;
    }   
    
    for (ULONG readSize = 1; readSize <= maxSize; ++readSize)
    {
        UCHAR b = (byteValue & MASK_7BIT);

        value <<= 7; // make room for data
        value |= b; // take only the data portion

        if ((byteValue & MASK_MOREDATA) == 0)
        {
            //data end reached.
            break;
        }

        if(readSize == maxSize)
        {
            // we have read the max number of bytes but not the end byte yet...
            return K_STATUS_INVALID_STREAM_FORMAT;
        }
        else
        {
            status = stream->ReadRawBytes(1, &byteValue);  
            if (NT_ERROR(status))
            {
                return status;
            }  
        }
    }

    return status;
}

class SignedByteCompressor
{
public:

    template <class T>
    static ULONG Compress(T t, UCHAR * buffer)
    {
        // Don't compress BOOL, CHAR, UCHAR
        static_assert(sizeof(T) >= 2, "Values under 2 bytes cannot be compressed");

        return ByteCompressor::CompressValue(t, buffer, true);
    }

    template <class T>
    static NTSTATUS ReadAndUncompress(Serialization::FabricSerializableStream* stream, __out T& value)
    {
        return ByteCompressor::ReadAndUncompress<T>(stream, value, true);
    }    
};

class UnsignedByteCompressor
{
public:

    template <class T>
    static ULONG Compress(T t, UCHAR * buffer)
    {
        // Don't compress BOOL, CHAR, UCHAR
        static_assert(sizeof(T) >= 2, "Values under 2 bytes cannot be compressed");

        return ByteCompressor::CompressValue(t, buffer, false);
    }

    template <class T>
    static NTSTATUS ReadAndUncompress(Serialization::FabricSerializableStream* stream, __out T& value)
    {
        return ByteCompressor::ReadAndUncompress<T>(stream, value, false);
    }
};
