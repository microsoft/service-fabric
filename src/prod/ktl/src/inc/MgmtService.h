/*--

Copyright (c) 2011  Microsoft Corporation

Module Name:

    MgmtService.h

Abstract:

    This file implements a base class that can be used to implement an
    RVD management service that sits on top of a KMgmtServer object.
    This class is a bridge between the KMgmtServer and the specific
    management service and abstracts out much of the common
    functionality needed by a service when working with KMgmtServer.

Author:

    Alan Warwick (AlanWar) 2-July-2012

Environment:

Notes:

Revision History:

--*/

const ULONG TAG_KMANAGEMENT_SERVICE = 'TSMK';        // 'KMST' Default TAG for KManagementService() allocations.


//
// KManagementService is the base class that implements the common
// functionality needed by a management service. To use this class, a
// management service would derive from this class and implement
// whatever virtual methods it needs depending upon the functionality
// of the management service.
//
// One set of methods that can be overriden is the set of verb
// handlers. Each verb has a corresponding verb handler that is used to
// implement the actions corresponding to that verb. For example the
// CreateHandler() is invoked when a create verb is received. Derived
// classes can override a handler to perform work. If the handler is
// not overridden then the default handler will return an error to the
// caller.
//
// Another set of overrides is used for creation and initialization of
// the ManagementRequestContext structures. Each incoming request is
// represented by one of these. A derived class can derive a new class
// from KManagementService::ManagementRequestContext that will service
// as the ManagementRequestContext used by the base class so that when
// the handler receives a callback it can then downcast the
// ManagementRequestContext to its own derived type. To do this the
// derived class would need to override
// CreateManagementRequestContext() in the KManagementService class and
// optionally override InitializeForUse() and OnReuse() in the
// ManagmentRequestContext class
//
class KManagementService : public KIManagementOps
{
    K_FORCE_SHARED_WITH_INHERITANCE(KManagementService);

    protected:
        class ManagementRequestContext;
        friend ManagementRequestContext;
    
    private:
//        static const KTagType _ThisTypeTag;

    //
    // Public apis
    //
    public:
        //
        // These are the values that correspond to the verbs specified
        // in the request
        //
        enum Verbs { NoVerb, GetVerb, CreateVerb, UpdateVerb, QueryVerb, DeleteVerb, InvokeVerb };

        //
        // Creation method. Use this method to create the
        // KManagementService object itself. Once the service is
        // created Activate() can be used to enable it while
        // Deactivate() can be used to disable it. The
        // KManagementService async itself will complete only when
        // deactivation is finished.
        //
        static NTSTATUS Create(
            __out KManagementService::SPtr& Service,
            __in  KAllocator& Allocator,
            __in  ULONG AllocationTag = TAG_KMANAGEMENT_SERVICE
        );
            

        //
        // Activate will register for callbacks from KMgmtServer
        //
        NTSTATUS Activate(
            __in  KMgmtServer::SPtr MgmtServer,
            __in  KUriView& ProviderUri,
            __in  KStringView& Description,
            // TODO: list of objects/URI that are handled
            __in  KAsyncContextBase::CompletionCallback Callback,
            __out_opt KAsyncContextBase::SPtr *ChildAsync = nullptr,
            __in_opt KAsyncContextBase* Parent = NULL,
            __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr                       
        );

        //
        // Deactivate will shutdown the management service
        //
        VOID Deactivate(
        );

    //
    // Entrypoints for management operations called by KMgmtServer.
    // These should not be called directly.
    //
    public:
        
        NTSTATUS
        StartGetAsync(
            __in  KUriView& Target,
            __in  KIDomNode::SPtr RequestHeaders,
            __out KIMutableDomNode::SPtr& ResponseHeaders,
            __out KIMutableDomNode::SPtr& ResponseBody,
            __in  KAsyncContextBase::CompletionCallback Callback,
            __out_opt KAsyncContextBase::SPtr *ChildAsync,
            __in_opt KAsyncContextBase* Parent,
            __in_opt KAsyncGlobalContext* const GlobalContextOverride
            ) override ;

        NTSTATUS
        StartCreateAsync(
            __in KUriView& Target,
            __in KIDomNode::SPtr RequestHeaders,
            __in KIDomNode::SPtr RequestBody,
            __out KIMutableDomNode::SPtr& ResponseHeaders,
            __out KIMutableDomNode::SPtr& ResponseBody,
            __in KAsyncContextBase::CompletionCallback Callback,
            __out_opt KAsyncContextBase::SPtr *ChildAsync,
            __in_opt KAsyncContextBase* Parent,
            __in_opt KAsyncGlobalContext* const GlobalContextOverride
            ) override ;

