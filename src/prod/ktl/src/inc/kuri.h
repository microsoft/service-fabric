


/*++

    (c) 2012 by Microsoft Corp. All Rights Reserved.

    kuri.h

    Description:
      Kernel Tempate Library (KTL): KUri

      URI wrapper classes with built-in parser

    History:
      raymcc          7-Mar-2012         Initial version (no parser).
      raymcc          8-May-2012         Revised with full parser.


    The parser is a loose superset of RFC 3986.  The superset is to allow
    for custom macro expansion, Microsoft UNC values, XML URI Reference,
    and other slightly non-URI constructs.  The parser currently does not accept
    some types of legal URIs, such as those empty internal segments
    in the path portion.  The parser does not expand or convert escape characters,
    such as %20 vs. a space.  The string should be preprocessed/postprocessed
    if that is a concern.

--*/

#pragma once

// class KUriView
//
// This is a non-allocating read-only view of a URI. Its primary purpose is to act as a wrapper for constant LPWSTR
// Uri values as parameters, i.e.,
//
//      SomeFunctionCall(KUriView(L"foo://bar/x/y/z"));
//
// Initial parsing occurs during construction.  The parser is a loose superset of RFC 3986.  The superset is to allow
// for custom macro expansion, Microsoft UNC values, and other slightly non-URI constructs.
//
// If access to individual query string parameters or segments of the URI are required, you must use one of the
// derived classes so that the URI does not have to be reparsed each time these values are required.  Since this
// class does not allocate, asking for components which are variable in cardinality (segment count, etc.) would
// require a full reparse of the URI each time a field was requested.
//

class KUriView
{
public:
    // Bit fields for identifying various fields of the URI.
    //
    enum UriFieldType : ULONG
    {
            eScheme = 0x1,              // The scheme portion

            eHost = 0x2,                // The host name in the authority without the port
            ePort = 0x4,
            eAuthority = 0x8,           // The host port value together as they originally appear in the raw URI

            ePath = 0x10,               // The entire path
            eQueryString = 0x20,        // The entire query string
            eFragment = 0x40,           // The fragment

            eLiteralHost = 0x80,        // Host name without wildcard testing

            eRaw = 0x80000000           // The entire original raw URI string.
    };

    KUriView()
    {
        _PortVal = 0;
        _ErrorPos = 0;
        _IsValid = FALSE;
        _StandardQueryString = FALSE;
    }

    KUriView(
        __in const KUriView& Src
        )
    {
        *this = Src;
    }

    KUriView& operator=(
        __in const KUriView& Src
        )
    {
        _Scheme = Src._Scheme;
        _Authority = Src._Authority;
        _Host = Src._Host;
        _PortVal = Src._PortVal;
        _ErrorPos = Src._ErrorPos;
        _Path = Src._Path;
        _QueryString = Src._QueryString;
        _Fragment = Src._Fragment;
        _Raw = Src._Raw;
        _IsValid = Src._IsValid;
        _StandardQueryString = Src._StandardQueryString;
        return *this;
    }

    VOID
    Parse(
        __in KStringView Src
        )
    {
        Initialize(Src);
    }

    // IsValid
    //
    // Returns TRUE if the URI is valid.  This does not guarantee the presence of any specific field, since
    // the URI spec is quite liberal.  This will typically be combined with Has() in order to ensure
    // that the URI meets requirements.  For example,
    //
    //      if (MyUri.IsValid() && MyUri.Has(eScheme)) ...  // ensure a scheme is present
    //
    BOOLEAN
    IsValid() const
    {
        return _IsValid;
    }

    // IsEmpty
    //
    // Returns TRUE if the URI is empty.
    //
    BOOLEAN
    IsEmpty() const
    {
        return _Raw.IsEmpty();
    }


    // HostIsWildcard
    //
    // Returns TRUE if the authority is a 'wildcard', i.e., * or +
    // This type of authority logically matches any other authority when comparing URIs.
    //
    BOOLEAN
    HostIsWildcard() const;

    // Has
    //
    // Returns TRUE if one or more the specified fields is non-empt after parsing.  This is used
    // to test for the present of specific fields, such as a scheme, a port, or a fragment, etc.
    //
    // Parameters:
    //      Fields   A bit mask of the UriFieldType fields to be tested.
    //
    // Return value:
    //  TRUE if all of the fields in the mask are present
    //  FALSE if any one of the specified fields is missing.
    //
    BOOLEAN
    Has(
        __in ULONG Fields
        ) const;

