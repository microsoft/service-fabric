/*++

    (c) 2012 by Microsoft Corp. All Rights Reserved.

    kstringview.h

    Description:
      Kernel Tempate Library (KTL): KStringView prototype definition - both CHAR (Ansi) and WCHAR supported

      String manipulation & conversion utilities

      KStringView(A)   - view only, doesn't own memory
      KLocalString(A)  - template-based, object must be stack/struct local, will consume stack/struct space
      KDynString(A)    - buffer is on the heap, but object itself must be stack/struct local
      KString(A)       - Shared pointer, dynamic buffer
      KStringPool(A)   - KStringView(A) pool

      NOTE: The (A) form of the types support the Ansi (8-bit CHAR) string semantics. Not all methods are support
            for the Ansi types - e.g. To/From Ansi

      Null termination is not required for perf reasons.
      When interoperating with other APIs that require null termination,
      be sure to use IsNullTerminated() to query and SetNullTerminator()
      to ensure the terminator is present, assuming the buffer
      is large enough to accommodate it.

    History:
      raymcc          27-Feb-2012         Initial version.

    TBD:
      wmemcmp() for comparison
      Normalize LONGLONG vs. ULONGLONG for system time/duration
      Consider if the use of Reverse() in the scalar From*() conversions is best approach 
--*/

// NOTE: Should only be included via kstringview.h>

// Type parameter defs
#if defined(K$AnsiStringTarget)
#define K$KStringView KStringViewA
#define K$KStringViewTokenizer KStringViewTokenizerA
#define K$KStringPool KStringPoolA
#define K$KLocalString KLocalStringA
#define K$KDynString KDynStringA
#define K$KString KStringA
#define K$KBufferString KBufferStringA
#define K$KSharedBufferString KSharedBufferStringA
#define K$CHAR CHAR
#define K$CHARSIZE 1
#define K$LPCSTR LPCSTR
#define K$PCHAR PCHAR
#define K$STRING(s) s
#define K$StringCompare strncmp


#else
#define K$KStringView KStringView
#define K$KStringViewTokenizer KStringViewTokenizer
#define K$KStringPool KStringPool
#define K$KLocalString KLocalString
#define K$KDynString KDynString
#define K$KString KString
#define K$KBufferString KBufferString
#define K$KSharedBufferString KSharedBufferString
#define K$CHAR WCHAR
#define K$CHARSIZE 2
#define K$LPCSTR LPCWSTR
#define K$PCHAR PWCHAR
#define K$STRING(s) L##s
#define K$StringCompare wcsncmp
#endif


class K$KStringView
{
    friend class K$KStringPool;

public:
    static const ULONG MaxIso8601DateTime = 24;
    static const ULONG MaxIso8601Duration = 64;
    static const ULONG Max64BitNumString  = 24;
    static const ULONG Max32BitNumString  = 12;
    static const ULONG MaxBooleanString   =  8;
    static const ULONG MaxGuidString      = 40;
    static const ULONG GuidLengthInChars  = 38;
    static const ULONG MaxStandardType    = 64; // Largest of the above for properly sized temp work buffers used for conversion
    static const ULONG MaxLength          = MAXLONG;


    // KStringView
    //
    // Default constructor.  Sets the internal fields to zero/null.
    //
    K$KStringView()
    {
        Zero();
    }

    // KStringView
    //
    // Constructor which takes a supplied buffer. The null terminator
    // is not required and it is legal to exclude or include it in the length.
    //
    // Parameters:
    //      Buffer               Points to the WCHAR array. This may be null.
    //      BufLenInChars        The size of the character array in characters (not bytes).
    //      StringLengthInChars  Number of characters in the string.
    //
    // If the string length is not known, the default value of the 3rd parameter
    // can be left at zero, or explicitly set to zero.
    //
    // A subsequent call to MeasureThis() will scan for the NULL and establish the
    // string length.
    //
    K$KStringView(
        __in_ecount(BufLenChars) K$PCHAR Buffer,
        __in ULONG   BufLenChars,
        __in ULONG   StringLengthInChars = 0
        )
    {
        KInvariant(StringLengthInChars <= MaxLength);
        KInvariant(BufLenChars <= MaxLength);

        _Buffer = Buffer;
        _BufLenInChars = BufLenChars;
        _LenInChars = StringLengthInChars;
        _UseShortBuffer = FALSE;
    }

    //  KStringView
    //
    //  This overload is intended primarily for compile-time string constants (quoted strings).
    //  If not null, this will measure the string at construct-time, assuming there is a
    //  null terminator.
    //
    //  Typical use
    //              KStringView Sample(L"Quoted String");
    //
    //  Note that this will set the buffer length to one larger than the string length.
    //
    //  Parameters:
    //      ConstStr        May be null.  If not null, there MUST be a NULL terminator for the string.
    //                      The null terminator will be included in the computed buffer length.
    //
    K$KStringView(
        __in_z K$LPCSTR ConstStr
        )
    {
        Zero();
        _Buffer = K$PCHAR(ConstStr);
        if (_Buffer)
        {
            _BufLenInChars = MaxLength;
            KInvariant(MeasureThis());
            _BufLenInChars = _LenInChars + 1;
        }
    }

#if !defined(K$AnsiStringTarget)
    // KStringView
    //
    // Constructs from an external UNICODE_STRING.  This will convert the byte-oriented count to
    // the character-oriented count in KStringView.
    //
    // If the last character is null, then the internal buffer size will be adjusted to not
    // include it, since nulls are not maintained by KStringView.
    //
    K$KStringView(
        __in const UNICODE_STRING& Src
        )
    {
        Zero();
        if (Src.Buffer)
        {
            _LenInChars = Src.Length / K$CHARSIZE;
            _BufLenInChars = Src.MaximumLength / K$CHARSIZE;
            _Buffer = Src.Buffer;
            KAssert(_BufLenInChars <= MaxLength);
        }
    }
#endif

    // Copy constructor
    //
    // Copies the fields of the referenced KStringView, but does not make a copy of the string.
    //
    K$KStringView(
        __in K$KStringView const & Src
        )
    {
        Zero();
        *this = Src;
    }


    // Copy constructor with offset
    //
    // Constructs a new string based on an offset into the source. Used
    // for building a view of the right-hand side of the string, such as when tokenizing.
    //
    // Parameters:
    //      Src         The source string
    //      Offset      How far into the string to construct the new view
    //
    K$KStringView(
        __in K$KStringView const & Src,
        __in ULONG Offset
        )
    {
        Zero();
        *this = Src;
        ResetOrigin(Offset);
    }


    //  operator =
    //
    //  This copies a referece to the string and does not allocate memory or make a literal
    //  copy of the string.
    //
    K$KStringView& operator=(
        __in K$KStringView const & Src
        )
    {
        _UseShortBuffer = FALSE;
        _Buffer = Src._Buffer;
        _LenInChars = Src._LenInChars;
        _BufLenInChars = Src._BufLenInChars;
        return *this;
    }


    // Destructor
    //
    // Does not deallocate the memory.
    //
   ~K$KStringView()
    {
        Zero();
    }


    // Set
    //
    // Used to set all the values.  This is used primarily for _out parameters acting as return values.
    //
    VOID
    Set(
        __in K$PCHAR Address,
        __in ULONG   LengthChars,
        __in ULONG   BufLengthChars
        )
    {
        KInvariant(LengthChars <= MaxLength);
        KInvariant(BufLengthChars <= MaxLength);

        _UseShortBuffer = FALSE;
        _Buffer = Address;
        _LenInChars = LengthChars;
        _BufLenInChars = BufLengthChars;
    }

    // Length
    //
    // Returns the length of the string in code units.
    //
    ULONG
    Length() const
    {
        return _LenInChars;
    }


    // LengthInBytes
    //
    // Returns the length of the string in bytes (char * K$CHARSIZE)
    //
    ULONG
    LengthInBytes() const
    {
        return _LenInChars * K$CHARSIZE;
    }

    // Clear
    //
    // Sets string length to zero.
    //
    VOID
    Clear()
    {
        _LenInChars = 0;
    }

    // CopyFrom
    //
    // Copies from the specified source to the buffer pointed to by 'this'.
    // This is distinct from the constructor, which merely takes a reference
    // to the source string.
    //
    // Parameters:
    //      Src                 Source string view; must not be an empty string.
    //      SetNull             If TRUE, the null terminator will be set as part of the copy,
    //                          (whether or not the source has one) provided
    //                          the buffer size is large enough.
    //
    // BOOLEAN
    //      TRUE on success
    //      FALSE if there was insufficient buffers space in the current string to receive the buffer,
    //      set the null, or the source string was empty.
    //
    BOOLEAN
    CopyFrom(
        __in K$KStringView const & Src,
        __in BOOLEAN SetNull = FALSE
        )
    {
        if (IsNull() || Src.IsNull() || (Src._LenInChars > _BufLenInChars))
        {
            return FALSE;
        }
        else
        {
            _LenInChars = Src._LenInChars;
        }

        KMemCpySafe(_Buffer, _BufLenInChars*K$CHARSIZE, Src._Buffer, Src._LenInChars*K$CHARSIZE);

        if (SetNull)
        {
            return SetNullTerminator();
        }
        return TRUE;
    }

#if !defined(K$AnsiStringTarget)
    // CopyFromAnsi
    //
    // Copies to the current buffer from an ANSI source buffer, converting to UNICODE (en-us) in the process.
    //
    // This does not convert foreign code pages.
    //
    // Parameters:
    //      Source          An ANSI character buffer/string.
    //      CharsToCopy     Number of characters to copy.
    //
    // Return value:
    //
    //  TRUE on success, FALSE if the current KStringView does not have sufficient buffer space
    //  to receive the string.
    //
    BOOLEAN
    CopyFromAnsi(
        __in_ecount_opt(CharsToCopy) CHAR const * Source,
        __in ULONG CharsToCopy
        )
    {
        if (_BufLenInChars < CharsToCopy)
        {
            return FALSE;
        }
        if (Source == nullptr || CharsToCopy == 0)
        {
            _LenInChars = 0;
            return TRUE;
        }
        ULONG Ix = 0;
        _LenInChars = CharsToCopy;
        while (CharsToCopy--)
        {
            _Buffer[Ix] = WCHAR(Source[Ix]);
            Ix++;
        }

        return TRUE;
    }

