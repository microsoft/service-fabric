// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    // The value type is used to represent a large integer (128 bit unsigned int) that  
    // is required to represent a Node ID. It supports the primitive mathematical
    // and boolean operations that can be performed on it.
    struct LargeInteger
    {

    public:
        // Constructors
        // TODO: Add Move constructor

        //---------------------------------------------------------------------
        // Constructor.
        //
        // Description:
        //          Takes the high and low values and sets them accordingly for this object.
        //
        // Arguments:
        //          high - the high bits of the large integer.
        //          low - the low bits of the large integer.
        //
        LargeInteger(uint64 high, uint64 low);

        //---------------------------------------------------------------------
        // Constructor.
        //
        // Description: Default constructor.
        //
        LargeInteger();

        //---------------------------------------------------------------------
        // Constructor.
        //          Copy constructor
        // Description:
        //          Creates a copy of the argument.
        //
        // Arguments:
        //          value - the value to be copied.
        //
        LargeInteger(LargeInteger const & value);

        ~LargeInteger();

        // Parsers.

        //---------------------------------------------------------------------
        // Function: LargeInteger_<T>::TryParse
        //
        // Description:
        //          This function tries to parse a string and generate a LargeInteger_ based 
        //          on that string. 
        //
        // Arguments:
        //          from - This is the string that is to be parsed so as to create a LargeInteger_.
        //          value - This is the large integer reference whose low and high uint64s should 
        //          be set based on the string provided. The space for the value variable should
        //          be allocated before calling this function.
        //
        // Returns:
        //          true - if the parsing succeeded.
        //          false - if an error occured during parsing.
        //
        // Exceptions: (optional)
        //          none (this must be guaranteed)
        //
        static bool TryParse(std::wstring const & from, LargeInteger & value);

        // Generate a random LargeInteger_.
        static LargeInteger RandomLargeInteger_();

        // Properties

        //---------------------------------------------------------------------
        // Function: LargeInteger_<T>::IsSmallPart
        //
        // Description:
        //          Whether the highest bit is 0.
        //
        // Returns:
        //          true - if highest bit is 0.
        //          false - otherwise.
        //
        // Exceptions: (optional)
        //          none (this must be guaranteed)
        //
        bool IsSmallerPart() const;

        LargeInteger & operator = (LargeInteger const & other);

        // Comparison Operators
        bool operator < (LargeInteger const & other ) const;
        bool operator >= (LargeInteger const & other ) const;
        bool operator > (LargeInteger const & other ) const;
        bool operator <= (LargeInteger const & other ) const;
        bool operator == (LargeInteger const & other ) const;
        bool operator != (LargeInteger const & other ) const;

        // Mathematical Operators.
        LargeInteger & operator +=(LargeInteger const & other);
        const LargeInteger operator +(LargeInteger const & other) const;

        LargeInteger & operator -=(LargeInteger const & other);
        const LargeInteger operator -(LargeInteger const & other) const;

        LargeInteger & operator *= (LargeInteger const & other);
        const LargeInteger operator *(LargeInteger const & other) const;

        LargeInteger & operator ++();
        LargeInteger & operator --();
        LargeInteger operator++(int);
        LargeInteger operator--(int);

        // Shift Operators
        LargeInteger & operator <<=(int count);
        const LargeInteger operator <<(int count) const;
        LargeInteger & operator >>=(int count);
        const LargeInteger operator >>(int count) const;

        // Logical Operators.
        LargeInteger & operator &=(LargeInteger const & other);
        const LargeInteger operator &(LargeInteger const & other) const;
        LargeInteger & operator |=(LargeInteger const & other);
        const LargeInteger operator |(LargeInteger const & other) const;
        LargeInteger & operator ^=(LargeInteger const & other);
        const LargeInteger operator ^(LargeInteger const & other) const;
        const LargeInteger operator ~() const;
        const LargeInteger operator -() const;

        std::wstring ToString() const;
        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        // The maximum value allowed for a LargeInteger_.
        static LargeInteger const & MaxValue;

        // Used to initialize for other compilation units
        static LargeInteger const & StaticInit_MaxValue();

        // The LargeInteger_ representation of value 1.
        static LargeInteger const One;

        // The LargeInteger_ representation of value 0.  It is also the minimum value allowed.
        static LargeInteger const & Zero;

        // Used to initialize for other compilation units
        static LargeInteger const & StaticInit_Zero();

        __declspec (property(get=get_High)) uint64 High;
        uint64 get_High() const { return high_; }

        __declspec (property(get=get_Low)) uint64 Low;
        uint64 get_Low() const { return low_; }

        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name)
        {
            traceEvent.AddField<uint64>(name + "._type:LargeInteger_high");
            traceEvent.AddField<uint64>(name + ".low");

            return "{0:x}{1:x}";
        }

        void FillEventData(Common::TraceEventContext & context) const;

        FABRIC_NODE_ID ToPublicApi() const;
        void FromPublicApi(FABRIC_NODE_ID const& nodeId);

        FABRIC_PRIMITIVE_FIELDS_02(high_, low_);

        struct Hasher
        {
            size_t operator() (LargeInteger const & key) const
            {
                return key.low_;
            }

            bool operator() (LargeInteger const & left, LargeInteger const & right) const
            {
                return (left == right);
            }
        };

    private:
        // Higher order bits of LargeInteger_
        uint64 high_;

        // Lower order bits of LargeInteger_
        uint64 low_;

        // Size of a large integer.
        static size_t const SizeOfLargeInteger;

        // Number of bits in the integer.
        static int const NumBits;

        // Half the number of bits in the integer.
        static int const NumHalfBits;   

        // A random number generator.
        static std::mt19937_64 RandomEngine;

        // A constant representing the most significant bit
        static uint64 const MostSignificantBit;
    };
}
