// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#if !defined(PLATFORM_UNIX)
#include "windns.h"
#endif

namespace DNS
{
#define DNS_MAX_NAME_LENGTH             (255)
#define DNS_MAX_LABEL_LENGTH            (63)

#define BIT7(ch)        ((ch) & 0x80)
#define BIT6(ch)        ((ch) & 0x40)
#define BIT5(ch)        ((ch) & 0x20)
#define BIT4(ch)        ((ch) & 0x10)
#define BIT3(ch)        ((ch) & 0x08)

#define B_RFC                   0x00000001
#define B_NUMBER                0x00000002
#define B_UPPER                 0x00000004
#define B_NON_RFC               0x00000008

#define B_UTF8_TRAIL            0x00000010
#define B_UTF8_FIRST_TWO        0x00000020
#define B_UTF8_FIRST_THREE      0x00000040
#define B_UTF8_PAIR             0x00000080

#define B_DOT                   0x00000800
#define B_SPECIAL               0x00001000
#define B_LEADING_ONLY          0x00004000

    //
    //  Generic characters
    //

#define DC_RFC          (B_RFC)
#define DC_LOWER        (B_RFC)
#define DC_UPPER        (B_UPPER | B_RFC)
#define DC_NUMBER       (B_NUMBER | B_RFC)
#define DC_NON_RFC      (B_NON_RFC)

#define DC_UTF8_TRAIL   (B_UTF8_TRAIL)
#define DC_UTF8_1ST_2   (B_UTF8_FIRST_TWO)
#define DC_UTF8_1ST_3   (B_UTF8_FIRST_THREE)
#define DC_UTF8_PAIR    (B_UTF8_PAIR)

//
//  Special characters
//      * valid as single label wildcard
//      _ leading SRV record domain names
//      / in classless in-addr
//

#define DC_DOT          (B_SPECIAL | B_DOT)

#define DC_ASTERISK     (B_SPECIAL | B_LEADING_ONLY)

#define DC_UNDERSCORE   (B_SPECIAL | B_LEADING_ONLY)

#define DC_BACKSLASH    (B_SPECIAL)

//
//  More special
//  These have no special validations, but have special file
//  properties, so define to keep table in shape for merge with
//  file chars.
//

#define DC_NULL         (0)

#define DC_OCTAL        (B_NON_RFC)
#define DC_RETURN       (B_NON_RFC)
#define DC_NEWLINE      (B_NON_RFC)
#define DC_TAB          (B_NON_RFC)
#define DC_BLANK        (B_NON_RFC)
#define DC_QUOTE        (B_NON_RFC)
#define DC_SLASH        (B_NON_RFC)
#define DC_OPEN_PAREN   (B_NON_RFC)
#define DC_CLOSE_PAREN  (B_NON_RFC)
#define DC_COMMENT      (B_NON_RFC)



//
//  DNS character table
//
//  These routines handle the name conversion issues relating to
//  writing names and strings in flat ANSI files
//      -- special file characters
//      -- quoted string
//      -- character quotes for special characters and unprintable chars
//
//  The character to char properties table allows simple mapping of
//  a character to its properties saving us a bunch of compare\branch
//  instructions in parsing file names\strings.
//