    // CopyToAnsi
    //
    // Copies to the current buffer to an ANSI buffer, converting from UNICODE to ANSI (en-us) in the process.
    //
    // This does not convert foreign code pages.
    //
    // Parameters:
    //      Dest            An ANSI character buffer.
    //      CharsToCopy     Number of characters to copy.
    //
    // Return value:
    //
    //  TRUE on success, FALSE if the current KStringView does not have sufficient buffer space
    //  to receive the string.
    //
    BOOLEAN
    CopyToAnsi(
        __out_ecount_opt(CharsToCopy) CHAR* Dest,
        __in ULONG CharsToCopy
        ) const
    {
        if (IsEmpty() || Dest == nullptr || CharsToCopy == 0)
        {
            return FALSE;
        }
        ULONG Ix = 0;
        while (CharsToCopy--)
        {
            Dest[Ix] = (CHAR) _Buffer[Ix];
            Ix++;
        }

        return TRUE;
    }
#endif


    //
    //  AddressOf
    //
    //  This is a parsing helper. It returns a KStringView
    //  whose buffer address is set to the specified offset into the
    //  current string's buffer.
    //
    //  This is distinct from the copy constructor overload in that
    //  it does not modify the length or buffer size.
    //
    //  This is intended for making one KStringView over the current string and
    //  for use with IncLen().
    //
    //  Parameters:
    //      Offset          The offset into the current string.
    //
    //  Return value:
    //      A new KStringView which is either Empty/Null or whose
    //      buffer is set to the specified offset in the current string,
    //      but whose length and buffer length are still zero.
    //
    K$KStringView
    AddressOf(
        __in ULONG Offset
        ) const
    {
        K$KStringView Ret;
        if (Offset < _BufLenInChars)
        {
            Ret._Buffer = &_Buffer[Offset];
        }
        return Ret;
    }

    // IncLen
    //
    // Parser helper.
    //
    // Increments the length & buffer length of the current string.
    //
    // A parsing helper intended for supervised use of one KStringView
    // over the top of another.  This simply increments the length
    // and buffer length, and assumes the _Buffer address is correct.
    //
    // This is used in combination with AddressOf.
    // This must be carefully used as there is no error checking.
    //
    VOID
    IncLen()
    {
        KAssert(!_UseShortBuffer || (_BufLenInChars < (ARRAYSIZE(_ShortBuffer) - 1)));
        _BufLenInChars++;
        _LenInChars++;
    }

    VOID
    IncLen(
        __in ULONG Total
        )
    {
        while (Total--)
        {
            IncLen();
        }
    }

#if !defined(K$AnsiStringTarget)
    // SkipBOM
    //
    // Adjusts the current string to begin past the BOM, if it is present.  The starting
    // address of the string, the length, and buffer size are all adjusted accordingly.
    //
    // Return value:
    //      Return TRUE if a BOM was detected and skipped, FALSE if not.
    //
    BOOLEAN
    SkipBOM()
    {
        if (IsNull() || _LenInChars == 0)
        {
            return FALSE;
        }
        if (_Buffer[0] == 0xFEFF)
        {
            ConsumeChar();
        }
        return TRUE;
    }

    // HasBOM
    //
    // Returns TRUE if the current KStringView begins with a BOM.
    //
    BOOLEAN
    HasBOM() const
    {
        if (IsNull() || _LenInChars == 0)
        {
            return FALSE;
        }
        if (_Buffer[0] == 0xFEFF)
        {
            return TRUE;
        }
        return FALSE;
    }

    // EnsureBOM
    //
    // Ensures that the first character is a BOM.  This will
    // test for the existence of a BOM and if it is not present
    // will set or insert it without ovewriting any existing
    // string data.
    //
    // Returns:
    //   TRUE if a BOM is verified as being present before returning.
    //   FALSE if there was insufficient room to add a BOM.
    //
    BOOLEAN
    EnsureBOM()
    {
        if (HasBOM())
        {
            return TRUE;
        }

        // Return FALSE is there is no place to put a BOM.
        //
        if (IsNull() || _BufLenInChars == 0 || _LenInChars == _BufLenInChars)
        {
            return FALSE;
        }

        return PrependChar(WCHAR(0xFEFF));
    }
#endif


    // SetNullTerminator
    //
    // Sets a null terminator on the current string, if there is sufficient space.
    //
    // Return values:
    //      TRUE    If there is room and the null terminator was set.
    //      FALSE   If the buffer was too small or null.
    //
    BOOLEAN
    SetNullTerminator()
    {
        if (IsNull() || FreeSpace() == 0)
        {
            return FALSE;
        }
        _Buffer[_LenInChars] = 0;
        return TRUE;
    }


    // IsNullTerminated
    //
    // Returns TRUE if the string is null terminated (there is a null char within the buffer
    // after the end of the string), FALSE if not.
    //
    BOOLEAN
    IsNullTerminated() const
    {
        if (IsNull() || _BufLenInChars == 0 || _BufLenInChars == _LenInChars || _Buffer[_LenInChars] != 0)
        {
            return FALSE;
        }
        return TRUE;
    }

    //
    //  AppendChar
    //
    //  Appends a character to the end of the string.
    //
    //  Parameters:
    //      Char            The character to append.
    //
    //  Returns TRUE on success, FALSE if there was insufficient space to append the character.
    //
    //
    BOOLEAN
    AppendChar(
        __in K$CHAR Char
        )
    {
        if (IsNull() || _LenInChars == _BufLenInChars)
        {
            return FALSE;
        }
        KAssert(_BufLenInChars > 0);
        _Buffer[_LenInChars++] = Char;
        return TRUE;
    }



    // PrependChar
    //
    // Prepends a character onto the string, if there is sufficient space.
    //
    // Parameter:
    //      Char        The character to prepend.
    //
    // Returns TRUE on success, FALSE if there was insufficient buffer space.
    //
    BOOLEAN
    PrependChar(
        __in K$CHAR Char
        )
    {
        if (IsNull() || _LenInChars == _BufLenInChars)
        {
            return FALSE;
        }
        KAssert(_BufLenInChars > 0);
        RtlMoveMemory(&_Buffer[1], &_Buffer[0], _LenInChars*K$CHARSIZE);
        _Buffer[0] = Char;
        _LenInChars++;
        return TRUE;
    }

    // Prepend
    //
    // Prepends the specified string to the current string
    //
    BOOLEAN
    Prepend(
        __in K$KStringView const & Src
        )
    {
        if (Src.IsEmpty())
        {
            return TRUE;
        }
        if (IsNull() || _LenInChars == _BufLenInChars)
        {
            return FALSE;
        }

        ULONG RequiredSize = Src._LenInChars + _LenInChars;
        if (_BufLenInChars < RequiredSize)
        {
            return FALSE;
        }

        RtlMoveMemory(&_Buffer[Src._LenInChars], &_Buffer[0], _LenInChars*K$CHARSIZE);
        RtlMoveMemory(&_Buffer[0], &Src._Buffer[0], Src._LenInChars*K$CHARSIZE);

        _LenInChars += Src._LenInChars;

        return TRUE;
    }

    // operator K$LPCSTR
    //
    //
    operator K$LPCSTR() const
    {
        return _Buffer;
    }

    // operator K$PCHAR
    //
    //
    operator K$PCHAR() const
    {
        return _Buffer;
    }


    // operator PVOID
    //
    operator PVOID() const
    {
        return _Buffer;
    }


#if !defined(K$AnsiStringTarget)
    // IsUNICODE_STRINGCompatible - Validates that the current state can be represented by a UNICODE_STRING
    // structure
    //
    //  Returns TRUE if compatible
    BOOLEAN
    IsUNICODE_STRINGCompatible() const
    {
        return (_LenInChars <= ((ULONG)MAXUSHORT * K$CHARSIZE)) &&
               (_BufLenInChars <= ((ULONG)MAXUSHORT * K$CHARSIZE));
    }

    // ToUNICODE_STRING - Return an UNICODE_STRING referencing this instances state
    //
    // NOTE: Warning: Only works if Length() <= 32K -
    //
    UNICODE_STRING
    ToUNICODE_STRING() const
    {
        UNICODE_STRING temp;

        if (!IsUNICODE_STRINGCompatible())
        {
            temp.Buffer = nullptr;
            temp.Length = temp.MaximumLength = 0;
            return temp;
        }

        temp.Buffer = _Buffer;
        temp.Length = (USHORT)_LenInChars * K$CHARSIZE;
        temp.MaximumLength = (USHORT)_BufLenInChars * K$CHARSIZE;
        return temp;
    }
#endif

    // IsNull
    //
    // Returns TRUE if there is no internal buffer mapped.
    //
    //
    BOOLEAN
    IsNull() const
    {
        return _Buffer == nullptr;
    }

    // IsEmpty
    //
    // Returns null if there is no internal buffer or
    // if there is a buffer but the string length is zero.
    //
    BOOLEAN
    IsEmpty() const
    {
        if (_Buffer == nullptr || _LenInChars == 0)
        {
            return TRUE;
        }
        return FALSE;
    }

    // FreeSpace
    //
    // Returns the amount of unused characters left in the buffer.
    //
    ULONG
    FreeSpace() const
    {
        KAssert(_BufLenInChars >= _LenInChars);
        return _BufLenInChars - _LenInChars;
    }


    // operator[]
    //
    // Parser helper accessor.
    //
    // Parameters:
    //      Offset          Offset into the string for which to return the WCHAR.
    //
    // Returns 0 if out of range.  This is used to normalize loop-based testing
    // for at-end-of-string conditions.
    //
    //
    K$CHAR
    operator[](__in ULONG Offset) const
    {
        if (IsEmpty() || Offset >= _LenInChars)
        {
            return 0;
        }
        return _Buffer[Offset];
    }


    //  Find
    //
    //  Scans the current string for the target character.
    //
    //  Parameters:
    //      Target          The character we are trying to find.
    //      Pos             The offset at which the target was found, if it was found.
    //
    //  Return value:
    //      TRUE            If the character was found
    //      FALSE           If the character was not found.
    //
    //
    BOOLEAN
    Find(
        __in K$CHAR  Target,
        __out ULONG& Pos
        ) const
    {
        K$PCHAR Scanner = _Buffer;
        while ((ULONG)(Scanner - _Buffer) < _LenInChars)
        {
            if (*Scanner == Target)
            {
                Pos = ULONG(Scanner - _Buffer);
                return TRUE;
            }
            Scanner++;
        }
        return FALSE;
    }


