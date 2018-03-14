// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

wchar_t HexTable[] = { L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9', L'a', L'b', L'c', L'd', L'e', L'f' }; 

size_t const LargeInteger::SizeOfLargeInteger = sizeof(uint64) * 2;

int const LargeInteger::NumBits = static_cast<int>(LargeInteger::SizeOfLargeInteger * 8);

int const LargeInteger::NumHalfBits = static_cast<int>(LargeInteger::NumBits / 2);

LargeInteger const LargeInteger::One = (LargeInteger const) LargeInteger(0, 1);

LargeInteger const & LargeInteger::StaticInit_Zero()
{
    static const LargeInteger zero = (LargeInteger const) LargeInteger(0, 0);
    return zero;
}

LargeInteger const & LargeInteger::Zero = LargeInteger::StaticInit_Zero();

uint64 const LargeInteger::MostSignificantBit = ((uint64) 1) << (LargeInteger::NumHalfBits - 1);

LargeInteger const & LargeInteger::StaticInit_MaxValue()
{
    static const LargeInteger maxValue = LargeInteger(0, 0) - LargeInteger(0, 1);
    return maxValue;
}

LargeInteger const & LargeInteger::MaxValue = LargeInteger::StaticInit_MaxValue();

mt19937_64 LargeInteger::RandomEngine(Stopwatch::Now().Ticks);

LargeInteger::LargeInteger()
    : high_(0), low_(0)
{
}

LargeInteger::LargeInteger(uint64 high, uint64 low)      
    : high_(high), low_(low)
{
}

LargeInteger::LargeInteger(LargeInteger const & value)
    : high_(value.high_), low_(value.low_)
{
}

LargeInteger::~LargeInteger()
{
}

LargeInteger & LargeInteger::operator = (LargeInteger const & other)
{
    high_ = other.high_;
    low_ = other.low_;

    return *this;
}

bool LargeInteger::operator < (LargeInteger const & other ) const
{
    return high_ < other.high_ || (high_ == other.high_ && low_ < other.low_);
}

bool LargeInteger::operator >= (LargeInteger const & other ) const
{
    return !(*this < other);
}

bool LargeInteger::operator > (LargeInteger const & other ) const
{
    return high_ > other.high_ || (high_ == other.high_ && low_ > other.low_);
}

bool LargeInteger::operator <= (LargeInteger const & other ) const
{
    return !(*this > other);
}

bool LargeInteger::operator == (LargeInteger const & other ) const
{
    return (low_ == other.low_ && high_ == other.high_);
}

bool LargeInteger::operator != (LargeInteger const & other ) const
{
    return !( *this == other);
}

// Add the other LargeInteger to the current LargeInteger.  Overflow will be discarded.
LargeInteger & LargeInteger::operator +=(LargeInteger const & other)
{
    uint64 newLow = low_ + other.low_;
    if (newLow < low_)
    {
        high_ ++;
    }

    low_ = newLow;
    high_ += other.high_;
    
    return *this;
}

// Add two LargeInteger and return a new LargeInteger.  Overflow will be discarded.
const LargeInteger LargeInteger::operator +(LargeInteger const & other) const
{
    LargeInteger newInteger = *this;
    newInteger += other;
    return newInteger;
}

// Subtract the other LargeInteger from the current LargeInteger.  Overflow will be discarded.
LargeInteger & LargeInteger::operator -=(LargeInteger const & other)
{        
    uint64 newLow = low_ - other.low_;
    if (newLow > low_)
    {
        high_ --;
    }

    low_ = newLow;
    high_ -= other.high_;
    
    return *this;
}

// Subtract two LargeInteger and return a new LargeInteger.  Overflow will be discarded.
const LargeInteger LargeInteger::operator -(LargeInteger const & other) const
{
    LargeInteger newInteger = *this;
    newInteger -= other;
    return newInteger;
}

// Multiply two 64 bit unsigned integers and return the result as a LargeInteger.
// a*b = c * 2^64 + (d+e) * 2^32 + f
const LargeInteger GetProduct(const uint64 a, const uint64 b)
{
    uint64 mask = (static_cast<uint64>(1) << 32) - 1;
    uint64 aHigh = (a & ~mask) >> 32;
    uint64 aLow = a & mask;
    uint64 bHigh = (b & ~mask) >> 32;
    uint64 bLow = b & mask;

    // y = (d+e) * 2^32
    uint64 d = aHigh * bLow;
    uint64 e = aLow * bHigh;
    LargeInteger y(0, d);
    y += LargeInteger(0, e);
    y <<= 32;

    uint64 c = aHigh * bHigh;
    LargeInteger product(c, 0);

    product += y;
    uint64 f = aLow * bLow;
    product += LargeInteger(0, f);

    return product;
}

// Multiply modulo 2^128, with the other LargeInteger.
LargeInteger & LargeInteger::operator *=(LargeInteger const & other)
{
    LargeInteger x = GetProduct(high_, other.low_);
    x += GetProduct(low_, other.high_);
    LargeInteger y(x.Low, 0);
    y += GetProduct(low_, other.low_);

    high_ = y.High;
    low_ = y.Low;

    return *this;
}

// Multiplies two LargeIntegers and returns a new LargeInteger. Overflow will be discarded.
const LargeInteger LargeInteger::operator *(LargeInteger const & other) const
{
    LargeInteger newInteger = *this;
    newInteger *= other;
    return newInteger;
}

LargeInteger & LargeInteger::operator ++()
{
    return *this += One;
}

LargeInteger & LargeInteger::operator --()
{
    return *this -= One;
}

LargeInteger LargeInteger::operator ++(int)
{
    LargeInteger copy(*this);
    ++(*this);
    return copy;
}

LargeInteger LargeInteger::operator --(int)
{
    LargeInteger copy(*this);
    --(*this);
    return copy;
}

LargeInteger & LargeInteger::operator <<=(int count)
{
    if (count < NumHalfBits)
    {
        int overflow_bit = NumHalfBits - count;
        uint64 overflow = low_ >> overflow_bit;
        low_ <<= count;
        high_ <<= count;
        high_ |= overflow;
    }
    else if (count < NumBits)
    {
        high_ = low_;
        high_ <<= count - NumHalfBits;
        low_ = 0;
    }
    else
    {
        high_ = 0;
        low_ = 0;
    }
    
    return *this;
}

const LargeInteger LargeInteger::operator <<(int count) const
{
    LargeInteger newInteger = *this;
    newInteger <<= count;
    return newInteger;
}

LargeInteger & LargeInteger::operator >>=(int count)
{
    if (count < NumHalfBits)
    {
        int overflow_bit = NumHalfBits - count;
        uint64 overflow = high_ << overflow_bit;
        low_ >>= count;
        high_ >>= count;
        low_ |= overflow;
    }
    else if (count < NumBits)
    {
        low_ = high_;
        low_ >>= count - NumHalfBits;
        high_ = 0;
    }
    else
    {
        high_ = 0;
        low_ = 0;
    }
    
    return *this;
}

const LargeInteger LargeInteger::operator >>(int count) const
{
    LargeInteger newInteger = *this;
    newInteger >>= count;
    return newInteger;
}

LargeInteger & LargeInteger::operator &=(LargeInteger const & other)
{
    high_ &= other.high_;
    low_ &= other.low_;
    
    return *this;
}

const LargeInteger LargeInteger::operator &(LargeInteger const & other) const
{
    LargeInteger newInteger = *this;
    newInteger &= other;
    
    return newInteger;
}

LargeInteger & LargeInteger::operator |=(LargeInteger const & other)
{
    high_ |= other.high_;
    low_ |= other.low_;

    return *this;
}

const LargeInteger LargeInteger::operator |(LargeInteger const & other) const
{
    LargeInteger newInteger = *this;
    newInteger |= other;

    return newInteger;
}

LargeInteger & LargeInteger::operator ^=(LargeInteger const & other)
{
    high_ ^= other.high_;
    low_ ^= other.low_;

    return *this;
}

const LargeInteger LargeInteger::operator ^(LargeInteger const & other) const
{
    LargeInteger newInteger = *this;
    newInteger ^= other;

    return newInteger;
}

const LargeInteger LargeInteger::operator ~(void) const
{
    LargeInteger newInteger(~high_, ~low_);
    return newInteger;
}

const LargeInteger LargeInteger::operator -(void) const
{
    return LargeInteger::Zero - *this;
}

LargeInteger LargeInteger::RandomLargeInteger_()
{
    uint64 low = (uint64) RandomEngine();
    uint64 high = (uint64) RandomEngine();
 
    return LargeInteger(low, high);
}

bool LargeInteger::IsSmallerPart() const
{
    return (high_ & MostSignificantBit) == 0;
}

void WriteLargeInteger_To(uint64 high, uint64 low, TextWriter& writer)
{
    if (high == 0)
    {
        writer.Write("{0:x}", low);
    }
    else
    {
        writer.Write("{0:x}{1:016x}", high, low);
    }
}

bool TryParseLargeInteger(size_t sizeOfLargeInteger, wstring const & from, uint64 & high, uint64 & low)
{
    low = 0;
    high = 0;

    size_t len = from.size();
    size_t high_len = 0;

    if (len > sizeOfLargeInteger * 2)
    {
        return false;
    }
    else if (len > sizeOfLargeInteger)
    {
        high_len = len - sizeOfLargeInteger;
        
        if (!StringUtility::TryFromWString(from.substr(0, high_len), high, 16))
        {
            return false;
        }
    }

    if (!StringUtility::TryFromWString(from.substr(high_len), low, 16))
    {
        return false;
    }

    return true;
}

wstring LargeInteger::ToString() const
{
    wstring result;
    result.reserve(33);
    StringWriter(result).Write(*this);
    return result;
}

void LargeInteger::WriteTo(TextWriter& writer, FormatOptions const &) const
{
    WriteLargeInteger_To(high_, low_, writer);
}

void LargeInteger::FillEventData(TraceEventContext & context) const
{
    context.Write<uint64>(high_);
    context.Write<uint64>(low_);
}

bool LargeInteger::TryParse(wstring const & from, LargeInteger & value)
{
    return TryParseLargeInteger(LargeInteger::SizeOfLargeInteger, from, value.high_, value.low_);
}

FABRIC_NODE_ID LargeInteger::ToPublicApi() const
{   
    FABRIC_NODE_ID nodeId;
    nodeId.High = high_;
    nodeId.Low = low_;
    nodeId.Reserved = NULL;
    return nodeId;
}

void LargeInteger::FromPublicApi(FABRIC_NODE_ID const& nodeId)
{   
    high_ = nodeId.High;
    low_ = nodeId.Low;
}
