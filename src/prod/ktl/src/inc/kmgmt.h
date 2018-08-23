
/*++

    (c) 2012 by Microsoft Corp. All Rights Reserved.

    kmgmt.h

    Description:
      Kernel Tempate Library (KTL): Management provider abstract interface

    History:
      alanwar, raymcc    13-Jul-2012         Initial version.

--*/

#pragma once

class KIManagementOps : public KAsyncContextBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(KIManagementOps);

public:
    // StartGetAsync
    //
    // Gets an resource/instrumentation value.
    //
    // Parameters:
    //      Target              The URI of the resources requested.
    //
    //      RequestHeaders      May be NULL.  If the request arrived on the network,
    //                          this object will be populated with any headers supplied by the client.
    //                          The provider may require certain values to be present and return an error if they are not present.
    //
    //      ResponseHeaders     Receives any header values that the provider wants to be included in the response.
    //                          This may be set to NULL.  The provider (callee) is responsible for allocation if it is populated.
    //
    //      ResponseBody        This may be the updated resource or may be an informational object which indicates the status of the update
    //                          if creation takes a long time.  In all cases, the ResponseBody should contain a URI value which can be
    //                          subsequently retrieved using a StartGetAsync so that the update process can be monitored.
    //
    //                          The provider (callee) is responsible for allocation.
    //
    //                          This may be set to a descriptive error object in cases where the failure to create requires complex
    //                          error/diagnostic information to be sent back to the caller.
    //
    //      Callback            Async completion callback.
    //      Parent              Parent async.
    //
    //  Standard async completion status codes:
    //      STATUS_SUCCESS                  The item was retrieved
    //      STATUS_OBJECT_PATH_INVALID      The Target URI was not valid or recognized by the provider.
    //      STATUS_NOT_FOUND                The requested item was not found, although the URI itself was valid.(Can occur after a deletion)
    //      STATUS_INSUFFICIENT_RESOURCES
    //
    virtual NTSTATUS
    StartGetAsync(
        __in  KUriView& Target,
        __in  KIDomNode::SPtr RequestHeaders,
        __out KIMutableDomNode::SPtr& ResponseHeaders,
        __out KIMutableDomNode::SPtr& ResponseBody,
        __in  KAsyncContextBase::CompletionCallback Callback,
        __out_opt KAsyncContextBase::SPtr *ChildAsync = nullptr,
        __in_opt KAsyncContextBase* Parent = NULL,
        __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr
        ) = 0;

    // StartCreateAsync
    //
    // Begins creation of a new resource.
    //
    // Parameters:
    //      Target              The URI of the factory or constructor of the resource to be created.  This is not the proposed URI
    //                          of the resource being created.
    //      RequestHeaders      May be NULL.  If the request arrived on the network,
    //                          this object will be populated with any headers supplied by the client.
    //      RequestBody         Contains any necessary values to create or construct the new instance.
    //
    //      ResponseHeaders     Receives any header values that the provider wants to be included in the response.
    //                          This may be set to NULL.  The provider (callee) is responsible for allocation if it is populated.
    //
    //      ResponseBody        This may be the created resource or may be an informational object which indicates the status of the creation
    //                          if creation takes a long time.  In all cases, the ResponseBody should contain a URI value which can be
    //                          subsequently retrieved using a StartGetAsync so that the creation process can be monitored.
    //
    //                          The provider (callee) is responsible for allocation.
    //
    //                          This may be set to a descriptive error object in cases where the failure to create requires complex
    //                          error/diagnostic information to be sent back to the caller.
    //
    //      Callback            Async completion callback.
    //      Parent              Parent async.
    //
    //  Standard async completion status codes:
    //      STATUS_SUCCESS                  The creation process was initiated or completed, depending on the provdier.
    //      STATUS_OBJECT_PATH_INVALID      The Target URI was not valid or recognized by the provider.
    //      STATUS_INVALID_PARAMETER_1 .. n
    //      STATUS_INSUFFICIENT_RESOURCES
    //
    virtual NTSTATUS
    StartCreateAsync(
        __in KUriView& Target,
        __in KIDomNode::SPtr RequestHeaders,
        __in KIDomNode::SPtr RequestBody,
        __out KIMutableDomNode::SPtr& ResponseHeaders,
        __out KIMutableDomNode::SPtr& ResponseBody,
        __in KAsyncContextBase::CompletionCallback Callback,
        __out_opt KAsyncContextBase::SPtr *ChildAsync = nullptr,
        __in_opt KAsyncContextBase* Parent = NULL,
        __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr
        ) = 0;




    // StartUpdateAsync
    //
    // Begins the updating of an existing resource.
    //
    // Versioning conventions are indicated in the RequestHeaders.
    //
    // Parameters:
    //      Target              The URI of the resource to be updated.
    //      RequestHeaders      May be NULL.  If the request arrived on the network,
    //                          this object will be populated with any headers supplied by the client.
    //      RequestBody         Contains the proposed update value for the entity.
    //
    //      ResponseHeaders     Receives any header values that the provider wants to be included in the response.
    //                          This may be set to NULL.  The provider (callee) is responsible for allocation if it is populated.
    //
    //      ResponseBody        This may be the updated resource or may be an informational object which indicates the status of the update
    //                          if creation takes a long time.  In all cases, the ResponseBody should contain a URI value which can be
    //                          subsequently retrieved using a StartGetAsync so that the update process can be monitored.
    //
    //                          The provider (callee) is responsible for allocation.
    //
    //                          This may be set to a descriptive error object in cases where the failure to create requires complex
    //                          error/diagnostic information to be sent back to the caller.
    //
    //      Callback            Async completion callback.
    //      Parent              Parent async.
    //
    //  Standard async completion status codes:
    //      STATUS_SUCCESS                  The update process was initiated or completed, depending on the provdier.
    //      STATUS_OBJECT_PATH_INVALID      The Target URI was not valid or recognized by the provider.
    //      STATUS_INSUFFICIENT_RESOURCES
    //      STATUS_INVALID_PARAMETER_1 .. n

    virtual NTSTATUS
    StartUpdateAsync(
        __in KUriView& Target,
        __in KIDomNode::SPtr RequestHeaders,
        __in KIDomNode::SPtr RequestBody,
        __out KIMutableDomNode::SPtr& ResponseHeaders,
        __out KIMutableDomNode::SPtr& ResponseBody,
        __in KAsyncContextBase::CompletionCallback Callback,
        __out_opt KAsyncContextBase::SPtr *ChildAsync = nullptr,
        __in_opt KAsyncContextBase* Parent = NULL,
        __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr
        ) = 0;


    // StartDeleteAsync
    //
    // Begins the updating of an existing resource.
    //
    // Versioning and drain-vs-kill conventions are indicated in the RequestHeaders.
    //
    // Parameters:
    //      Target              The URI of the resource to be deleted.
    //      RequestHeaders      May be NULL.  If the request arrived on the network,
    //                          this object will be populated with any headers supplied by the client.
    //
    //      ResponseHeaders     Receives any header values that the provider wants to be included in the response.
    //                          This may be set to NULL.  The provider (callee) is responsible for allocation if it is populated.
    //
    //      ResponseBody        This may be the final versio of the deleted resource or may be an informational object which indicates the status of the deletion
    //                          if deletion takes a long time.  In all cases, the ResponseBody should contain a URI value which can be
    //                          subsequently retrieved using a StartGetAsync so that the final status of the deletion can be monitored.  That URI
    //                          is usually to an archival or audit object.
    //
    //                          The provider (callee) is responsible for allocation.
    //
    //                          This may be set to a descriptive error object in cases where the failure to create requires complex
    //                          error/diagnostic information to be sent back to the caller.
    //
    //      Callback            Async completion callback.
    //      Parent              Parent async.
    //
    //  Standard async completion status codes:
    //      STATUS_SUCCESS                  The update deletion was initiated or completed, depending on the provdier.
    //      STATUS_OBJECT_PATH_INVALID      The Target URI was not valid or recognized by the provider.
    //      STATUS_INSUFFICIENT_RESOURCES
    //      STATUS_INVALID_PARAMETER_1 .. n

    virtual NTSTATUS
    StartDeleteAsync(
        __in KUriView& Target,
        __in KIDomNode::SPtr RequestHeaders,
        __out KIMutableDomNode::SPtr& ResponseHeaders,
        __out KIMutableDomNode::SPtr& ResponseBody,
        __in KAsyncContextBase::CompletionCallback Callback,
        __out_opt KAsyncContextBase::SPtr *ChildAsync = nullptr,
        __in_opt KAsyncContextBase* Parent = NULL,
        __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr
        ) = 0;

    // StartQueryAsync
    //
    // Executes a full or partial query over a resource which is a container.  The paradigm is for stateless operation,
    // so queries which have very large result sets can only be executed by making more than one call, supplying a bookmark
    // (cookie/token/cursor, etc.) in subsequent calls.   The implementation must be able to continue where the 'bookmark'
    // left off on the previous call.  There is no expectation or suggestion that the implementation cache any result sets.
    // Because the result set can become incoherent if updates occur between calls to this API, the provider is free to
    // return an error and require the caller to start over, or it may have an intelligent bookmark and adjust the remaining
    // results accordingly.  This interface does not specify any such behaviors; it only must be able to transmit them if
    // there is a convention to be observed.
    //
    // If multi-step queries are required due to very large result sets, the bookmark/cursor usage
    // is established within the request and response headers.
    //
    // Parameters:
    //      Target              The URI of the container resource to be queried.
    //      Query               A DOM object representing a query.  If a null SPtr, then "enumerate" is assumed.
    //
    //      RequestHeaders      May be NULL.  If the request arrived on the network,
    //                          this object will be populated with any headers supplied by the client.
    //
    //      ResponseHeaders     Receives any header values that the provider wants to be included in the response.
    //                          This may be set to NULL.  The provider (callee) is responsible for allocation if it is populated.
    //
    //      ResponseBodies      The array should be populated with DOM objects which represent the result set of the query.
    //
    //                          The provider (callee) is responsible for allocation.
    //
    //      MoreData            This must be set to TRUE if the query contained more results than were actually returned.
    //                          In this case, the ResponseHeaders should contain bookmarks, cookies, or cursor data that can be used
    //                          in a subsequent call to continue where the previous enumeration left off.
    //
    //      Callback            Async completion callback.
    //      Parent              Parent async.
    //
    //      A query which returns zero results is legal and correct and should not result in an error code.  Errors should
    //      be returned only if there is an actual error in processing the query.
    //
    //  Standard async completion status codes:
    //      STATUS_SUCCESS                  The query batch was successfully generated.
    //      STATUS_OBJECT_PATH_INVALID      The Target URI was not valid or recognized by the provider.
    //      STATUS_INSUFFICIENT_RESOURCES
    //      STATUS_INVALID_PARAMETER_1 .. n
    //
    virtual NTSTATUS
    StartQueryAsync(
        __in  KUriView& Target,
        __in  KIDomNode::SPtr Query,
        __in  KIDomNode::SPtr RequestHeaders,
        __out KIMutableDomNode::SPtr& ResponseHeaders,
        __out KArray<KIMutableDomNode::SPtr>& ResponseBodies,
        __out BOOLEAN& MoreData,
        __in KAsyncContextBase::CompletionCallback Callback,
        __out_opt KAsyncContextBase::SPtr *ChildAsync = nullptr,
        __in_opt KAsyncContextBase* Parent = NULL,
        __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr
        ) = 0;

    // StartInvokeAsync
    //
    // This is intended for non-rest "punctual" operations, such as "Restart" or whatever.  Its used should be minimized.
    //
    virtual NTSTATUS
    StartInvokeAsync(
        __in  KUriView& Target,
        __in  KUriView& Verb,
        __in  KIDomNode::SPtr RequestHeaders,
        __in  KIDomNode::SPtr RequestBody,
        __out KIMutableDomNode::SPtr& ResponseHeaders,
        __out KIMutableDomNode::SPtr& ResponseBody,
        __in KAsyncContextBase::CompletionCallback Callback,
        __out_opt KAsyncContextBase::SPtr *ChildAsync = nullptr,
        __in_opt KAsyncContextBase* Parent = NULL,
        __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr
        ) = 0;
};