    //  Find Reverse
    //
    //  Scans the current string for the target character.
    //
    //  Parameters:
    //      Target          The character we are trying to find.
    //      Pos             The offset at which the target was found, if it was found.
    //
    //  Return value:
    //      TRUE            If the character was found
    //      FALSE           If the character was not found.
    //
    //
    BOOLEAN
    RFind(
        __in K$CHAR  Target,
        __in ULONG& Pos
        ) const
    {
        if (_LenInChars == 0)
        {
            return FALSE;
        }

        K$PCHAR const   bufferPtr = _Buffer;
        K$PCHAR         scannerPtr = &bufferPtr[_LenInChars - 1];
        while (scannerPtr >= bufferPtr)
        {
            if (*scannerPtr == Target)
            {
                Pos = ULONG(scannerPtr - bufferPtr);
                return TRUE;
            }
            scannerPtr--;
        }
        return FALSE;
    }


    // Search
    //
    // Search for the first occurrence of a substring within a string.
    //
    // Parameters:
    //  Target      The string to search for
    //  Pos         Receive the offset at which the string was found, if TRUE is returned.
    //  StartPos    The offset at which to start the search.  Default to zero.
    //
    // Return value:
    //      TRUE if the string was found.
    //
    BOOLEAN
    Search(
        __in K$KStringView const & Target,
        __out ULONG& Pos,
        __in ULONG  StartPos = 0
        ) const;

    // Search Reverse
    //
    // Search for the last occurrence of a substring within a string.
    //
    // Parameters:
    //  Target      The string to search for
    //  Pos         Receive the offset at which the string was found, if TRUE is returned.
    //  LimitPos    The offset at which to include the search.  Default to entire string.
    //
    // Return value:
    //      TRUE if the string was found.
    //
    BOOLEAN
    RSearch(
        __in K$KStringView const& Target,
        __out ULONG& Pos,
        __in ULONG  LimitPos = MAXULONG
        ) const;

    // Replace
    //
    // Replaces the target string with the replacement string.
    //
    // Parameters:
    //      Target          The string to search for.
    //      Replacement     The string which replaces the target.
    //                      This may be empty/null, which means
    //                      that the target is simply removed.
    //
    //      ReplacmentCount Number of occurrences that were replaced.
    //
    //
    //      ReplaceAll      If TRUE, all occurrences of Target are
    //                      replaced.  If FALSE, only the first
    //                      occurrence is replaced.
    //
    //  Return value:
    //      TRUE on successful processing, even if no replacements
    //      have occurred.
    //
    //      FALSE occurs if the target string is NULL, the current
    //      string is null, or insufficient memory was present
    //      to accommodate replacement. All of these are basic failures.
    //
    //  Note: In FALSE is returned, the buffer will remain coherent.
    //        Not all replacements may have been made, but no fragmented subset
    //        replacements will occur which might garble the buffer.
    //
    BOOLEAN
    Replace(
        __in  K$KStringView const & Target,
        __in  K$KStringView const & Replacement,
        __out ULONG& ReplacementCount,
        __in  BOOLEAN ReplaceAll = TRUE
        );


    // ReplaceChar
    //
    // Replaces the character at the specified index.  Only fails
    // if the index is not valid, so TRUE/FALSE is an adequate
    // return value.
    //
    // Parameters:
    //      Offset          Which character to replace.
    //      Char            The new value
    //
    // Return value:
    //      TRUE on success, FALSE on offset error.
    //
    BOOLEAN
    ReplaceChar(
        __in ULONG Index,
        __in K$CHAR Char
        )
    {
        if (IsEmpty() || Index >= _LenInChars)
        {
            return FALSE;
        }
        _Buffer[Index] = Char;
        return TRUE;
    }

    //  Concat
    //
    //  Concatenates the supplied string onto the current string.
    //
    //  Parameters:
    //      Src     The string to be concatenated. May be null/empty, which still results
    //              in success with no change to the current string.
    //
    //  Return value:
    //      FALSE   Fails, only if there is insufficient buffer.
    //      TRUE    In all other cases, including if the Src is empty.
    //
    //
    //  Sample:
    //          KStringView x = ...;
    //          x.Concat(KStringView(L"Suffix"));
    //
    BOOLEAN
    Concat(
        __in K$KStringView const & Src
        )
    {
        if (Src._LenInChars + _LenInChars > _BufLenInChars)
        {
            return FALSE;
        }
        KMemCpySafe(&_Buffer[_LenInChars], (_BufLenInChars - _LenInChars) * K$CHARSIZE, Src._Buffer, Src._LenInChars*K$CHARSIZE);
        _LenInChars += Src._LenInChars;

        return TRUE;
    }

    // Concat (overload)
    //
    // Concatenates an K$LPCSTR, primarily intended for in-line quoted strings.
    // If there is insufficient buffer space, a partial copy can occur.
    //
    // Returns TRUE if the source string is zero length or null, as this is
    // considered 'successful' concatenation of nothing. Returns FALSE
    // only on insufficient buffer space to receive the string.
    //
    BOOLEAN
    Concat(
        __in K$LPCSTR Str
        )
    {
        if (!Str)
        {
            return TRUE;
        }
        if (IsNull())
        {
            return FALSE;
        }
        K$PCHAR Tracer = (K$PCHAR) Str;
        while (*Tracer)
        {
            if (FreeSpace() == 0)
            {
                return FALSE;
            }
            if (!AppendChar(*Tracer))
            {
                return FALSE;
            }
            Tracer++;
        }
        return TRUE;
    }

#if !defined(K$AnsiStringTarget)
    //  ConcatAnsi
    //
    //  Concatenates the supplied ANSI string onto the current string. This only
    //  works with the en-us code page within the ASCII subset.
    //
    //  In this case, NULL is considered a character to concatenate.
    //
    //  Parameters:
    //      Source         The character string sequence to be concatenated.
    //      CharsToConcat  Number of characters to concatenate.
    //
    //  Return value:
    //      FALSE   Fails, only if there is insufficient buffer.
    //      TRUE    In all other cases, including if the Src is empty.
    //
    //
    BOOLEAN
    ConcatAnsi(
        __in_ecount_opt(CharsToConcat) CHAR const * Source,
        __in ULONG CharsToConcat
        )
    {
        if (!Source)
        {
            return TRUE;
        }

        CHAR const * Tracer = Source;
        while (CharsToConcat--)
        {
            if (FreeSpace() == 0)
            {
                return FALSE;
            }
            if (! AppendChar((WCHAR) *Tracer))
            {
                return FALSE;
            }
            Tracer++;
        }
        return TRUE;
    }
#endif


    // Concat (overload)
    //
    //
    BOOLEAN
    Concat(
        __in K$CHAR c
        )
    {
        return AppendChar(c);
    }


    // PeekFirst
    //
    // Returns the first character of the current string, or 0/NULL if there
    // are no characters left or the string is null.
    //
    K$CHAR
    PeekFirst() const
    {
        if (_LenInChars == 0)
        {
            return 0;
        }
        KAssert(_Buffer);
        return _Buffer[0];
    }

    // PeekN
    //
    // Peeks the Nth character.  Returns NULL if attempt to peek past end.
    // This is used as a parser helper and its non-assert behavior on past-end conditions is required.
    //
    K$CHAR
    PeekN(
        __in ULONG Pos
        ) const
    {
        if (Pos >= _LenInChars)
        {
            return 0;
        }
        KAssert(_Buffer);
        return _Buffer[Pos];
    }


    // PeekLast
    //
    // Returns the last character of the current string, or 0/NULL if the string
    // is empty or null.
    //
    K$CHAR
    PeekLast() const
    {
        if (_LenInChars == 0)
        {
            return 0;
        }
        KAssert(_Buffer);
        return _Buffer[_LenInChars-1];
    }


    // TruncateLast
    //
    // Truncates the string by one character, if possible.  Returns TRUE
    // if a character was truncated, FALSE if the string was already zero length or null.
    //
    BOOLEAN
    TruncateLast()
    {
        if (_LenInChars == 0)
        {
            return FALSE;
        }
        _LenInChars--;
        return TRUE;
    }


    // ConsumeChar
    //
    // Move the origin of the string forward by one character, thus consuming it.
    // The character is not modified; the origin of the current StringView is simply
    // incremented.
    //
    // Returns TRUE if successful, including consuming the last character, or
    // FALSE if the string is already empty/null.
    //
    BOOLEAN
    ConsumeChar()
    {
        KInvariant(!_UseShortBuffer);
        if (IsEmpty())
        {
            return FALSE;
        }
        _Buffer++;
        _LenInChars--;
        _BufLenInChars--;
        return TRUE;
    }


    // ConsumeChars
    //
    // Consumes the specified number of characters.
    //
    BOOLEAN
    ConsumeChars(
        __in ULONG Total
        )
    {
        while (Total--)
        {
            if (ConsumeChar() == FALSE)
                return FALSE;
        }
        return TRUE;
    }

    // MatchUntil
    //
    // Begins matching all characters starting at the beginning of the string
    // until one of the characters in the terminator set is encountered or until
    // the end of the string was reached.
    //
    // Parameters:
    //      TerminatorSet       A set of characters which can be terminators.
    //      ResultString        Receives the newly detected string, if a match occurred.
    //                          This is a view over the string represented by *this
    //                          and is thus not null-terminatead.
    //
    // Returns:
    //      TRUE if a non-zero match occurred
    //      FALSE if no match occurred.
    //
    BOOLEAN
    MatchUntil(
        __in  K$KStringView const & TerminatorSet,
        __out K$KStringView& ResultString
        ) const;

    // MatchWhile
    //
    // Begins matching all characters in the acceptor set until one is encountered
    // which is not in that set.
    //
    // Parameters:
    //      AcceptorSet         A set of characters to be matched
    //      ResultString        Receives the newly detected string, if a match occurred.
    //                          This is a view over the string represented by *this
    //                          and is thus not null-terminatead.
    //
    // Returns:
    //      TRUE if a non-zero match occurred
    //      FALSE if no match occurred.
    //
    //
    BOOLEAN
    MatchWhile(
        __in  K$KStringView const & AcceptorSet,
        __out K$KStringView& ResultString
        ) const;