    const DWORD DnsCharPropertyTable[] =
    {
        //  control chars 0-31 must be octal in all circumstances
        //  end-of-line and tab characters are special

        DC_NULL,                // zero special on read, some RPC strings NULL terminated

        DC_OCTAL,   DC_OCTAL,   DC_OCTAL,   DC_OCTAL,
        DC_OCTAL,   DC_OCTAL,   DC_OCTAL,   DC_OCTAL,

        DC_TAB,                 // tab
        DC_NEWLINE,             // line feed
        DC_OCTAL,
        DC_OCTAL,
        DC_RETURN,              // carriage return
        DC_OCTAL,
        DC_OCTAL,

        DC_OCTAL,   DC_OCTAL,   DC_OCTAL,   DC_OCTAL,
        DC_OCTAL,   DC_OCTAL,   DC_OCTAL,   DC_OCTAL,
        DC_OCTAL,   DC_OCTAL,   DC_OCTAL,   DC_OCTAL,
        DC_OCTAL,   DC_OCTAL,   DC_OCTAL,   DC_OCTAL,

        DC_BLANK,               // blank, special char but needs octal quote

        DC_NON_RFC,             // !
        DC_QUOTE,               // " always must be quoted
        DC_NON_RFC,             // #
        DC_NON_RFC,             // $
        DC_NON_RFC,             // %
        DC_NON_RFC,             // &
        DC_NON_RFC,             // '

        DC_OPEN_PAREN,          // ( datafile line extension
        DC_CLOSE_PAREN,         // ) datafile line extension
        DC_ASTERISK,            // *
        DC_NON_RFC,             // +
        DC_NON_RFC,             // ,
        DC_RFC,                 // - RFC for hostname
        DC_DOT,                 // . must quote in names
        DC_BACKSLASH,           // /

        // 0 - 9 RFC for hostname

        DC_NUMBER,  DC_NUMBER,  DC_NUMBER,  DC_NUMBER,
        DC_NUMBER,  DC_NUMBER,  DC_NUMBER,  DC_NUMBER,
        DC_NUMBER,  DC_NUMBER,

        DC_NON_RFC,             // :
        DC_COMMENT,             // ;  datafile comment
        DC_NON_RFC,             // <
        DC_NON_RFC,             // =
        DC_NON_RFC,             // >
        DC_NON_RFC,             // ?
        DC_NON_RFC,             // @

        // A - Z RFC for hostname

        DC_UPPER,   DC_UPPER,   DC_UPPER,   DC_UPPER,
        DC_UPPER,   DC_UPPER,   DC_UPPER,   DC_UPPER,
        DC_UPPER,   DC_UPPER,   DC_UPPER,   DC_UPPER,
        DC_UPPER,   DC_UPPER,   DC_UPPER,   DC_UPPER,
        DC_UPPER,   DC_UPPER,   DC_UPPER,   DC_UPPER,
        DC_UPPER,   DC_UPPER,   DC_UPPER,   DC_UPPER,
        DC_UPPER,   DC_UPPER,

        DC_NON_RFC,             // [
        DC_SLASH,               // \ always must be quoted
        DC_NON_RFC,             // ]
        DC_NON_RFC,             // ^
        DC_UNDERSCORE,          // _
        DC_NON_RFC,             // `

        // a - z RFC for hostname

        DC_LOWER,   DC_LOWER,   DC_LOWER,   DC_LOWER,
        DC_LOWER,   DC_LOWER,   DC_LOWER,   DC_LOWER,
        DC_LOWER,   DC_LOWER,   DC_LOWER,   DC_LOWER,
        DC_LOWER,   DC_LOWER,   DC_LOWER,   DC_LOWER,
        DC_LOWER,   DC_LOWER,   DC_LOWER,   DC_LOWER,
        DC_LOWER,   DC_LOWER,   DC_LOWER,   DC_LOWER,
        DC_LOWER,   DC_LOWER,

        DC_NON_RFC,             // {
        DC_NON_RFC,             // |
        DC_NON_RFC,             // }
        DC_NON_RFC,             // ~
        DC_OCTAL,               // 0x7f DEL code

        //  UTF8 trail bytes
        //      - chars   0x80 <= X < 0xc0
        //      - mask [10xx xxxx]
        //
        //  Lead UTF8 character determines count of bytes in conversion.
        //  Trail characters fill out conversion.

        DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,
        DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,
        DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,
        DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,
        DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,
        DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,
        DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,
        DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,

        DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,
        DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,
        DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,
        DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,
        DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,
        DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,
        DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,
        DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,

        //  UTF8_1ST_OF_2
        //      - chars > 0xc0 to 0xdf
        //      - mask [110x xxxx]
        //
        //  Converting unicode chars > 7 bits <= 11 bits (from 0x80 to 0x7ff)
        //  consists of first of two char followed by one trail bytes

        DC_UTF8_1ST_2,  DC_UTF8_1ST_2,  DC_UTF8_1ST_2,  DC_UTF8_1ST_2,
        DC_UTF8_1ST_2,  DC_UTF8_1ST_2,  DC_UTF8_1ST_2,  DC_UTF8_1ST_2,
        DC_UTF8_1ST_2,  DC_UTF8_1ST_2,  DC_UTF8_1ST_2,  DC_UTF8_1ST_2,
        DC_UTF8_1ST_2,  DC_UTF8_1ST_2,  DC_UTF8_1ST_2,  DC_UTF8_1ST_2,
        DC_UTF8_1ST_2,  DC_UTF8_1ST_2,  DC_UTF8_1ST_2,  DC_UTF8_1ST_2,
        DC_UTF8_1ST_2,  DC_UTF8_1ST_2,  DC_UTF8_1ST_2,  DC_UTF8_1ST_2,
        DC_UTF8_1ST_2,  DC_UTF8_1ST_2,  DC_UTF8_1ST_2,  DC_UTF8_1ST_2,
        DC_UTF8_1ST_2,  DC_UTF8_1ST_2,  DC_UTF8_1ST_2,  DC_UTF8_1ST_2,

        //  UTF8_1ST_OF_3
        //      - chars > 0xe0
        //      - mask [1110 xxxx]
        //
        //  Converting unicode > 11 bits (0x7ff)
        //  consists of first of three char followed by two trail bytes

        DC_UTF8_1ST_3,  DC_UTF8_1ST_3,  DC_UTF8_1ST_3,  DC_UTF8_1ST_3,
        DC_UTF8_1ST_3,  DC_UTF8_1ST_3,  DC_UTF8_1ST_3,  DC_UTF8_1ST_3,
        DC_UTF8_1ST_3,  DC_UTF8_1ST_3,  DC_UTF8_1ST_3,  DC_UTF8_1ST_3,
        DC_UTF8_1ST_3,  DC_UTF8_1ST_3,  DC_UTF8_1ST_3,  DC_UTF8_1ST_3,
        DC_UTF8_1ST_3,  DC_UTF8_1ST_3,  DC_UTF8_1ST_3,  DC_UTF8_1ST_3,
        DC_UTF8_1ST_3,  DC_UTF8_1ST_3,  DC_UTF8_1ST_3,  DC_UTF8_1ST_3,
        DC_UTF8_1ST_3,  DC_UTF8_1ST_3,  DC_UTF8_1ST_3,  DC_UTF8_1ST_3,
        DC_UTF8_1ST_3,  DC_UTF8_1ST_3,  DC_UTF8_1ST_3,  DC_UTF8_1ST_3
    };