    // IsRelative
    //
    // Retursn TRUE if the URI is a path-only/'relative' URI
    // FALSE if not.
    //
    BOOLEAN
    IsRelative() const
    {
        if (_Scheme.IsEmpty() && _Authority.IsEmpty() && _Host.IsEmpty() && _PortVal == 0)
        {
            return TRUE;
        }
        return FALSE;
    }

    // Compare
    //
    // Convenience function which tests two URIs for precise identity. All fields are checked, including
    // query string, port, etc.
    //
    // Note that the authorities of the two URIs must match, since we compare the raw URI string.
    // For a test which takes into account wildcard host name values, use the version of Compare()
    // below which allows field-by-field tests.
    //
    // Returns TRUE if all fields are identical, FALSE if not.
    //
    BOOLEAN
    Compare(
        __in KUriView const & Comparand
        ) const
    {
        return Compare(eRaw, Comparand);
    }


    // Compare
    //
    // An overload which checks only the fields in the specified bitmask.
    //
    // If eHost is specified, a wildcard host name value * or + will match
    // any other host.
    // To check the host for precise identity without wildcards, use eLiteralHost.
    //
    // Returns TRUE if the specified fields are the same, FALSE if not.
    //
    BOOLEAN
    Compare(
        __in ULONG FieldsToCompare,     // UriFieldType mask
        __in KUriView const & Comparand
        ) const;

    // operator==
    //
    // Tests two URIs for precise identity. All fields are checked, including
    // query string, port, etc.
    BOOLEAN operator==(
        __in KUriView const & Other
        ) const
    {
        return Compare(Other);
    }

    // operator!=
    //
    // Tests two URIs for precise identity. All fields are checked, including
    // query string, port, etc.
    BOOLEAN operator!=(
        __in KUriView const & Other
        ) const
    {
        return Compare(Other) == FALSE;
    }

    // KUriView constructor overload
    //
    // Takes a raw string.  Parsing occurs during construction, so verify the URI, call IsValid() afterwards.
    //
    KUriView(
        __in LPCWSTR RawUri
        )
        {
#if defined(PLATFORM_UNIX)
            KStringView StrView(RawUri);
            Initialize(StrView);
#else
            Initialize(KStringView(RawUri));
#endif
        }

    // KUriView constructor overload.
    //
    // Takes KStringView.  Parsing occurs during construction, so verify the URI, call IsValid() afterwards.
    //
    KUriView(
        __in KStringView RawUri
        )
        {
            Initialize(RawUri);
        }

    // Get
    //
    // Returns the specified parsed field.  This only works with one field at a time, i.e.,
    // you cannot bitwise OR several fields.
    //
    // Parameters:
    //      Field           The requested field of the URI.  This may return an empty string view,
    //                      so test the returned value with KStringView::IsEmpty() before assuming
    //                      there is a non-empty string being returned.
    //
    //
    KStringView&
        Get(
            __in UriFieldType Field
        );

    // Get const
    //
    // Returns the specified parsed field.  This only works with one field at a time, i.e.,
    // you cannot bitwise OR several fields.
    //
    // Parameters:
    //      Field           The requested field of the URI.  This may return an empty string view,
    //                      so test the returned value with KStringView::IsEmpty() before assuming
    //                      there is a non-empty string being returned.
    //
    //
    const KStringView&
    Get(
        __in UriFieldType Field
        ) const;

    // GetPort
    //
    // Returns the port value converted to a number.  Since 0 is not a valid port number,
    // a return value of zero indicates that no port was specified.
    //
    ULONG
    GetPort() const
    {
        return _PortVal;
    }

    //  GetErrorPosition
    //
    //  Returns the offset at which the parser encountered the error in the URI if it is not valid.
    //
    ULONG
    GetErrorPosition() const
    {
        return _ErrorPos;
    }

    //
    //  operator KStringView&
    //
    operator KStringView&()
    {
        return _Raw;
    }

    operator LPCWSTR()
    {
        return LPCWSTR(_Raw);
    }


    operator PWSTR()
    {
        return PWSTR(_Raw);
    }

    // HasStandardQueryString
    //
    // Returns TRUE if the query string is one of the standardized name/value pairs, FALSE if it is an ad-hoc
    // query string.
    //
    BOOLEAN
    HasStandardQueryString() const
    {
        return _StandardQueryString;
    }

protected:

    // SegmentRecognized
    //
    // This is implemented by derived classes to build the segment array.
    // In the base class, since we can't allocate, we just ignore these calls from the parser.
    //
    // Returning FALSE from this will cause parsing to halt with an error.  This may be needed
    // if the higher-level semantics determine that the segment value is invalid (even though syntactically correct).
    //
    //
    virtual BOOLEAN
    SegmentRecognized(
        __in ULONG SegmentIndex,
        __in const KStringView& Seg
        )
        {
            UNREFERENCED_PARAMETER(SegmentIndex);
            UNREFERENCED_PARAMETER(Seg);
            return TRUE;
        }

    // QueryParamRecognized
    //
    // This is called each time a query string parameter is recognized. If the implementation returns
    // FALSE, the parser will halt and it will result in an overall parsing error and invalid URI.
    //
    // Derived classes will implement this to build a dynamic array of query string parameters.
    //
    virtual BOOLEAN
    QueryStringParamRecognized(
        __in ULONG ParamIndex,
        __in const KStringView& ParamName,
        __in const KStringView& ParamVal
        )
    {
        UNREFERENCED_PARAMETER(ParamIndex);
        UNREFERENCED_PARAMETER(ParamName);
        UNREFERENCED_PARAMETER(ParamVal);
        return TRUE;
    }

    VOID
    Initialize(
        __in const KStringView& Src
        );

    VOID
    Clear();

    BOOLEAN
    ParserCallback(
        ULONG UriElement,
        const KStringView& Val,
        ULONG SegIndex
        );

    BOOLEAN
    QueryStrCallback(
        __in const KStringView& Param,
        __in const KStringView& Val,
        __in ULONG ParamNum
        );


    // Data members
    //
    KStringView _Scheme;
    KStringView _Authority;
    KStringView _Host;
    ULONG       _PortVal;
    ULONG       _ErrorPos;
    KStringView _Path;
    KStringView _QueryString;
    KStringView _Fragment;
    KStringView _Raw;
    BOOLEAN     _IsValid;
    BOOLEAN     _StandardQueryString;
};

// class KDynUri
//
// This version of the class allows access to the individual query string parameters and segments.
// This can be easily updated to allow full read/write access if we need to rewrite or edit URIs on the fly.
//
//
class KDynUri : public KUriView, public KObject<KDynUri>
{
    K_DENY_COPY(KDynUri);

public:
    //
    //  Constructor with no associated URI; for use as member in another struct.
    //
    KDynUri(
        __in KAllocator& Allocator
        )
        : _Allocator(Allocator), _src_Raw(Allocator), _Segments(Allocator), _QueryStrParams(Allocator)
        {
        }

    // Constructor which takes an external string. Call IsValid() before using the URI to ensure it is valid.
    //
    KDynUri(
        __in const KStringView Src,
        __in KAllocator& Allocator
        )
        :  _Allocator(Allocator), _src_Raw(Allocator), _Segments(Allocator), _QueryStrParams(Allocator)
        {
		    HRESULT hr;
			ULONG result;
			hr = ULongAdd(Src.Length(), 1, &result);
			KInvariant(SUCCEEDED(hr));
            _src_Raw.Resize(result);
            _src_Raw.CopyFrom(Src);
            _src_Raw.SetNullTerminator();
            Initialize(_src_Raw);
            if (!_StandardQueryString)
            {
                _QueryStrParams.Clear();
            }
        }

    // Clear
    //
    // Clears the URI to an empty state.
    //
    VOID
    Clear()
    {
         _QueryStrParams.Clear();
         _Segments.Clear();
         _src_Raw.Clear();
         KUriView::Clear();
    }

    //  Set
    //
    //  Sets the object to a new URI value. This is used when the object is constructd without a URI.
    //
    NTSTATUS
    Set(
        __in KStringView Src
        )
    {
         Clear();
         KStringView& RawSrc = Src;

 		 HRESULT hr;
		 ULONG result;
		 hr = ULongAdd(RawSrc.Length(), 1, &result);
		 KInvariant(SUCCEEDED(hr));

		 _src_Raw.Resize(result);
         _src_Raw.CopyFrom(RawSrc);
         _src_Raw.SetNullTerminator();
         Initialize(_src_Raw);
         if (!_StandardQueryString)
         {
             _QueryStrParams.Clear();
         }
         if (!IsValid())
         {
            return STATUS_OBJECT_PATH_SYNTAX_BAD;
         }
         return STATUS_SUCCESS;
    }