    // SubString
    //
    // Returns a KStringView of a substring of the current string.
    // If the specified parameters are incorrect, invalid, or the current string
    // is null or zero length, the resulting KStringView will also be null.
    //
    // Parameters:
    //      StartPos    Where to start in extracting the substring
    //      Count       How many characters are in the substring
    //
    // Return value:
    //      KStringView that is set to match the specified substring. This can be
    //      null if the substring was out of bounds or zero length.
    //
    K$KStringView
    SubString(
        __in ULONG StartPos,
        __in ULONG Count
        ) const
    {
        K$KStringView RetVal;
        if ((StartPos >= _LenInChars) || (Count > _LenInChars - StartPos) || Count == 0)
        {
            return RetVal;  // This is the zero-length default
        }
        RetVal._Buffer = &_Buffer[StartPos];
        RetVal._LenInChars = Count;
        RetVal._BufLenInChars = _BufLenInChars - StartPos;
        return RetVal;
    }

    // LeftString
    //
    // Returns the leftmost substring of Count characters.
    //
    // Parameters:
    //      Count   The N leftmost character count.
    //
    // The returned substring may be null/zero length.
    //
    K$KStringView
    LeftString(
        __in ULONG Count
        ) const
    {
        return SubString(0, Count);
    }

    // RightString
    //
    // Returns the rightmost substring of Count characters.
    //
    // Parameters:
    //      Count   The N rightmost character count.
    //
    // The returned substring may be null/zero length.
    //
    K$KStringView
    RightString(
        __in ULONG Count
        ) const
    {
        ULONG Start = _LenInChars - Count;
        return SubString(Start, Count);
    }


    // Remainder
    //
    // Returns the rest of the string (rightmost) starting at the offset.
    // Includes the value at the offset.
    //
    // Parameters:
    //      Offset  The starting point for the returned value.
    //
    K$KStringView
    Remainder(
        __in ULONG Offset
        ) const
    {
        ULONG Count = _LenInChars - Offset;
        return SubString(Offset, Count);
    }

    //  Insert
    //
    //  Modifies the current string by inserting a new substring at the specified position.
    //
    //  Parameters:
    //      Offset          The position at which to insert the new substring.
    //      ToBeInserted    The string to insert.
    //
    //  Return value:
    //      TRUE if the insertion succeeded and the string was modified.
    //      FALSE in all other cases, i.e., where the string is left unmodified.
    //
    BOOLEAN
    Insert(
        __in ULONG Offset,
        __in K$KStringView const & ToBeInserted
        )
    {
        if (IsNull() || ToBeInserted.IsNull() || ToBeInserted.Length() == 0 ||
            ToBeInserted._LenInChars + _LenInChars > _BufLenInChars ||
            Offset > _LenInChars)
        {
            return FALSE;
        }

        // Create the hole
        //
        if (Offset < _LenInChars)
        {
            ULONG BytesToCreateHole = (_LenInChars - Offset) * K$CHARSIZE;

            // Safe: Doing an intentional overlapped copy.
            RtlMoveMemory(&_Buffer[Offset + ToBeInserted._LenInChars], &_Buffer[Offset], BytesToCreateHole);
        }

        // Now copy into the hole
        // NOTE: The two KMemCpySafe()s below are safe because of the above test!!!
        KMemCpySafe(&_Buffer[Offset], ToBeInserted._LenInChars * K$CHARSIZE, ToBeInserted._Buffer, ToBeInserted._LenInChars * K$CHARSIZE);

        // Update the length
        //
        _LenInChars += ToBeInserted._LenInChars;

        return TRUE;
    }

    //  Insert
    //
    //  Modifies the current string by inserting a new set of chars at the specified position.
    //
    //  Parameters:
    //      Offset          The position at which to insert the new substring.
    //      ToBeInserted    The char to insert.
    //      Count           Number of ToBeInserted chars to be inserted
    //
    //  Return value:
    //      TRUE if the insertion succeeded and the string was modified.
    //      FALSE in all other cases, i.e., where the string is left unmodified.
    //
    BOOLEAN
    InsertChar(
        __in ULONG Offset,
        __in K$CHAR ToBeInserted,
        __in ULONG Count = 1
        )
    {
        if (IsNull() || (_LenInChars + Count) > _BufLenInChars || Offset > _LenInChars)
        {
            return FALSE;
        }

        // Create the hole
        //
        if (Offset < _LenInChars)
        {
            ULONG BytesToCreateHole = (_LenInChars - Offset) * K$CHARSIZE;
            RtlMoveMemory(&_Buffer[Offset + Count], &_Buffer[Offset], BytesToCreateHole);
        }

        // Now copy into the hole
        while (Count > 0)
        {
            _Buffer[Offset++] = ToBeInserted;
            _LenInChars++;
            Count--;
        }

        return TRUE;
    }

    // Reverse
    //
    // Reverses the string.  This is primarily a helper utility for numeric conversion.
    //
    // Always logically succeeds.
    //
    VOID
    Reverse()
    {
        if (IsNull() || _LenInChars == 0)
        {
            return;
        }
        ULONG First = 0;
        ULONG Last = _LenInChars - 1;
        while (First < Last)
        {
            K$CHAR Tmp = _Buffer[First];
            _Buffer[First] = _Buffer[Last];
            _Buffer[Last] = Tmp;
            First++;
            Last--;
        }
    }

    // Remove
    //
    // Modifies the current string by removing a subtring.  This involves a memory copy.
    //
    // Parameters:
    //      Offset      The point at which to begin removal
    //      Count       The number of characters to remove
    //
    // Return value:
    //      TRUE on success.
    //      FALSE if the string was not removed due to *this being null, a zero or invalid count or invalid offset
    //
    BOOLEAN
    Remove(
        __in ULONG Offset,
        __in ULONG Count
        )
    {
        if (IsNull() || Offset + Count > _LenInChars || Count == 0)
        {
            return FALSE;
        }
        RtlMoveMemory(&_Buffer[Offset], &_Buffer[Offset+Count], (_LenInChars - (Count + Offset)) * K$CHARSIZE);
        _LenInChars -= Count;
        return TRUE;
    }

    // Compare
    //
    // A case-sensitive comparison,
    //
    // Returns -1 if the current string is lexically smaller than the comparand.
    // Retursn +1 if the current string is lexically larger than the comparand.
    // Returns  0 if the current string is lexically identical to the comparand.
    //
    LONG
        Compare(
            __in K$KStringView const & Comparand
        ) const
    {
        if (Comparand.IsNull())
        {
            if (IsNull())
            {
                return 0;
            }
            else
            {
                return 1;
            }
        }

        // TBD: get strncmp to work in kernel mode
#if KTL_USER_MODE
        ULONG   todo = __min(_LenInChars, Comparand._LenInChars);
        LONG compResult = K$StringCompare(&_Buffer[0], &Comparand._Buffer[0], todo);
        if (compResult != 0)
        {
            return (compResult < 0) ? -1 : 1;
        }
#else
        ULONG Max = _LenInChars > Comparand._LenInChars ? Comparand._LenInChars : _LenInChars;
        for (ULONG ix = 0; ix < Max; ix++)
        {
            LONG Diff = LONG(_Buffer[ix] - Comparand._Buffer[ix]);
            if (Diff > 0)
            {
                return 1;
            }
            else if (Diff < 0)
            {
                return -1;
            }
        }
#endif

        if (_LenInChars > Comparand._LenInChars)
        {
            return 1;
        }
        else if (Comparand._LenInChars > _LenInChars)
        {
            return -1;
        }
        return 0;
    }

    BOOLEAN
    operator== (K$KStringView const & Other) const
    {
        return Compare(Other) == 0 ? TRUE : FALSE;
    }

    BOOLEAN
    operator!= (K$KStringView const & Other) const
    {
        return Compare(Other) == 0 ? FALSE : TRUE;
    }

    // ToUpper()  en-us only
    //
    // Converts characters in the current string to upper case.
    // Observes only the subranges U+0061..U+007A to U+0041..U+005A (English lower case to upper case)
    // This does not currently work with other languages.
    //
    VOID
    ToUpper()
    {
        if (IsNull())
        {
            return;
        }
        for (ULONG ix = 0; ix < _LenInChars; ix++)
        {
            if (_Buffer[ix] >= 0x61 && _Buffer[ix] <= 0x7A)
            {
                _Buffer[ix] -= 0x20;
            }
        }
    }

    // ToLower() en-us only
    //
    // Converts U+0041..U+005A to  U+0061..U+007A (English upper case to lower case)
    // This does not currently work with other languages.
    //
    VOID
    ToLower()
    {
        if (IsNull())
        {
            return;
        }
        for (ULONG ix = 0; ix < _LenInChars; ix++)
        {
            if (_Buffer[ix] >= 0x41 && _Buffer[ix] <= 0x5A)
            {
                _Buffer[ix] += 0x20;
            }
        }
    }

    // Upper() en-us only
    //
    // Converts the specified character to upper case, if it can be converted.
    // Otherwise, returns the original character.
    //
    // Note that this is a convenience function and doesn't reference any members.
    //
    inline static K$CHAR
    CharToUpper(
        __in K$CHAR Src
        )
    {
        if (Src >= 'a' && Src <= 'z')
        {
            return Src - 0x20;
        }
        return Src;
    }

    // Lower() en-us only
    //
    // Converts the specified character to lower case, if it can be converted.
    // Otherwise, returns the original character.
    //
    // Note that this is a convenience function and doesn't reference any members.
    //
    inline static K$CHAR
    CharToLower(
        __in K$CHAR Src
        )
    {
        if (Src >= 'A' && Src <= 'Z')
        {
            return Src + 0x20;
        }
        return Src;
    }


    // HexDigitToULONG
    //
    // Converts the hex character digit to its numeric equivalent.
    //
    // Parameters:
    //      c       The hex digit character to convert.
    //      Dest    The hex value.
    //
    // Returns TRUE on conversion, FALSE if the character is not a hex digit.
    //
    // Note that this is a convenience function and doesn't reference any members.
    //
    _Success_(return == TRUE)
    inline static BOOLEAN
    HexDigitToULONG(
        __in  K$CHAR c,
        __out ULONG& Dest
        )
    {
        if (c >= '0' && c <= '9')
        {
            Dest = c - 0x30;
        }
        else if (c >= 'a' && c <= 'f')
        {
            Dest = c - 0x57;
        }
        else if (c >= 'A' && c <= 'F')
        {
            Dest = c - 0x37;
        }
        else  // Not a number after all
        {
            return FALSE;
        }
        return TRUE;
    }


    // CompareNoCase() en-us only
    //
    // A case-insensitive comparison for UNICODE code points from U+0000 through U+007F (English only).
    //
    // Parameters:
    //      Comparand           The string to compare to the current string.
    //
    // This works with null/empty strings.  Non-empty strings are considered lexically larger than empty/null ones.
    // Two null/empty strings are considered equal.
    //
    // Returns -1 if the current string is lexically smaller than the comparand.
    // Retursn +1 if the current string is lexically larger than the comparand.
    // Returns  0 if the current string is lexically identical to the comparand.
    //
    LONG
    CompareNoCase(
        __in K$KStringView const & Comparand
        ) const;