    bool Dns_ValidateUtf8Byte(
        __in        BYTE            chUtf8,
        __inout     PDWORD          pdwTrailCount
    )
    {
        DWORD   trailCount = *pdwTrailCount;

        //
        //  if ASCII byte, only requirement is no trail count
        //

        if ((UCHAR)chUtf8 < 0x80)
        {
            if (trailCount == 0)
            {
                return true;
            }
            return false;
        }

        //
        //  trail byte
        //      - must be in multi-byte set
        //

        if (BIT6(chUtf8) == 0)
        {
            if (trailCount == 0)
            {
                return false;
            }
            --trailCount;
        }

        //
        //  multi-byte lead byte
        //      - must NOT be in existing multi-byte set
        //      - verify valid lead byte

        else
        {
            if (trailCount != 0)
            {
                return false;
            }

            //  first of two bytes (110xxxxx)

            if (BIT5(chUtf8) == 0)
            {
                trailCount = 1;
            }

            //  first of three bytes (1110xxxx)

            else if (BIT4(chUtf8) == 0)
            {
                trailCount = 2;
            }

            //  first of four bytes (surrogate character) (11110xxx)

            else if (BIT3(chUtf8) == 0)
            {
                trailCount = 3;
            }

            else
            {
                return false;
            }
        }

        //  reset caller's trail count

        *pdwTrailCount = trailCount;

        return true;
    }