    // IsPrefixFor
    //
    // Tests the current URI to see if it is a prefix match for the longer Candidate.
    //
    // Returns TRUE if the current URI has a common prefix with the Candidate. For example,
    //
    //    *this = foo://a/b/c
    //    Candidate = fo0://a/b/c/d/e
    //
    // ...returns TRUE
    //
    // Also returns TRUE if the two URIs are equivalent. FALSE is returned if the Candidate
    // is shorter or if it doesn't match at all.
    //
    // This only includes the scheme, authority, and path, and does not include query string/fragment.
    //
    BOOLEAN
    IsPrefixFor(
        __in KDynUri& Candidate
        );


    // GetSuffix
    //
    // Returns the suffix portion of the Candidate which is longer than *this.
    // If *this is  foo:/a/b/c and Candidate is foo:/a/b/c/d/e then Suffix will contain
    // d/e on exit.
    //
    // Parameters:
    //      Candidate               The URI from which to extract the suffix, if any.
    //      Suffix                  Receives the suffix.
    //
    // Return value:
    //      STATUS_SUCCESS                  If the suffix was retrieved, including a zero-length suffix if the Candidate is
    //                                      the same as *this.
    //      STATUS_INSUFFICIENT_RESOURCES   If the Suffix could not be allocated.
    //      STATUS_UNSUCCESSFUL             The candidate did not have a matching prefix with *this.
    //      STATUS_INVALID_PARAMETER        If the candidate is invalid
    //
    NTSTATUS
    GetSuffix(
        __in  KDynUri& Candidate,
        __out KDynUri& Suffix
        );


    // Clone
    //
    // Convenience function for simple clone of entire URI.
    //
    NTSTATUS
    Clone(
        __out KDynUri& NewCopy
        );


    // GetSegmentCount
    //
    // Receives the number of segments in the URI.
    //
    ULONG
    GetSegmentCount()
    {
        return _Segments.Count();
    }

    // GetSegment
    //
    // For efficiently returns an internal reference to the specified segment, which is read-only.
    //
    // Parameters:
    //      SegmentIndex        The zero-origin segment value to be retrieved.
    //      Segment             Receives the segement value, to be treated as read-only.
    //
    // Return value:
    //      TRUE if the segment was valid, FALSE if the specified index is out of range.
    //
    _Success_(return == TRUE)
	BOOLEAN
    GetSegment(
        __in  ULONG SegmentIndex,
        __out KStringView& Segment
        )
    {
        if (SegmentIndex >= _Segments.Count())
        {
            return FALSE;
        }
        Segment = *_Segments[SegmentIndex];
        return TRUE;
    }

    // Receives the number of query string parameters.
    //
    // This will return 0 if the query string is non-standard.
    //
    ULONG
    GetQueryStringParamCount()
    {
        return _QueryStrParams.Count();
    }

    // GetQueryStringParam
    //
    // For efficiently returns an internal reference to the specified segment, which is read-only.
    //
    // Parameters:
    //      SegmentIndex        The zero-origin parameter to be retrieved.
    //      ParamName           Receives the the parameter name.
    //      ParamValue          Receives the parameter value.
    //
    // Return value:
    //      TRUE if the segment was valid, FALSE if the specified index is out of range.
    //
	_Success_(return == TRUE)
	BOOLEAN
    GetQueryStringParam(
        __in  ULONG Index,
        __out KStringView& ParamName,
        __out KStringView& ParamValue
        )
     {
        if (Index >= _QueryStrParams.Count())
        {
            return FALSE;
        }

        ParamName = _QueryStrParams[Index]->_Param;
        ParamValue = _QueryStrParams[Index]->_Value;
        return TRUE;
     }

private:
    //
    //  SetSegment
    //
    //  This is private for now until we determine if partial URI rewriting is needed.
    //
    NTSTATUS
    SetSegment(
        __in ULONG SegmentIndex,
        __in const KStringView& NewValue
        )
    {
        if (SegmentIndex > _Segments.Count())
        {
            return FALSE;
        }

        // Clone the string
        //
        KString::SPtr NewStr = KString::Create(NewValue, _Allocator);
        if (!NewStr)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        // Append
        //
        if (SegmentIndex == _Segments.Count())
        {
            return _Segments.Append(NewStr);
        }

        // Else a rewrite
        //
        _Segments[SegmentIndex] = NewStr;  // Wipe previous elemnet
        return STATUS_SUCCESS;
    }