    // CompareNoCase() en-us only
    //
    // A case-insensitive comparison for UNICODE code points from U+0000 through U+007F (English only).
    //
    // Parameters:
    //      Comparand           The string to compare to the current
    //                          string.
    //
    //      Index               The index of the first character that
    //                          is not the same.
    //
    // This works with null/empty strings.  Non-empty strings are considered lexically larger than empty/null ones.
    // Two null/empty strings are considered equal.
    //
    // Returns -1 if the current string is lexically smaller than the comparand.
    // Retursn +1 if the current string is lexically larger than the comparand.
    // Returns  0 if the current string is lexically identical to the comparand.
    //
    LONG
    CompareNoCase(
        __in K$KStringView const & Comparand,
        __out ULONG& Index
        ) const;


#if !defined(K$AnsiStringTarget)
    // CompareNoCaseAnsi
    //
    // Logically compares the current string in a case-insesntive manner to an ANSI buffer (en-us code page) for the specified
    // number of characters.
    //
    //
    // Returns -1 if the current string is lexically smaller than the comparand.
    // Retursn +1 if the current string is lexically larger than the comparand.
    // Returns  0 if the current string is lexically identical to the comparand.
    //
    LONG
    CompareNoCaseAnsi(
        __in_ecount_opt(CharsToCompare) CHAR const * TestBuffer,
        __in ULONG CharsToCompare
        ) const;

    // CompareNoCaseAnsi
    //
    // Logically compares the current string to an ANSI buffer (en-us code page) for the specified
    // number of characters.
    //
    //
    // Returns -1 if the current string is lexically smaller than the comparand.
    // Retursn +1 if the current string is lexically larger than the comparand.
    // Returns  0 if the current string is lexically identical to the comparand.
    //
    LONG
    CompareAnsi(
        __in_ecount_opt(CharsToCompare) CHAR const * TestBuffer,
        __in ULONG CharsToCompare
        ) const;
#endif


    // StripWs
    //
    // Strips leading and/or trailing whitespace from the string.
    // Whitespace in this case is any UNICODE code point betwee U+0001 and U+0020.
    // Some of the more esoteric UNICODE whitespace characters like ideaographic spaces
    // are not supported (http://en.wikipedia.org/wiki/Whitespace_character).
    //
    // Parameters:
    //      StripLeadingWs      If TRUE, leading (leftmost) whitespace is stripped.
    //      StripTrailingWs     If TRUE, trailing whitespace (rightmost) is stripped.
    //
    //
    // Note that this can't fail and returns *this as the output.
    //
    K$KStringView&
    StripWs(
        __in BOOLEAN StripLeadingWs,
        __in BOOLEAN StripTrailingWs
        )
    {
         if (IsNull())
         {
            return *this;
         }
         for (K$CHAR c = 0; StripLeadingWs && ((c = PeekFirst()) != 0); ConsumeChar())
         {
            if (!(c >= 0x1 && c <= 0x20))
            {
                break;
            }
         }
         for (K$CHAR c = 0; StripTrailingWs && ((c = PeekLast()) != 0); TruncateLast())
         {
            if (!(c >= 0x1 && c <= 0x20))
            {
                break;
            }
         }
         return *this;
    }



    // ToTimeFromuration
    //
    // Converts the current ISO 8601 duration string to its equivalent 64 bit perf duration value.
    //
    // Parameter:
    //      Duration     Receives the duration value.
    //
    // Return value:
    //      TRUE on successful conversion
    //      FALSE if the buffer is too small or the time value is invalid.
    //
    BOOLEAN
    ToTimeFromDuration(
        __out LONGLONG& Time
        ) const;


    // ToDurationFromTime
    //
    // Converts a perf time to an ISO 8601 duration string.
    //
    // Parameter:
    //      Time       The 64 bit time value.
    //
    // Return value:
    //      TRUE on successful conversion
    //      FALSE if the buffer is too small or the time value is invalid.
    //
    BOOLEAN
    ToDurationFromTime(
        __in LONGLONG Time
        );


    // ToBOOLEAN
    //
    // Convers the current string to a boolean value.
    //
    // The string value is parsed using XML rules, i.e., a string of "TRUE" "FALSE" "0" "1"
    // The string values are treated in a case-insenstive manner.
    //
    // Parameters:
    //      Dest            Receives the boolean value on successful conversion.
    //
    // Returns TRUE on a successful conversion, FALSE if the string is
    // not a boolean representation
    //
    BOOLEAN
    ToBOOLEAN(
        __out BOOLEAN& Dest
        ) const;

    // ToULONG
    //
    // Converts the current string to a ULONG.
    //
    // The current string must be either an unsigned decimal
    // or hex constant.
    //
    // Parameters:
    //      Destination     Receives the value.
    //
    // Returns:
    //
    //  TRUE on successful conversion, FALSE if the current string is not a valid numeric/hex representation or is too long.
    //
    BOOLEAN
    ToULONG(
        __out ULONG& Destination
        ) const
    {
        ULONGLONG Tmp = 0;
        if (ToULONGLONG(Tmp))
        {
            Destination = ULONG(Tmp);           // Move to 32 bit
            if (ULONGLONG(Destination) == Tmp) // Check for 32 bit overflow
            {
                return TRUE;
            }
        }
        Destination = 0;
        return FALSE;
    }

    // FromULONGLONG
    //
    // Converts the specified ULONGLONG value to a string.
    //
    // Parameters:
    //      Src         The value to be convered to a string.
    //      EncodeAsHex Whether the string should be encoded as a hex string.
    //                  If FALSE, then the conversion is decimal.
    //      DoAppend  : FALSE = Will clean this before appending conversion
    //                : TRUE  = Will just Append to this //
    // Return value:
    //      TRUE on successful conversion.
    //      FALSE if there is insufficient buffer space.
    //
    //
    BOOLEAN
    FromULONGLONG(
        __in ULONGLONG Src,
        __in BOOLEAN EncodeAsHex = FALSE,
        __in BOOLEAN DoAppend = FALSE
        );


    // ToLONGLONG
    //
    // Converts the current string to a LONGLONG.
    //
    // Parameters:
    //      Destination     Receives the numeric value.
    //
    // Return value:
    //      Returns TRUE on success, FALSE if no conversion took place,
    //      typically because of an empty string or non-numeric content.
    //
    BOOLEAN
    ToLONGLONG(
        __out LONGLONG& Destination
        ) const;


    // FromLong
    //
    // Converts the input source value to a string.
    //
    // Parameters:
    //    Src       The value to convert.
    //    DoAppend  : FALSE = Will clean this before appending conversion
    //              : TRUE  = Will just Append to this 
    //
    // Return value:
    //   FALSE if there was insufficient room for the conversion.
    //   TRUE on success
    //
    BOOLEAN
    FromLONGLONG(
        __in LONGLONG Src,
        __in BOOLEAN DoAppend = FALSE
        );

    //
    //  ToULONGLONG
    //
    //  Converts the current string to a ULONGLONG.  Assumes the current string
    //  is either a hex constant or an unsigned decimal constant.
    //
    //  Parameters:
    //      Destination     Receives the numeric value.
    //
    //  Return value:
    //      TRUE if a successful conversion took place.
    //      FALSE if the current string is empty or non-numeric or too long.
    //
    BOOLEAN
    ToULONGLONG(
        __out ULONGLONG& Destination
        ) const;


    // FromULONG
    //
    // Converts the specified ULONG value to a string.
    //
    // Parameters:
    //      Src         The value to be convered to a string.
    //      EncodeAsHex Whether the string should be encoded as a hex string.
    //                  If FALSE, then the conversion is decimal.
    //      DoAppend  : FALSE = Will clean this before appending conversion
    //                : TRUE  = Will just Append to this 
    //
    // Return value:
    //      TRUE on successful conversion.
    //      FALSE if there is insufficient buffer space.
    //
    //
    BOOLEAN
    FromULONG(
        __in ULONG Src,
        __in BOOLEAN EncodeAsHex = FALSE,
        __in BOOLEAN DoAppend = FALSE
        )
    {
        return FromULONGLONG(Src, EncodeAsHex, DoAppend);
    }


    // ToLONG
    //
    // Converts the current string to a LONG, if it is within 32 bit range
    // and a legal signed value.
    //
    // Parameters:
    //      Destination     Receives the numeric value.
    //
    // Return value:
    //      TRUE on successful conversion
    //      FALSE if the current string is empty, too long, or not numeric.
    //
    //
    BOOLEAN
    ToLONG(
        __out LONG& Destination
        ) const
    {
        LONGLONG Tmp = 0;
        if (ToLONGLONG(Tmp))
        {
            Destination = LONG(Tmp);           // Move to 32 bit
            if (LONGLONG(Destination) == Tmp) // Check for 32 bit overflow
            {
                return TRUE;
            }
        }
        Destination = 0;
        return FALSE;
    }


    // Dequote
    //
    // Returns a subset string view of the current string without beginning/ending quotes, whether single or double.
    // Returns the current string if it is not quoted.
    //
    // This does NOT process quotes occurring within the middle of the string.
    //
    //
    K$KStringView
    Dequote() const
    {
        K$KStringView Tmp(*this);
        if (Tmp.PeekFirst() == '"' || Tmp.PeekFirst() == '\'')
        {
            Tmp.ConsumeChar();
        }
        if (Tmp.PeekLast() == '"' || Tmp.PeekLast() == '\'')
        {
            Tmp.TruncateLast();
        }
        return Tmp;
    }

    // UnencodeEscapeChars
    //
    // Resolves URL escaped characters back to their normal equivalents.  For example, %20 is
    // replaced by a space.
    //
    // Since this always results in a reduction in the string length it cannot fail. For
    // empty strings, this simply returns after performing no action.
    //
    //
    VOID
    UnencodeEscapeChars();

    // EncodeEscapeChars
    //
    // Converts non-printable characters to their escaped equivalents (URL encoding rules).
    // For example, a literal space is expanded to %20.  Since this will result in a string
    // becoming longer, it is possible that the underlying buffer does not have enough freespace
    // to continue the operation.
    //
    // This returns FALSE only if there is insufficient space. In that case, the string
    // is left untouched by converting it back to its unescaped equivalent.
    //
    // This returns TRUE on empty strings or strings in which no encoding changes occurred, since
    // those strings may be considered properly encoded in the logical sense.
    //
    // NOT YET IMPLEMENTED
    //
    BOOLEAN
    EncodeEscapeChars();