        NTSTATUS
        StartUpdateAsync(
            __in KUriView& Target,
            __in KIDomNode::SPtr RequestHeaders,
            __in KIDomNode::SPtr RequestBody,
            __out KIMutableDomNode::SPtr& ResponseHeaders,
            __out KIMutableDomNode::SPtr& ResponseBody,
            __in KAsyncContextBase::CompletionCallback Callback,
            __out_opt KAsyncContextBase::SPtr *ChildAsync,
            __in_opt KAsyncContextBase* Parent,
            __in_opt KAsyncGlobalContext* const GlobalContextOverride
            ) override ;

        NTSTATUS
        StartDeleteAsync(
            __in KUriView& Target,
            __in KIDomNode::SPtr RequestHeaders,
            __out KIMutableDomNode::SPtr& ResponseHeaders,
            __out KIMutableDomNode::SPtr& ResponseBody,
            __in KAsyncContextBase::CompletionCallback Callback,
            __out_opt KAsyncContextBase::SPtr *ChildAsync,
            __in_opt KAsyncContextBase* Parent,
            __in_opt KAsyncGlobalContext* const GlobalContextOverride
            ) override ;
        
        NTSTATUS
        StartQueryAsync(
            __in  KUriView& Target,
            __in  KIDomNode::SPtr Query,
            __in  KIDomNode::SPtr RequestHeaders,
            __out KIMutableDomNode::SPtr& ResponseHeaders,
            __out KArray<KIMutableDomNode::SPtr>& ResponseBodies,
            __out BOOLEAN& MoreData,
            __in KAsyncContextBase::CompletionCallback Callback,
            __out_opt KAsyncContextBase::SPtr *ChildAsync,
            __in_opt KAsyncContextBase* Parent,
            __in_opt KAsyncGlobalContext* const GlobalContextOverride
            ) override;

        NTSTATUS
        StartInvokeAsync(
            __in  KUriView& Target,
            __in  KUriView& Verb,
            __in  KIDomNode::SPtr RequestHeaders,
            __in  KIDomNode::SPtr RequestBody,
            __out KIMutableDomNode::SPtr& ResponseHeaders,
            __out KIMutableDomNode::SPtr& ResponseBody,
            __in KAsyncContextBase::CompletionCallback Callback,
            __out_opt KAsyncContextBase::SPtr *ChildAsync,
            __in_opt KAsyncContextBase* Parent,
            __in_opt KAsyncGlobalContext* const GlobalContextOverride
            ) override ;      
        
    protected:
        //
        // This async is used for performing an incoming management
        // request operation. It maintains all information about that specific
        // operation. This class can be overridden if the management
        // service needs to extend it.
        //
        class ManagementRequestContext : public KAsyncContextBase
        {
            friend KManagementService;
            
            K_FORCE_SHARED_WITH_INHERITANCE(ManagementRequestContext);

            private:
                static const KTagType _ThisTypeTag;

            protected:
                //
                // Any derived class must insure that the derived class
                // constructor forwards on to this base class
                // constructor
                //
                ManagementRequestContext(
                    __in KManagementService& Kms,
                    __in KIDomNode::SPtr RequestHeaders,                                       
                    __in KIDomNode::SPtr RequestBody,
                    __in KIMutableDomNode::SPtr& ResponseHeaders,
                    __in KIMutableDomNode::SPtr& ResponseBody
                );

                //
                // This routine will initialize the
                // ManagementRequestContext before processing
                // a new request. It may not be overriden however it
                // will call InitializeForUseDerived which itself may
                // be overriden.
                //
                NTSTATUS InitializeForUse(
                    __in Verbs Verb,
                    __in KUriView& Target
                    );

                virtual NTSTATUS InitializeForUseDerived(
                    __in Verbs Verb,
                    __in KUriView& Target
                    ) {return(STATUS_SUCCESS);};

                //
                // This routine will set the response status in the DOM
                // and complete the async.
                //
                // Status is typically STATUS_SUCCESS and denotes the
                // status of the async completion back to the HTTP
                // server.
                //
                // ResponseStatus is the status that is returned in the
                // HTTP response message.
                //
                // ResponseStatusText is optional text to include with the
                // response message.
                //
                VOID CompleteRequest(
                    __in NTSTATUS Status,
                    __in NTSTATUS ResponseStatus,
                    __in KStringView* ResponseStatusText = NULL
                    );

