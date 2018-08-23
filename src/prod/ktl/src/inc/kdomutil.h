
/*++

    (c) 2012 by Microsoft Corp. All Rights Reserved.

    kdomutil.h

    Description:
      Kernel Tempate Library (KTL): KDomPath, KDomDoc

      Wrappers and helpers for KDom objects.

    History:
      raymcc          19-Jul-2012

--*/

#pragma once


//
//  KDomPath
//
//  Models the path into a Dom document to a value or attribute.
//  Can also be used for accessing arrays of elements at any level.
//
//  Syntax:
//      a/b/c       References element c
//      a/b/c/@d    References attribute d on element c
//      a/b[0]      References the first child b under a
//      a/b[1]      References teh second child of b under a
//
//  In cases where it is not convenient to use literal array values in the path,
//  the other overloads are used and the path syntax uses a placeholder for the array:
//
//      KDomPath("a/b[]", 5);           // References the 6th child b of a
//      KDomPath("a/b[]/e/c[]", 2, 3)   // References the 4th occurrence of c under e under the 3rd occurrence of b under a
//
class KDomPath
{
public:
    KDomPath(
        __in LPCWSTR Src
        );

    KDomPath(
        __in LPCWSTR Src,
        __in ULONG Ix1
        );

    KDomPath(
        __in LPCWSTR Src,
        __in ULONG Ix1,
        __in ULONG Ix2
        );

    KDomPath(
        __in LPCWSTR Src,
        __in ULONG Ix1,
        __in ULONG Ix2,
        __in ULONG Ix3
        );

private:
};

//  KDomDoc
//
//  Convenience wrapper over KIMutableDomNode.
//
class KDomDoc
{
public:
    KDomDoc(
        __in KAllocator& Alloc
        );

    // Set
    //
    // Sets the specified value a the specified path. Creates any required intervening nodes to make the
    // final result at the correct location.
    //
    // Type T supports all major types in KTL, including KVariant.  If a simple type is used it will be
    // coerced to the nearest correct KVariant type for storage in the DOM.
    //
    // This should always be called with explicit template instantiation so that the type is not left to chance:
    //
    //      doc.Set<ULONG>(KDomPath("a/b/c", 100));
    //
    // This also works if the value type is itself a KDomDoc.
    //
    // Parameters:
    //      Path            The path to the value in the document.
    //      Value           The value.
    //
    // If FALSE is returned, then the operation failed.  However, it is not required to check every call,
    // as the error model is cumulative.
    //
    template <typename T>
    BOOLEAN
    Set(
        __in KDomPath& Path,
        __in T& Value
        );

    // Set
    //
    // Sets an empty value at the specified path.  This used to ensure that an element or attribute is present (with no vaLue) and
    // can be used to build up a document.
    //
    // Parameters:
    //      Path            The path to the value in the document.
    //
    // If FALSE is returned, then the operation failed.  However, it is not required to check every call,
    // as the error model is cumulative.
    //
    BOOLEAN
    Set(
        __in KDomPath& Path
        );

    // Get
    //
    // Retrieves a value.
    //
    // Parameters:
    //      Path            The path to the value.
    //      Value           Receives the value.  Supports all major types, including KVariant and KDomDoc itself.
    //
    template <typename T>
    BOOLEAN
    Get(
        __in  KDomPath& Path,
        __out T& Value
        );

    // GetLastError
    //
    // Returns an NTSTATUS code if the last operation returned FALSE.
    //
    // Once STATUS_INSUFFICIENT_RESOURCES has occurred, then that status remains and all future operations
    // will fail.
    //
    NTSTATUS
    GetLastError();

    // ErrorCount
    //
    // Returns the total number of errors that have occurred.
    //
    ULONG
    ErrorCount();

    // GetErrorTranscript
    //
    // Returns a string containing the cumulative error transcript. This works with GetErrorCount().  If GetErrorCount()
    // returns non-zero, then calling this will return all the errors that have occurred.   This is primarily
    // intended for debugging.
    //
    //
    NTSTATUS
    GetErrorTranscript(
        __out KString::SPtr ErrorStr
        );

    // ClearErrors
    //
    // Clears the error count, transcript and last error value.
    //
    VOID
    ClearErrors();


    // Bind
    //
    // Associates a memory location with a DOM value.  THis is for use with TransferToDoc or TransferFromDoc.
    //
    // Use this form for associating an actual address
    // with the path.
    //
    template <typename T>
    BOOLEAN
    Bind(
        __in KDomPath& Path,
        __in T* Address
        );


    NTSTATUS
    TransferToDoc();


    NTSTATUS
    TransferFromDoc();


    // Delete
    //
    BOOLEAN
    Delete(
        __in  KDomPath& Path
        );

    // Append
    //
    template <typename T>
    BOOLEAN
    Append(
        __in  KDomPath& Path,
        __in  T& Value
        );

    ULONG
    Count(
        __in  KDomPath& Path
        );

    // Interaction with DOM
    //
    NTSTATUS
    SetDom(
        __in KIMutableDomNode::SPtr Source
        );

    NTSTATUS
    GetDom(
        __out KIMutableDomNode::SPtr& Dom
        );

    NTSTATUS
    ToString(
        __out KString::SPtr& Stringized
        );

    NTSTATUS
    FromString(
        __in KStringView& View
        );

private:
    KIMutableDomNode::SPtr _Root;
};