    // EncodeBase64
    //
    // Encodes the specified buffer to *this using base64 character encoding.
    //
    // NOT YET IMPLEMENTED
    //
    NTSTATUS
    ToBase64(
        __in PVOID Buffer,
        __in ULONG ByteCount
        );

    // FromBase64
    //
    // Converts the current string from Base64 back to a binary buffer.
    //
    // NOT YET IMPLEMENTED

    NTSTATUS
    FromBase64(
        __out_bcount(BufferSize)  PVOID  Buffer,
        __in  ULONG  BufferSize,
        __out ULONG& Used
        );


    // FromSystemTimeToIso8601
    //
    // Converts a given system time value to an ISO 8601 string representation.
    // This can only fail if there insufficient space to receive the corresponding
    // date/time.
    //
    // Parameters:
    //      Time      : System time
    //      DoAppend  : FALSE = Will clean this before appending conversion
    //                : TRUE  = Will just Append to this 
    //
    // Returns TRUE on successful conversion.  Returns FALSE if the current string
    // is not large enough to receive the date, or the date value is not valid.
    //
    // The format is currently fixed at 20 characters and must be UTC:
    //
    //      yyyy-mm-ddThh:mm:ssZ
    //
    BOOLEAN
    FromSystemTimeToIso8601(
        __in LONGLONG Time,
        __in BOOLEAN DoAppend = FALSE
        );

    // FromULONG
    //
    // Produces the string representation of the specified
    // signed value.
    //
    // Parameters:
    //      Src         The value to convert.
    //      DoAppend  : FALSE = Will clean this before appending conversion
    //                : TRUE  = Will just Append to this 
    //
    // Returns TRUE on success, FALSE if insufficient buffer to receive the string.
    //
    //
    BOOLEAN
    FromLONG(
        __in LONG Src,
        __in BOOLEAN DoAppend = FALSE
        )
    {
        return FromLONGLONG(Src, DoAppend);
    }


    //  ToGuid()
    //
    //  Converts the current string to a GUID, assumes that
    //  the GUID is in normal form with the curly braces.
    //
    //  This is a robust parser, so feeding it non-GUIDs is safe.
    //
    //  Parameters:
    //      Dest            Receives the GUID.
    //
    //  Return value:
    //     TRUE if conversion was successful.
    //     FALSE if the current string is NULL or otherwise not a GUID.
    //
    BOOLEAN
    ToGUID(
        __out GUID& Dest
        ) const;


    //  FromGUID
    //
    //  Converts the input Src GUID to a string representation.  Returns TRUE
    //  on success, FALSE if there was insufficient buffer space to receive
    //  the GUID.   Curly braces are added, but not quotes.
    //
    //  Parameters:
    //      Src         The GUID to convert to a string.
    //      DoAppend  : FALSE = Will clean this before appending conversion
    //                : TRUE  = Will just Append to this 
    //
    //  Return value:
    //      TRUE on succesful conversion
    //      FALSE only if there is insufficient string space to receive the GUID representation.
    //
    BOOLEAN
    FromGUID(
        __in const GUID& Src,
        __in BOOLEAN DoAppend = FALSE
        );

    // BufferSize
    //
    // Returns the size of the buffer.  This can be used along with FreeSpace()
    // to determine if the buffer is large enough for an operation which will
    // increase the string length
    //
    ULONG
    BufferSizeInChars() const
    {
        return _BufLenInChars;
    }

    // MeasureThis()
    //
    // Scans the string for a NULL and assigns the length.
    // This does not reference or alter the memory.
    // Allows a null buffer.
    //
    // NOTE: THIS IS INTENDED FOR USE WHEN IT IS CERTAIN
    // THERE IS A NULL TERMINATOR OR THE BUFFER LENGTH
    // (_BufLenInChars) IS KNOWN TO BE SAFE BOUNDED OR
    // IT WILL EVENTUALLY FAULT.
    //
    // Return value:
    //      TRUE if a null was detected
    //      FALSE if this string is empty
    //
    BOOLEAN
    MeasureThis()
    {
        if (IsNull())
        {
            return FALSE;
        }

        _LenInChars = 0;
        K$CHAR*  Scanner = _Buffer;
        while (*Scanner++)
        {
            if (_BufLenInChars == _LenInChars)
            {
                return FALSE;
            }

            _LenInChars++;
        }

        return TRUE;
    }

    // SetLength()
    //
    // Explicitly sets the string length to the specified value.
    //
    // This is UNCHECKED and intended for supervised use by friend classes &
    // parsers which explicitly manipulate StringView.
    //
    // Parameters:
    //      NewLength
    //
    //
    VOID
    SetLength(
        __in ULONG NewLength
        )
    {
        KInvariant(NewLength <= MaxLength);
        _LenInChars = NewLength;
    }

    //** NOTE: The following methods are only safe when used on an actual KStringView instance as
    //         they alter _Buffer. Those wishing to use these behaviors on a derived type should do
    //         so by ctoring an actual KStringView instance over such a derived type instance.
    //
    // ResetOrigin
    //
    // Adjusts the origin of the string to a new offset.
    //
    //
    BOOLEAN
    ResetOrigin(
        __in ULONG Offset
        )
    {
        KInvariant(!_UseShortBuffer);
        if (IsNull() || Offset >= _BufLenInChars)
        {
            return FALSE;
        }

        _Buffer += Offset;
        _LenInChars -= Offset;
        _BufLenInChars -= Offset;

        return TRUE;
    }

    // Zero
    //
    // Does what it says.
    //
    VOID
    Zero()
    {
        _Buffer = nullptr;
        _BufLenInChars = _LenInChars = 0;
        _UseShortBuffer = FALSE;
    }

    // SetAddress
    //
    // Sets the buffer address with no other values. Used as a parser helper.
    //
    VOID
    SetAddress(
        __in K$PCHAR NewAddr
        )
    {
        Zero();
        _Buffer = NewAddr;
    }

private:
    union
    {
        K$PCHAR _IntBuffer;
        K$CHAR  _ShortBuffer[sizeof(void*) / K$CHARSIZE];
    };

    // High order bit == IsShortBufferInUse; LSB 31 bits == _BufLenInChars
    ULONG   _IntBufLenInChars;

protected:
    // K$PCHAR  _Buffer;
    __declspec(property(get = Get_Buffer, put = Put_Buffer)) K$PCHAR _Buffer;
    __inline K$PCHAR Get_Buffer(void) const
    {
        if (_UseShortBuffer)
        {
            KAssert(_BufLenInChars <= ARRAYSIZE(_ShortBuffer));

            if (_BufLenInChars > 0)
            {
                return K$PCHAR(&_ShortBuffer[0]);
            }
            return nullptr;
        }
        return _IntBuffer;
    }

    __inline ULONGLONG GetRaw_Buffer()          {return (ULONGLONG)_IntBuffer; }
    __inline void Put_Buffer(__in K$PCHAR NewValue)     {_IntBuffer = NewValue;}
    __inline void PutRaw_Buffer(__in ULONGLONG NewValue)    {_IntBuffer = (K$PCHAR)NewValue; }

    __declspec(property(get = Get_BufLenInChars, put = Put_BufLenInChars)) ULONG _BufLenInChars;
    __inline ULONG Get_BufLenInChars(void) const
    {
        return _IntBufLenInChars & MAXLONG;
    }
    __inline ULONG GetRaw_BufLenInChars(void) const
    {
        return _IntBufLenInChars;
    }

    __inline void Put_BufLenInChars(__in ULONG NewValue)
    {
        KAssert(NewValue <= MAXLONG);
        _IntBufLenInChars = (_IntBufLenInChars & ~MAXLONG) | NewValue;
    }
    __inline void PutRaw_BufLenInChars(__in ULONG NewValue)
    {
        _IntBufLenInChars = NewValue;
    }

    // BOOLEAN  _UseShortBuffer;
    __declspec(property(get = Get_UseShortBuffer, put = Put_UseShortBuffer)) BOOLEAN _UseShortBuffer;
    __inline BOOLEAN Get_UseShortBuffer(void) const
    {
        return (_IntBufLenInChars & ~MAXLONG) == 0 ? FALSE : TRUE;
    }
    __inline void Put_UseShortBuffer(__in BOOLEAN NewValue)
    {
        _IntBufLenInChars = _BufLenInChars | (NewValue ? ~MAXLONG : 0);
    }

    __inline ULONG MaxShortBufferSize() { return ARRAYSIZE(_ShortBuffer); }

    ULONG   _LenInChars;
};


//
//  KLocalString
//  Templated version which takes a fixed size and can live
//  as a member in another object, template, or on the stack.
//
//  Typical use:
//      KLocalString<128> Str;      // Similar to WCHAR Str[128];
//      KLocalStringA<128> Str;     // Similar to CHAR Str[128];
//
template <ULONG BufSizeChars>
class K$KLocalString : public K$KStringView
{
    K_NO_DYNAMIC_ALLOCATE();

public:
    K$KLocalString()
        :   K$KStringView()
    {
        KAssert(BufSizeChars > 0);

        _Buffer = _LocalBuffer;
        _LenInChars = 0;
        _BufLenInChars = BufSizeChars;
        _LocalBuffer[0] = 0;
    }

    VOID
    Clear()
    {
        _Buffer = _LocalBuffer;
        _LenInChars = 0;
        _BufLenInChars = BufSizeChars;
    }

    VOID
    Zero()
    {
        Clear();
    }

private:
    // The functions below alter the address of _Buffer.
    // They are inappropriate for this class.  If the functionality
    // is needed, just make a new KStringView of this object
    // and use the below functions on that.

    BOOLEAN
    ConsumeChars(
        __in ULONG Total
        );

    BOOLEAN
    ConsumeChar();

#if !defined(K$AnsiStringTarget)
    BOOLEAN
    SkipBOM();
#endif

    VOID
    SetAddresss(
        __in K$PCHAR NewAddr
        );

    BOOLEAN
    ResetOrigin(
        __in ULONG Offset
        );



private:
    K$CHAR _LocalBuffer[BufSizeChars];
};