                //
                // This method will the response status and response
                // status text in the DOM for the response headers and
                // response body.
                // 
                //
                NTSTATUS SetStandardResponse(
                    __in NTSTATUS FinalStatus,
                    __in KStringView* FinalStatusText = NULL
                    );
                
            protected:
                VOID
                OnStart(
                    );

                //
                // This is a standard KTL OnReuse() method. It may not be
                // overridden but it will invoke OnReuseDerived() which
                // itself may be overriden.
                //
                VOID
                OnReuse(
                    );

                virtual VOID
                OnReuseDerived(
                    ) {};

                VOID
                OnCancel(
                    );


                //
                // This will copy the child node on the source to a new
                // child node on the destination.
                //
                // NOTE: this belongs in a DOM helper class
                //
                NTSTATUS CopyNodeValue(
                    __in KIDomNode::SPtr Source,
                    __in LPCWSTR SourceChild,
                    __in KIMutableDomNode::SPtr Destination,
                    __in LPCWSTR DestinationChild   
                    );
                
                //
                // This will initialize the response headers based on
                // the values passed in the request headers.
                //
                NTSTATUS InitializeResponseHeaders(
                    );
                
                
            //
            // Member variables
            //
            private:
                enum FSMState { Unassigned,       // Context is uninitialized
                                Initial,          // Context has been initialized
                                Dispatch,         // Dispatch operation to handler
                                SendResponse,     // Send Response
                                Completed, CompletedWithError = -1 };
                
                FSMState _State;
                
            public:
                //
                // Pointer back to KManagementService
                //
                KManagementService& _Kms;
                
                //
                // this is the remainder of the URI string after it has
                // been routed to this provider. It is passed as a
                // parameter to the Start... callbacks
                //
                KDynString _Target;
                
                //
                // This is the object type as specified in the request
                // message body. 
                //
                KDynString _ObjectType;

                //
                // This is the object URI as specified in the request
                // message header. 
                //
                KDynString _ObjectUri;

                //
                // this is the verb as implied by the specific call to
                // Start... callback. It originates from the verb URI
                // specified in the message header
                //
                Verbs _Verb;

                //
                // If the verb is invoke then there is a sub-verb which
                // details exactly what action is to be performed.
                //
                KDynString _InvokeVerb;
                
                //
                // These are the complete DOMs for the request and
                // response headers and bodies.
                //
                KIDomNode::SPtr _RequestHeaders;
                KIDomNode::SPtr _RequestBody;
                KIMutableDomNode::SPtr& _ResponseHeaders;
                KIMutableDomNode::SPtr& _ResponseBody;

                KIMutableDomNode::SPtr _ResponseMessageHeader;

                //
                // Public accessors
                //
            public:
                KIDomNode::SPtr GetRequestHeaders() { return(_RequestHeaders); };
                KIDomNode::SPtr GetRequestBody() { return(_RequestBody); };
                KIMutableDomNode::SPtr GetResponseHeaders() { return(_ResponseHeaders); };
                KIMutableDomNode::SPtr GetResponseBody() { return(_ResponseBody); };

                KIMutableDomNode::SPtr GetResponseMessageHeader() { return(_ResponseMessageHeader); };
                
        };

        //
        // Default verb handlers will set the error to not implemented
        // and complete the request. It is expected that the derived
        // class will implement those verbs that are supported.
        //
        protected:

            virtual NTSTATUS CreateManagementRequestContext(
                __in Verbs Verb,
                __in KIDomNode::SPtr RequestHeaders,                                       
                __in KIDomNode::SPtr RequestBody,
                __in KIMutableDomNode::SPtr& ResponseHeaders,
                __in KIMutableDomNode::SPtr& ResponseBody,
                __out ManagementRequestContext::SPtr& Mrc,
                __in KAllocator& Allocator,
                __in ULONG AllocationTag = TAG_KMANAGEMENT_SERVICE
                );
            
            virtual VOID CreateHandler(
                __in ManagementRequestContext* Mrc
                )
            {
                Mrc->CompleteRequest(STATUS_SUCCESS,
                                     STATUS_NOT_IMPLEMENTED);                
            }

            virtual VOID DeleteHandler(
                __in ManagementRequestContext* Mrc
                )
            {
                Mrc->CompleteRequest(STATUS_SUCCESS,
                                     STATUS_NOT_IMPLEMENTED);                
            }