    virtual BOOLEAN
    SegmentRecognized(
        __in ULONG SegmentIndex,
        __in const KStringView& Seg
        )
        {
            NTSTATUS Res = SetSegment(SegmentIndex, Seg);
            if (NT_SUCCESS(Res))
            {
                return TRUE;
            }
            return FALSE;
        }


    virtual BOOLEAN
    QueryStringParamRecognized(
        __in ULONG ParamIndex,
        __in const KStringView& ParamName,
        __in const KStringView& ParamVal
        );

    struct QueryStrParam : public KObject<QueryStrParam>, public KShared<QueryStrParam>
    {
        typedef KSharedPtr<QueryStrParam> SPtr;

        KDynString _Param;
        KDynString _Value;

        QueryStrParam(
            __in KAllocator& Alloc
            )
            : _Param(Alloc), _Value(Alloc)
            {
            }
    };

    KAllocator&                   _Allocator;
    KArray<KString::SPtr>         _Segments;
    KArray<QueryStrParam::SPtr>   _QueryStrParams;

    KDynString  _src_Raw;        // Mapped to the base class _Raw during construction
};

//
//  class KUri
//
//  An KShared version of KDynUri.
//
//
class KUri : public KShared<KUri>, public KObject<KUri>, public KDynUri
{
    K_FORCE_SHARED_WITH_INHERITANCE(KUri);

public:
    typedef KSharedPtr<KUri> SPtr;

    static NTSTATUS
    Create(
        __in  const KStringView& RawString,
        __in  KAllocator& Alloc,
        __out KUri::SPtr& Uri
        );

    static NTSTATUS
    Create(
        __in  const KStringView& RawString,
        __in  KAllocator& Alloc,
        __out KUri::CSPtr& Uri
        );

    static NTSTATUS
    Create(
        __in  KUriView& ExistingUri,
        __in  KAllocator& Alloc,
        __out KUri::SPtr& Uri
        );

    static NTSTATUS
    Create(
        __in  KUriView& ExistingUri,
        __in  KAllocator& Alloc,
        __out KUri::CSPtr& Uri
        );

    //  Create (overload)
    //
    //  Creates a new URI based on a composite of a prefix URI and a relative
    //  URI that should be logically concatenated to it.
    //
    static NTSTATUS
    Create(
        __in KUriView& PrefixUri,
        __in KUriView& RelativeUri,
        __in KAllocator& Alloc,
        __out KUri::SPtr& NewCompositeUri
        );

    NTSTATUS
    Clone(
        __in  KAllocator& Alloc,
        __out KUri::SPtr& Uri
        )
    {
         return Create(*this, Alloc, Uri);
    }


    static NTSTATUS
    Create(
        __in  KAllocator& Alloc,
        __out KUri::SPtr& Uri
        );

    // Clone
    //
    // Convenience function for simple clone of entire URI.
    //
    NTSTATUS
    Clone(
        __out KUri::SPtr& NewCopy
        );

    friend BOOLEAN operator< (const KUri& lhs, const KUri& rhs);
    friend BOOLEAN operator> (const KUri& lhs, const KUri& rhs);
    friend BOOLEAN operator== (const KUri& lhs, const KUri& rhs);

private:
    KUri(__in KAllocator&);
    KUri(__in const KStringView& Src, __in KAllocator&);
    KUri(__in KUriView& Src, KAllocator&);
};

inline
BOOLEAN operator< (const KUri& lhs, const KUri& rhs)
{
    return lhs._Raw.Compare(rhs._Raw) < 0;
}

inline
BOOLEAN operator>= (const KUri& lhs, const KUri& rhs)
{
    return !(lhs < rhs);
}

inline
BOOLEAN operator> (const KUri& lhs, const KUri& rhs)
{
    return lhs._Raw.Compare(rhs._Raw) > 0;
}

inline
BOOLEAN operator<= (const KUri& lhs, const KUri& rhs)
{
    return !(lhs > rhs);
}

inline
BOOLEAN operator== (const KUri& lhs, const KUri& rhs)
{
    return lhs._Raw.Compare(rhs._Raw) == 0;
}

inline
BOOLEAN operator!= (const KUri& lhs, const KUri& rhs)
{
    return !(lhs == rhs);
}