//
//  KDynString
//
//  This derivation of KStringView owns its buffer, which is fixed length
//  at construct-time.   For read-only functions, this class just uses
//  the base class. For mutating calls, there is a possibility of the buffer
//  not being large enough.  In these cases, the base class function
//  will fail.  On an as-needed basis, overrides occur for those functions
//  are implemented in this class, which resize the buffer so that the base implementation
//  will succeed.  Eventually all mutating calls should have an override here.
//
//
//  This may be constructed empty and resized later upon first use.
//
//
class K$KDynString : public K$KStringView
{
    K_DENY_COPY(K$KDynString);

    KAllocator_Required();

public:
    K$KDynString(
        __in KAllocator& Alloc
        )
        : _Allocator(Alloc)
        {
            _Buffer = nullptr;
            _UseShortBuffer = TRUE;
        }

    K$KDynString(
        __in KAllocator& Alloc,
        __in ULONG WCharCount
        )
        : _Allocator(Alloc)
    {
        KAssert(WCharCount > 0);

        if (WCharCount <= MaxShortBufferSize())
        {
            // Make use of the internal short buffer optimization
            _UseShortBuffer = TRUE;
            _BufLenInChars = WCharCount;
            _Buffer[0] = 0;
            return;
        }

        _Buffer = _new(KTL_TAG_BUFFERED_STRINGVIEW, _Allocator) K$CHAR[WCharCount];
        _LenInChars = 0;

        if (!_Buffer)
        {
            _BufLenInChars = 0;
        }
        else
        {
            _BufLenInChars = WCharCount;
            _Buffer[0] = 0;
        }
    }

    operator BOOLEAN()
    {
        return _Buffer ? TRUE : FALSE;
    }

   ~K$KDynString()
    {
        if (!_UseShortBuffer)
        {
            _delete(_Buffer);
        }
       _Buffer = nullptr;
       _UseShortBuffer = FALSE;
    }


   //   Expand
   //
   //   Expands the buffer by the specifed number of characters.  Retains the content of
   //   the StringView, if any.
   //
   //   This will not resize a string to less than its current size.
   //   Returns TRUE on success, FALSE on failure, which can only occur on failure to allocate.
   //
   BOOLEAN
   Resize(__in ULONG NewSize)
   {
        KInvariant(NewSize <= MaxLength);
        if (NewSize <= _BufLenInChars)
        {
            return TRUE;
        }

        if ((NewSize <= MaxShortBufferSize()) && _UseShortBuffer)
        {
            // Still using short internal buffer; just bump the size
            _BufLenInChars = NewSize;
            return TRUE;
        }

        K$PCHAR Tmp = _new(KTL_TAG_BUFFERED_STRINGVIEW, _Allocator) K$CHAR[NewSize];
        if (!Tmp)
        {
            return FALSE;
        }

        K$KStringView TmpView(Tmp, NewSize, 0);

        // If there is exiting content, retain it.
        //
        if (_Buffer)
        {
            TmpView.CopyFrom(*this);  // Can't fail.
            if (!_UseShortBuffer)
            {
                _delete(_Buffer);         // Kill old buffer
            }
        }

        // Transfer ownrship
        //
        _UseShortBuffer = FALSE;
        _Buffer = K$PCHAR(TmpView);
        _LenInChars = TmpView.Length();
        _BufLenInChars = TmpView.BufferSizeInChars();

        return TRUE;
   }

   // CopyFrom
   //
   // Allocates as necessary to make room.
   //
   BOOLEAN
   CopyFrom(
       __in K$KStringView const & Src,
       __in BOOLEAN AppendNull = TRUE
       )
   {
       if (Src.IsNull())
       {
           return FALSE;
       }

       ULONG RequiredLength = Src.Length();
       if (AppendNull)
       {
           HRESULT hr;
           hr = ULongAdd(RequiredLength, 1, &RequiredLength);
           KInvariant(SUCCEEDED(hr));
       }

       if (_BufLenInChars < RequiredLength)
       {
            if (!Resize(RequiredLength))
            {
                return FALSE;
            }
       }

       if (! K$KStringView::CopyFrom(Src))
       {
           return FALSE;
       }
       if (AppendNull)
       {
           SetNullTerminator();
       }
       return TRUE;
   }

    // Concat
    //
    // Allocates as necessary to expand the string to accommodate the new total length.
    // FALSE is returned only on failure to allocate.  A Concat of a null/empty KStringView
    // is allowed, although it is effectively a no-op.
    //
    BOOLEAN
    Concat(
        __in K$KStringView const & Src,
        __in BOOLEAN AppendNull = TRUE
        )
    {
        if (Src.IsEmpty())
        {
            return TRUE;
        }

        HRESULT hr;
        ULONG RequiredLength;

        hr = ULongAdd(Src.Length(), _LenInChars, &RequiredLength);
        KInvariant(SUCCEEDED(hr));

        if (AppendNull)
        {
           hr = ULongAdd(RequiredLength, 1, &RequiredLength);
           KInvariant(SUCCEEDED(hr));
        }
        if (_BufLenInChars < RequiredLength)
        {
            if (!Resize(RequiredLength))
            {
                return FALSE;
            }
        }
        if (! K$KStringView::Concat(Src))
        {
            return FALSE;
        }
        if (AppendNull)
        {
            SetNullTerminator();
        }
        return TRUE;
    }


    // Prepend
    //
    BOOLEAN
    Prepend(
        __in K$KStringView const & Src,
        __in BOOLEAN AppendNull = TRUE
        )
    {
        if (Src.IsEmpty())
        {
            return TRUE;
        }

        HRESULT hr;
        ULONG RequiredLength;

        hr = ULongAdd(Src.Length(), _LenInChars, &RequiredLength);
        KInvariant(SUCCEEDED(hr));

        if (AppendNull)
        {
           hr = ULongAdd(RequiredLength, 1, &RequiredLength);
           KInvariant(SUCCEEDED(hr));
        }
        if (_BufLenInChars < RequiredLength)
        {
            if (!Resize(RequiredLength))
            {
                return FALSE;
            }
        }
        if (!K$KStringView::Prepend(Src))
        {
            return FALSE;
        }
        if (AppendNull)
        {
            SetNullTerminator();
        }
        return TRUE;
    }


    BOOLEAN
    SetNullTerminator()
    {
        if (K$KStringView::SetNullTerminator() == TRUE)
        {
            return TRUE;
        }

		HRESULT hr;
		ULONG result;
		hr = ULongAdd(_LenInChars, 1, &result);
		KInvariant(SUCCEEDED(hr));

        if (!Resize(result))
        {
            return FALSE;
        }
        return SetNullTerminator();
    }

    BOOLEAN
    AppendChar(
        __in K$CHAR Char
        )
    {
        if (K$KStringView::AppendChar(Char))
        {
            return TRUE;
        }

		HRESULT hr;
		ULONG result;
		hr = ULongAdd(_LenInChars, 1, &result);
		KInvariant(SUCCEEDED(hr));

        if (!Resize(result))
        {
            return FALSE;
        }
        return K$KStringView::AppendChar(Char);
    }

    BOOLEAN
    PrependChar(
        __in K$CHAR Char
        )
    {
        if (K$KStringView::PrependChar(Char))
        {
            return TRUE;
        }

		HRESULT hr;
		ULONG result;
		hr = ULongAdd(_LenInChars, 1, &result);
		KInvariant(SUCCEEDED(hr));

        if (!Resize(result))
        {
            return FALSE;
        }
        return K$KStringView::PrependChar(Char);
    }

    VOID
    Zero()
    {
        _LenInChars = 0;
        if (_Buffer)
        {
            _Buffer[0] = 0;
        }
    }

    BOOLEAN
    Replace(
        __in  const K$KStringView& Target,
        __in  const K$KStringView& Replacement,
        __out ULONG& ReplacementCount,
        __in  BOOLEAN ReplaceAll = TRUE
        )
    {
        ReplacementCount = 0;
        BOOLEAN Result = FALSE;
        ULONG TmpReplacementCount;
        while (!Result)
        {
            TmpReplacementCount = 0;
            Result = K$KStringView::Replace(Target, Replacement, TmpReplacementCount, ReplaceAll);
            if (! Result)
            {
				HRESULT hr;
				ULONG result;
				hr = ULongMult(_BufLenInChars, 2, &result);
				KInvariant(SUCCEEDED(hr));

				if (!Resize(result))
                {
                    return FALSE;
                }
            }
            ReplacementCount += TmpReplacementCount;
        }
        return TRUE;
    }

    // Overload for situations where you don't care about the replacement count.
    //
    BOOLEAN
    Replace(
        __in  K$KStringView& Target,
        __in  K$KStringView& Replacement
        )
    {
        ULONG DummyCount = 0;
        return Replace(Target, Replacement, DummyCount, TRUE);
    }



protected:
    KAllocator& _Allocator;

private:

    // The functions below alter the address of _Buffer.
    // They are inappropriate for this class.  If the functionality
    // is needed, just make a new KStringView of this object
    // and use the below functions on that.

    BOOLEAN
    ConsumeChars(
        __in ULONG Total
        );

    BOOLEAN
    ConsumeChar();

#if !defined(K$AnsiStringTarget)
    BOOLEAN
    SkipBOM();
#endif

    VOID
    SetAddresss(
        __in K$PCHAR NewAddr
        );

    BOOLEAN
    ResetOrigin(
        __in ULONG Offset
        );

};



//
//  class KString
//
//  Shared string pointer version of KStringView
//
class K$KString : public K$KDynString, public KObject<K$KString>, public KShared<K$KString>
{
    K_FORCE_SHARED(K$KString);

public:

    // Create
    //
    // A series of overloads to create shared strings.
    //
    // The KString may be created empty and resized later on first use.
    //

    //
    //  Create a KString with a given buffer size
    //
    static NTSTATUS
    Create(
        __out K$KString::SPtr& String,
        __in KAllocator& Alloc,
        __in ULONG BufferSizeInChars = 0
        );

    //
    //  Create a KString with contents equal to the supplied KStringView
    //
    static NTSTATUS
    Create(
        __out K$KString::SPtr& String,
        __in KAllocator& Alloc,
        __in K$KStringView const & ToCopy,
        __in BOOLEAN IncludeNullTerminator = TRUE
        );

    //
    //  Create a KString with contents equal to the supplied constant string
    //
    static NTSTATUS
    Create(
        __out K$KString::SPtr& String,
        __in KAllocator& Alloc,
        __in K$LPCSTR ToCopy,
        __in BOOLEAN IncludeNullTerminator = TRUE
        );

    //
    //  DEPRECATED - use K$KString::Create(String, Alloc)
    //
    static K$KString::SPtr
    Create(
        __in KAllocator& Alloc
        );