            virtual VOID UpdateHandler(
                __in ManagementRequestContext* Mrc
                )
            {
                Mrc->CompleteRequest(STATUS_SUCCESS,
                                     STATUS_NOT_IMPLEMENTED);                
            }

            virtual VOID QueryHandler(
                __in ManagementRequestContext* Mrc
                )
            {
                Mrc->CompleteRequest(STATUS_SUCCESS,
                                     STATUS_NOT_IMPLEMENTED);                
            }

            virtual VOID GetHandler(
                __in ManagementRequestContext* Mrc
                )
            {
                Mrc->CompleteRequest(STATUS_SUCCESS,
                                     STATUS_NOT_IMPLEMENTED);                
            }

            virtual VOID InvokeHandler(
                __in ManagementRequestContext* Mrc
                )
            {
                Mrc->CompleteRequest(STATUS_SUCCESS,
                                     STATUS_NOT_IMPLEMENTED);                
            }

        public:
            //
            // This will find a specific DOM node based upon the
            // path passed
            //
            // CONSIDER: these really belongs in KDom.h
            static
            NTSTATUS FindDOMNode(
                __in KIDomNode& Source,
                __in KStringView& NodePath,
                __in KAllocator& Allocator,
                __out KIDomNode::SPtr& Node
                );

            //
            // This will get the value of a child node and validate its
            // type
            //
            static
            NTSTATUS GetChildValueAndValidateType(
                __in KIDomNode::SPtr Node,
                __in KIDomNode::QName& ChildNodeName,
                __in KVariant::KVariantType Type,
                __out KVariant& Value,
                __in LONG Index = 0
                );


            struct QueryToken
            {
                LPCWSTR Path;
                KVariant::KVariantType Type;
                KVariant* Value;
                LONG Index;
            };

            static
            NTSTATUS GetValuesAndValidateTypes(
                __in KIDomNode::SPtr Node,
                __in QueryToken* QueryTokens,
                __in ULONG QueryTokenCount,
                __in ULONG& FailedIndex,
                __in KAllocator& Allocator
            );
            

            //
            // This routine will take a XML template and perform string
            // replacements in the template and then optionally create
            // a DOM representing the resulting XML
            //
            //
            // Caller should build an array (not KArray) of
            // ReplaceToken structures that define the placeholder
            // string and the replacement value for that placeholder.
            //
            // Caller should also be sure that the passed XmlTemplate
            // parameter is large enough to hold the entire XML after
            // all replacements; it is not expanded in this routine
            //
            struct ReplaceToken
            {
                LPCWSTR TextPlaceholder;
                KVariant* ReplacementValue;
            };

            static
            NTSTATUS BuildXMLFromTemplate(
                __inout KString::SPtr XmlTemplate,
                __out_opt KIMutableDomNode::SPtr* Dom,
                __in ReplaceToken *Tokens,
                __in ULONG TokenCount,
                __in KAllocator& Allocator
            );
            
        protected:
            VOID
            OnStart(
                );

            VOID
            OnReuse(
                );

            VOID
            OnCancel(
                );
            
        private:

            NTSTATUS StartVerbAsync(
                __in  Verbs Verb,
                __in  KUriView& Target,
                __in  KIDomNode::SPtr RequestHeaders,
                __in  KIDomNode::SPtr RequestBody,
                __out KIMutableDomNode::SPtr& ResponseHeaders,
                __out KIMutableDomNode::SPtr& ResponseBody,
                __in  KAsyncContextBase::CompletionCallback Callback,
                __out_opt KAsyncContextBase::SPtr *ChildAsync,
                __in_opt KAsyncContextBase* Parent,
                __in_opt KAsyncGlobalContext* const GlobalContextOverride
                );
            
            VOID FSMContinue(
                __in NTSTATUS Status
            );

            VOID CompletionRoutine(
                __in_opt KAsyncContextBase* const ParentAsync,
                __in KAsyncContextBase& Async
                );
                        
        //
        // member variables
        //
        private:
            //
            // the underlying KMgmtServer object
            //
            KMgmtServer::SPtr _MgmtServer;

            KDynString _Description;
            KDynString _UrlPrefix;
            KDynString _ProviderUri;

            enum FSMState { Created,        // Create but not activated
                            Registering,    // Registration of this provider is in progress
                            Running,        // Provider is running and servicing request
                            Deactivating,   // Deactivation of the underlying KMgmtServer is in progress
                            Completed, CompletedWithError = -1 };
            FSMState _State;            
};