    DNSNAME_STATUS IsDnsNameValidInternal(
        __in LPCWSTR wszName
    )
    {
        const int nameLength = (int)wcslen(wszName);

        char buffer[2 * DNS_MAX_NAME_LENGTH];
        int length = WideCharToMultiByte(
            CP_UTF8, 0,
            wszName, nameLength,
            buffer, ARRAYSIZE(buffer),
            nullptr, nullptr
        );

        if ((length == 0) || (length > DNS_MAX_NAME_LENGTH))
        {
            return DNSNAME_INVALID_NAME;
        }

        buffer[length] = '\0';

        // "." root valid
        if ((length == 1) && (buffer[0] == '.'))
        {
            return DNSNAME_VALID;
        }

        DWORD       trailCount = 0;
        INT         labelCharCount = 0;
        INT         labelNumberCount = 0;
        BOOL        fnonRfc = FALSE;
        BOOL        finvalidChar = FALSE;

        //
        //  validations
        //      - name length (255)
        //      - label length (63)
        //      - UTF8 encoding correct
        //      - no unprintable characters
        //

        for (int i = 0; i < length; i++)
        {
            labelCharCount++;

            UCHAR ch = (UCHAR)buffer[i];
            DWORD chProp = DnsCharPropertyTable[ch];

            if (ch >= 0x80)
            {
                if (!Dns_ValidateUtf8Byte(ch, &trailCount))
                {
                    return DNSNAME_INVALID_NAME;
                }

                continue;
            }

            //
            //  trail count check: all ASCII chars, must not be in middle of UTF8
            //
            if (trailCount)
            {
                return DNSNAME_INVALID_NAME;
            }

            //  full RFC -- continue
            if (chProp & B_RFC)
            {
                if (chProp & B_NUMBER)
                {
                    labelNumberCount++;
                }
                continue;
            }

            //  label termination, terminate on dot
            //  - ".." or ".xyz" cases invalid
            if (ch == '.')
            {
                labelCharCount--;

                if (labelCharCount == 0)
                {
                    return DNSNAME_INVALID_NAME;
                }

                if (labelCharCount > DNS_MAX_LABEL_LENGTH)
                {
                    return DNSNAME_INVALID_NAME;
                }

                labelCharCount = 0;
                labelNumberCount = 0;
                continue;
            }

            //
            //  non-RFC
            //      - currently accepting only "_" as allowable as part of
            //      microsoft acceptable non-RFC set
            //
            //  underscore
            //      - can be valid as part of SRV domain name
            //      - otherwise non-RFC

            if (ch == '_')
            {
                fnonRfc = TRUE;
                continue;
            }


            //  anything else is complete junk
            fnonRfc = TRUE;
            finvalidChar = TRUE;
            continue;
        }

        if (labelCharCount > DNS_MAX_LABEL_LENGTH)
        {
            return DNSNAME_INVALID_NAME;
        }

        if (finvalidChar)
        {
            return DNSNAME_INVALID_NAME_CHAR;
        }

        if (fnonRfc)
        {
            return DNSNAME_NON_RFC_NAME;
        }

        return DNSNAME_VALID;
    }
}

DNS::DNSNAME_STATUS DNS::IsDnsNameValid(
    __in LPCWSTR wszName
)
{
#if defined(PLATFORM_UNIX)
    return IsDnsNameValidInternal(wszName);
#else
    // Check the result against DNSAPI which is always right due to backwards compatibility
    // Eventually remove DNSAPI once parity is proven. 
    DNS_STATUS dnsStatus = DnsValidateName(wszName, DnsNameHostnameFull);
    switch (dnsStatus)
    {
    case ERROR_SUCCESS:
        return DNSNAME_VALID;
    case DNS_ERROR_NON_RFC_NAME:
        return DNSNAME_NON_RFC_NAME;
    case DNS_ERROR_INVALID_NAME_CHAR:
        return DNSNAME_INVALID_NAME_CHAR;
    }
    return DNSNAME_INVALID_NAME;
#endif
}