    //
    //  DEPRECATED - use K$KString::Create(String, Alloc, BufferSizeInChars)
    //
    static K$KString::SPtr
    Create(
        __in KAllocator& Alloc,
        __in ULONG BufferSizeInChars
        );

    //
    //  DEPRECATED - use K$KString::Create(String, Alloc, ToCopy, IncludeNullTerminator)
    //
    static K$KString::SPtr
    Create(
        __in K$LPCSTR Str,
        __in KAllocator& Alloc,
        __in BOOLEAN IncludeNullTerminator = TRUE
        );

    //
    //  DEPRECATED - use K$KString::Create(String, Alloc, ToCopy, IncludeNullTerminator)
    //
    static K$KString::SPtr
    Create(
        __in const K$KStringView& Src,
        __in KAllocator& Alloc,
        __in BOOLEAN IncludeNullTerminator = TRUE
        );

#if !defined(K$AnsiStringTarget)

    //
    //  Create A KString with contents equal to the supplied UNICODE_STRING
    //
    static NTSTATUS
    Create(
        __out KString::SPtr& String,
        __in KAllocator& Alloc,
        __in UNICODE_STRING const & ToCopy,
        __in BOOLEAN IncludeNullTerminator = TRUE
        );

    //
    //  DEPRECATED - use KString::Create(String, Alloc, ToCopy, IncludeNullTerminator)
    //
    static K$KString::SPtr
    Create(
        __in UNICODE_STRING& Src,
        __in KAllocator& Alloc,
        __in BOOLEAN IncludeNullTerminator = TRUE
        );
#endif

    //  Clone
    //
    //  Returns a copy of the current string. Returns a null SPtr
    //  if there is insufficient memory.  That is the only failure case.
    //
    //  DEPRECATED - use K$KString::Create(String, Alloc, ToCopy, IncludeNullTerminator)
    //
    K$KString::SPtr
    Clone(
        __in BOOLEAN IncludeNullTerminator = TRUE
        ) const;

private:
};


//
//  KBufferString
//
//  Version of KStringView backed by a region of a KBuffer, which is held via a SPtr.
//  Created without a backing region and later mapped to one, or via a copy constructor.
//  Shares ownership of the backing KBuffer, and will keep it alive while mapped.
//
//  Care should be taken when modifying the backing KBuffer KBufferString, and when
//  sharing a KBuffer amongst multiple strings. KBufferString instances are not
//  informed about changes to the KBuffer they are backed by, and offer no protection
//  from other modifications to it.  The buffer size, for example, is checked only
//  at mapping time; if it is reduced without remapping the KBufferString, access
//  to invalid memory locations may occur.
//
class K$KBufferString : public K$KStringView, public KObject<K$KBufferString>
{

public:

    //
    //  Map this KBufferString to a buffer region.  May fail if an invalid offset and
    //  string length combination is specified for the given buffer.  Upon failure
    //  the KBufferString will be unmodified.
    //
    //  Parameters:
    //      Buffer - The KBuffer which contains the string to be mapped.
    //
    //      StringLengthInChars - Length (in characters, not bytes) of the string
    //          to map.  If too large for the given buffer and byte offset, this function
    //          will fail.
    //
    //      ByteOffset - The offset into Buffer where the string begins (inclusive).  If
    //          too large for the given buffer (>= the buffer size) this function will fail.
    //
    //      MaxStringLengthInChars - Allow the string to grow to this size at most.  The actual
    //          maximum will be the minimum between this value and the size of the region bounded
    //          by [ByteOffset, Buffer->QuerySize()).  If too small for the given string length,
    //          this function will fail.
    //
    NTSTATUS
    MapBackingBuffer(
        __in KBuffer& Buffer,
        __in ULONG StringLengthInChars,
        __in ULONG ByteOffset = 0,
        __in ULONG MaxStringLengthInChars = ULONG_MAX
        );

    //
    //  Remove the mapping between this string and the backing KBuffer
    //
    VOID
    UnMapBackingBuffer();

    //
    //  Get the KBuffer this string is backed by
    //
    KBuffer::SPtr
    GetBackingBuffer() const;

    NOFAIL K$KBufferString();
    NOFAIL ~K$KBufferString();

    //
    //  Copy constructor (allocates a KBuffer internally)
    //
    FAILABLE
    K$KBufferString(
        __in KAllocator& Allocator,
        __in K$KStringView const & ToCopy
        );

private:

    KBuffer::SPtr _KBuffer;
};


//
//  Shared version of KBufferString
//
class K$KSharedBufferString : public K$KBufferString, public KShared<K$KSharedBufferString>
{
    K_FORCE_SHARED(K$KSharedBufferString);

public:

    static NTSTATUS
    Create(
        __out K$KSharedBufferString::SPtr& SharedString,
        __in KAllocator& Alloc,
        __in ULONG AllocationTag = KTL_TAG_KSHARED
        );

    //
    //  Uses the KBufferString copy constructor to allocate a KBuffer internally and
    //  copy the given KStringView
    //
    static NTSTATUS
    Create(
        __out K$KSharedBufferString::SPtr& SharedString,
        __in K$KStringView const & ToCopy,
        __in KAllocator& Alloc,
        __in ULONG AllocationTag = KTL_TAG_KSHARED
        );

private:

    //
    //  Copy constructor (allocates a KBuffer internally)
    //
    FAILABLE
    K$KSharedBufferString(
        __in KAllocator& Allocator,
        __in K$KStringView const & ToCopy
        );
};


//
//  KStringPool
//
//  A generic suballocator for KStringView objects.
//  Typical use is as a scratch area in loops.  It is allocated once
//  and KStringViews are allocated from it. Calling Reset()
//  'frees' all previous KStringView mappings.
//
class K$KStringPool
{
    K_DENY_COPY(K$KStringPool);
    K_NO_DYNAMIC_ALLOCATE();

public:
    // Constructor
    //
    // Builds a string pool from the heap in the specified size.
    //
    // After construction, either FreeSpace() can be checked to verify
    // that there is sufficient work space, or each call to operator() can
    // be checked (which must occur anyway)
    //
    K$KStringPool(
        __in KAllocator& Alloc,
        __in ULONG Size
        )
    {
        _Buffer = _new(KTL_TAG_BUFFERED_STRINGVIEW, Alloc) K$CHAR[Size];
        _Size = Size;
        _Remaining = Size;
        if (!_Buffer)
        {
            _Size = 0;
            _Remaining = 0;
        }
    }

    // FreeSpace
    //
    // Returns the number of characters remaining in the pool.
    //
    ULONG
    FreeSpace() const
    {
        return _Remaining;
    }

    // Used
    //
    // Returns the number of characters used from the pool.
    //
    ULONG
    Used() const
    {
        return _Remaining - _Size;
    }

   ~K$KStringPool()
    {
        _delete(_Buffer);
    }

    // operator()
    //
    // Used to allocate a new KStringView from the pool.
    //
    // Note that the returned value is mapped onto a reserved chunk of memory, but not
    // allocated using normal heap calls.
    //
    // Parameters:
    //      Len         The number of characters for the buffer size of the new string.
    //                  Add 1 if a null terminator will be set on the string at some point.
    //
    // Return value:
    //      The mapped KStringView. This will be a null KStringView if there was insufficient
    //      pool space to allocate a KStringView of the requested size. This can be checked
    //      with IsNull()
    //
    //      KStringPool Pool(Alloc, 1024);
    //      KStringView NewStr = Pool(128);  // Get space for a 128 byte string
    //      if (NewStr.IsNull() { ... failure }
    //
    K$KStringView
    operator()(
        __in ULONG Len
        )
    {
        if (!_Buffer || _Remaining < Len)
        {
            return FALSE;
        }
        K$KStringView Dest;
        Dest._Buffer = &_Buffer[_Size - _Remaining];
        Dest._BufLenInChars = Len;
        Dest._LenInChars = 0;
        _Remaining -= Len;
        return Dest;
    }

    //  Reset
    //
    //  Invalidates the current extent and resets the entire pool. All outstanding KStringViews
    //  should be considered invalid.
    //
    //
    VOID
    Reset()
    {
        _Remaining = _Size;
    }
private:

    K$CHAR* _Buffer;
    ULONG   _Remaining;
    ULONG   _Size;
};


//
//  Simple tokenizer, which yields the substrings separated by a given delimiter.
//  Future work:  Make quote-aware, multiple delims
//
class K$KStringViewTokenizer
{

public:

    K$KStringViewTokenizer(
        ) :
            _DoneParsing(TRUE)
    {
    }

    //
    //  Initialize this tokenizer with the supplied string and delimiter.  Must be called before
    //  starting tokenization (with calls to NextToken)
    //
    VOID
    Initialize(
        __in K$KStringView const & String,
        __in K$CHAR Delimiter = K$STRING(',')
        )
    {
        _String = String;
        _Delimiter = Delimiter;

        _DoneParsing = FALSE;
        _StartIndex = 0;
        _EndIndex = 0;
    }

    //
    //  Retrieve the next token from the string initialied with a call to Initialize.  Returns
    //  TRUE if the out param (Token) is set to a valid token upon return.  If FALSE is returned,
    //  Token will represent the empty string and should not be considered a valid token.
    //
    _Success_(return == TRUE)
    BOOLEAN
    NextToken(
        __out K$KStringView& Token
        )
    {
        K$KStringView empty;
        Token = empty;

        if (_DoneParsing)
        {
            return FALSE;
        }

        while (_EndIndex < _String.Length() && _String[_EndIndex] != _Delimiter)
        {
            _EndIndex++;
        }

        if (_EndIndex == _String.Length())
        {
            _DoneParsing = TRUE;
        }

        Token = _String.SubString(_StartIndex, _EndIndex - _StartIndex);

        _EndIndex++;  //  Advance past the delimiter
        _StartIndex = _EndIndex;

        return TRUE;
    }

private:

    BOOLEAN _DoneParsing;
    ULONG _StartIndex;
    ULONG _EndIndex;
    K$KStringView _String;
    K$CHAR _Delimiter;
};


// Remove type parameters
#undef K$KStringView
#undef K$KStringViewTokenizer
#undef K$KStringPool
#undef K$KLocalString
#undef K$KDynString
#undef K$KString
#undef K$KBufferString
#undef K$KSharedBufferString
#undef K$CHAR
#undef K$LPCSTR
#undef K$PCHAR
#undef K$CHARSIZE
#undef K$STRING
#undef K$StringCompare