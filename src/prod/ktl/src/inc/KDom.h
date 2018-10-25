
/*++

    (c) 2012 by Microsoft Corp. All Rights Reserved.

    KDom.h

    Description:
      KTL in-memory Document Object Model framework definitions - XML info set based

    History:
      richhas          12-Jan-2012         Initial version.

--*/

#pragma once

#include <ktl.h>

//** Forward type defs
class KIMutableDomNode;


//** Common base abstraction for all KDom-based DOM graph node implementations
//
//   This interface provides read-access to an underlying graph node. Allowance is
//   made for runtime type checking for support of the KIMutableDomNode extension
//   to this interface.
//

//**BUG: richhas, xxxxxx, Consider adding overloads for all methods that take a QName in
//                        such that only the name aspect is passed as a string with the
//                        namespace prefix internally defaulting to null.
class KIDomNode : public KObject<KIDomNode>, public KShared<KIDomNode>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KIDomNode);
    friend class KDomPath;

public:
    typedef KSharedPtr<KIDomNode> SPtr;

    //* Fully qualified KIDomNode name (FQN)
    struct QName
    {
        LPCWSTR     Namespace;
        LPCWSTR     Name;

        __inline QName(LPCWSTR Namespace, LPCWSTR Name)
        {
            this->Namespace = Namespace;
            this->Name = Name;
        }

        __inline QName(LPCWSTR Name)
        {
            // Allow for a the defaulted namespace prefix
            this->Namespace = L"";
            this->Name = Name;
        }

        __inline QName()
        {
            this->Name = nullptr;
            this->Namespace = L"";
        }
    };

    //* Determine if a current KIDomNode instance supports the KIMutableDomNode interface
    //
    //  Returns TRUE is the KIMutableDomNode is supported - see ToMutatableForm()
    //
    virtual BOOLEAN
    IsMutable() = 0;

    //* Gain access to the KIMutableDomNode interface if it is supported
    //
    //  Returns an empty SPtr is KIMutableDomNode is not supported
    //
    virtual KSharedPtr<KIMutableDomNode>
    ToMutableForm() = 0;

    //* Return the FQN of a DOM graph node
    virtual QName
    GetName() = 0;

    //* Return the value of a DOM graph node
    virtual KVariant&
    GetValue() = 0;

    //* Indicate if the value for element is a well-known KTL type
    virtual BOOLEAN
    GetValueKtlType() = 0;

    //* Return the value of a specific DOM node attribute's value
    //
    //  Only KVariant of Type_UNICODE_STRING is supported.
    //
    //  Returns: STATUS_OBJECT_NAME_NOT_FOUND is named attribute (AttributeName) does not exist
    //
    virtual NTSTATUS
    GetAttribute(__in const QName& AttributeName, __out KVariant& Value) = 0;

    // Return all of a DOM node's attribute names
    virtual NTSTATUS
    GetAllAttributeNames(__out KArray<QName>& Names) = 0;

    //BUG: richhas, xxxxxx, TODO: Add support for: Get all attr names AND values

    //* Child node access

    //  Return a child DOM graph node
    //
    //  The first child with the name of ChildNodeName is returned if Index is 0.  The last child with the name
	//  of ChildNodeName is returned if Index is -1. Otherwise the commonly named child at the supplied index
	//  is returned (if present).
    //
    //  Parameters: ChildNodeName - FQN of the designed child node
    //              Index - Specific child element within a commonly named set
    //
    //
    //  Returns:    STATUS_OBJECT_NAME_NOT_FOUND if the requested child (ChildNodeName, Index) does not exist.
    //
    //
    virtual NTSTATUS
    GetChild(__in const QName& ChildNodeName, __out KIDomNode::SPtr& Child, __in LONG Index = 0) = 0;


    //* Return a child DOM graph node's value - the first child's value with the name of ChildNodeName is
    //  returned if Index is 0, the last if Index is -1, or the commonly named child's value at the supplied
	//  index is returned (if present).
    //
    virtual NTSTATUS
    GetChildValue(__in const QName& ChildNodeName, __out KVariant& Value, __in LONG Index = 0) = 0;

    //* Return all children graph nodes of a current node.
    virtual NTSTATUS
    GetAllChildren(__out KArray<KIDomNode::SPtr>& Children) = 0;

    //* Return all commonly named children graph nodes of a current node.
    virtual NTSTATUS
    GetChildren(__in const QName& ChildNodeName, __out KArray<KIDomNode::SPtr>& Children) = 0;

    //***BUG: richhas, xxxxxx, TODO: Add: get all names AND values

    //** DOM search support
    //***BUG: richhas; xxxxx; Consider adding support for a form of Select() that takes a predicate function and
    //                        returns an expression graph that could be applied remotely.
    //
    typedef KDelegate<BOOLEAN(
        KIDomNode&,                 // Subject
        void*                       // Predicate context
        )> WherePredicate;

    //* Walk a current dom graph applying a provided WherePredicate; returning a result-set selected by that supplied
    //  predicate.
    //
    //  Returns:    STATUS_NOT_IMPLEMETED
    //
    NTSTATUS
    virtual Select(
        __out KArray<KIDomNode::SPtr>& Results,
        __in WherePredicate Where,
        __in_opt void* Context = nullptr) = 0;

    //* Validate the the Dom has the expected xnlns attribute.
    NTSTATUS
    ValidateNamespaceAttribute(__in_z LPCWSTR Namespace);

    // Path-based functions
    //

    //* Return child node value (node or attribute) via path
    virtual NTSTATUS
    GetValue(__in const KDomPath& Path, __out KVariant& Value) = 0;

    //* Return child node via path
    virtual NTSTATUS
    GetNode(__in const KDomPath& Path, __out KIDomNode::SPtr& Value) = 0;

    //* Return child node via path
    virtual NTSTATUS
    GetNodes(__in const KDomPath& Path, __out KArray<KIDomNode::SPtr>& ResultSet) = 0;

    //* Return the count of nodes at the specified path
    virtual NTSTATUS
    GetCount(__in const KDomPath& Path, __out ULONG& Count) = 0;

    virtual NTSTATUS
    GetPath(__out KString::SPtr& PathToThis) = 0;
};


//**  Interface representing a node within a DOM graph with allowance for mutation
class KIMutableDomNode : public KIDomNode
{
    K_FORCE_SHARED_WITH_INHERITANCE(KIMutableDomNode);

public:
    typedef KSharedPtr<KIMutableDomNode> SPtr;

    //* Change/Set the FQN of a DOM graph node.
    NTSTATUS
    virtual SetName(__in const QName& NodeName) = 0;

    //* Change/Set the value of a DOM graph node.
    NTSTATUS
    virtual SetValue(__in const KVariant& Value) = 0;

    //* Modify element as a well-known KTL type
    NTSTATUS
    virtual SetValueKtlType(__in BOOLEAN IsKtlType) = 0;

    //* Add an unique Attribute on a DOM graph node.
    //
    //  Only KVariant of Type_UNICODE_STRING is supported.
    //
    NTSTATUS
    virtual SetAttribute(__in const QName& AttributeName, __in const KVariant& Value) = 0;

    //* Remove an Attribute on a DOM graph node.
    //
    //  Returns: STATUS_OBJECT_NAME_NOT_FOUND is the named attribute is missing
    //
    NTSTATUS
    virtual DeleteAttribute(__in const QName& AttributeName) = 0;

    //* Create a new child DOM graph node on a current graph node.
    //
    //  If an Index value is provided, the new element is added at that offset within a set of commonly named
    //  graph nodes. If an Index value of -1 (default) is provided, the new child is appended to the such a named
    //  set.
    //
    NTSTATUS
    virtual AddChild(__in const QName& ChildNodeName, __out KIMutableDomNode::SPtr& UpdatableChildAdded, __in LONG Index = -1) = 0;

    //* Remove an existing child DOM graph node from a current graph node.
    //
    //  If an Index value is provided, the Index-ith commonly commonly named graph node is removed. If a value of 0 (default)
    //  is provided, the first such named child graph node is removed from such a set. If a value of -1 is provided, the last
	//  such named child graph node is removed from such a set.
    //
    //  Returns: STATUS_OBJECT_NAME_NOT_FOUND is the indexed child identified is missing from the named set
    //
    NTSTATUS
    virtual DeleteChild(__in const QName& ChildNodeName, __in LONG Index = 0) = 0;

    //* Remove all commonly named child DOM graph nodes from a current graph node.
    NTSTATUS
    virtual DeleteChildren(__in QName& ChildNodeName) = 0;

    //* Add a new child DOM graph node on a current graph node.
    NTSTATUS
    virtual AppendChild(
        __in const QName& ChildNodeName,
        __out KIMutableDomNode::SPtr& UpdatableChildCreated) = 0;

    //* Set the Index-ith commonly named child graph node's value.
	//
	//  If Index is -1, the value from the last commonly named child graph node is set.
    //
    //  Returns: STATUS_OBJECT_NAME_NOT_FOUND is the indexed child identified is missing from the named set
    //
    NTSTATUS
    virtual SetChildValue(__in const QName& ChildNodeName, __in KVariant& Value, __in LONG Index = 0) = 0;


    //* Add a DOM and it descendents (DomToAdd) as a child at the specified location (Index)
    //  in the current DomNode. DomToAdd must be a complete DOM (no parent).
    //
    //  If an Index value is provided, the new element is added at that offset within a set of commonly named
    //  graph nodes. If an Index value of -1 (default) is provided, the new child is appended to such a named
    //  set.
    //
    //  Returns: STATUS_OBJECT_NAME_NOT_FOUND - is the indexed child identified is missing from the named
    //                                          identified by DomToAdd's name.
    //           STATUS_INVALID_PARAMETER_1   - DomToAdd is not reference to "complete" DOM
    //
    NTSTATUS
    virtual AddChildDom(__in KIMutableDomNode& DomToAdd, __in LONG Index = -1) = 0;
};



//** Concrete local DOM factories and helper class
//
//   The KI*DomNode implementations created by these KDom methods can produce and consume
//   XML text (unicode) streams. These text streams (must) carry the unicode BOM marker
//   (0xFEFF) - which is automatically produced by the KDom stream production methods.
//
//   This package does not maintain an XML namespace dictionary nor does it validate
//   namespace usage. To keep the implementation simple, there is a set (one for now)
//   reserved namespace prefix values. Currently:
//
//          xsi       - used to describe reserved xml attributes for this package.
//
//   The reserved set of attribute names are:
//
//          xsi:type  - used to describe an element type within the KDom type-system.
//                      The current types supported are:
//
//              xsi:type="ktl:LONG"
//              xsi:type="ktl:ULONG"
//              xsi:type="ktl:LONGLONG"
//              xsi:type="ktl:ULONGLONG"
//              xsi:type="ktl:GUID"
//              xsi:type="ktl:STRING"
//              xsi:type="ktl:DATETIME"     (usage == xsd:dateTime)
//              xsi:type="ktl:DURATION"     (usage == xsd:duration)
//              xsi:type="ktl:URI"          (usage == xsd:anyURI)
//
//              NOTE: any element type not explicitly decorated by one of these reserved
//                    attributes will default to a string type.
//
//   The format and conversion rules for these supported types is defined by the KStringView
//   class's To* and From* convertion methods. When producing or consuming an XML stream using
//   KDom methods, the ktlns:type attributes are automatically added and removed.
//
//   NOTE: Throughout the KI*DomNode object model, all element values are carried within KVariant
//         instances.
//
//   When adding a new type to the ktlns type-system, one must take the following steps to make
//   that type fully supported:
//
//      1) Enhance KVariant to support the new type - see KVariant::ToStream()
//      2) Enhance KStringView to support conversion To*() and From*() the new type
//      3) Enhance the DomNode::ParserHookBase::AddAttribute() method and related defintions to
//         detect the new type attribute in an XML stream
//      4) Enhance the DomNode::AttributuesToStream() method to produce the new type attribute
//         into an XML stream
//      5) Enhance ktl.xsd
//
class KDom
{
public:
    //* Construct an empty DOM that is modifiable
    NTSTATUS
    static CreateEmpty(__out KIMutableDomNode::SPtr& Result, __in KAllocator& Allocator, __in ULONG Tag);

    //* Construct an modifiable DOM from the contents of a provided input buffer.
    NTSTATUS
    static CreateFromBuffer(
        __in KBuffer::SPtr Source,
        __in KAllocator& Allocator,
        __in ULONG Tag,
        __out KIMutableDomNode::SPtr& Result);

    //* Construct an read-only DOM from the contents of a provided source buffer.
    NTSTATUS
    static CreateReadOnlyFromBuffer(
        __in KBuffer::SPtr Source,
        __in KAllocator& Allocator,
        __in ULONG Tag,
        __out KIDomNode::SPtr& Result);

    //* Clone a source KIDomNode (graph) as a KIMutableDomNode graph
    //
    //      NOTE: All strings in the original (Source) DOM are reused in
    //            the resulting DOM; making the cloned DOMs light weight in terms
    //            of memory usage.
    //
    NTSTATUS
    static CloneAs(__in const KIDomNode::SPtr& Source, __out KIMutableDomNode::SPtr& Result);

    // Convenience overload
    //
    NTSTATUS
    static CloneAs(__in KIMutableDomNode::SPtr& Source, __out KIMutableDomNode::SPtr& Result);


    //* Clone a source KIDomNode (graph) as a (read-only) KIDomNode graph
    //
    //      NOTE: All strings in the original (Source) DOM are reused in
    //            the resulting DOM; making the cloned DOMs light weight in terms
    //            of memory usage.
    //
    NTSTATUS
    static CloneAs(__in KIDomNode::SPtr& Source, __out KIDomNode::SPtr& Result);

    //* Serialize the contents of a provided DOM into a provided output Stream.
    NTSTATUS
    static ToStream(__in const KIDomNode::SPtr& Source, __out KIOutputStream& Result);

    //* Serialize the contents of a provided DOM into a provided output Stream.
    NTSTATUS
    static ToStream(__in KIMutableDomNode::SPtr& Source, __out KIOutputStream& Result);

    //* XML Stream support
    class UnicodeOutputStream;

    NTSTATUS
    static CreateOutputStream(
        __out KSharedPtr<UnicodeOutputStream>& Result,
        __in KAllocator& Allocator,
        __in ULONG Tag,
        __in ULONG InitialStreamSize = 1024);


    // ToString
    //
    // Convenience function
    //
    // Serializes the Dom to a KString (UNICODE) with no BOM marker.
    //
    static NTSTATUS
    ToString(
        __in  KIDomNode::SPtr Source,
        __in  KAllocator& Allocator,
        __out KString::SPtr& String);

    // ToString (overload)
    //
    //
    static NTSTATUS
    ToString(
        __in  KIMutableDomNode::SPtr Source,
        __in  KAllocator& Allocator,
        __out KString::SPtr& String);

    // FromString
    //
    // Converts an XML string to a DOM object.
    // The string should not have a BOM.
    //
    static NTSTATUS
    FromString(
        __in  KStringView& Src,
        __in  KAllocator& Allocator,
        __out KIDomNode::SPtr& Dom);

    // FromString
    // overload
    //
    static NTSTATUS
    FromString(
        __in  KStringView& Src,
        __in  KAllocator& Allocator,
        __out KIMutableDomNode::SPtr& Dom);

private:
    KDom() {}           // Static class
};


//** Abstract unicode output stream class
class KDom::UnicodeOutputStream :
    public KObject<KDom::UnicodeOutputStream>,
    public KShared<KDom::UnicodeOutputStream>,
    public KIOutputStream
{
    K_FORCE_SHARED_WITH_INHERITANCE(UnicodeOutputStream);

public:
    virtual KBuffer::SPtr
    GetBuffer() = 0;

    virtual ULONG
    GetStreamSize() = 0;
};


//*** KDOM Pipe facility
//
//  To support the abstracted transfer of KIDomNode instances between anonymous components
//  within a KtlSystem, the KDomPipe facility is provided.
//
//  KIDomNode instances are written into a KDomPipe via StartWrite(). The completion delegate
//  supplied to StartWrite() will be invoked once the pipe starts the delivery process to the
//  KDomPipe consumer implemented via KDomPipe::DequeueOperation instances; using the KAsyncQueue
//  base class' DequeueOperation's StartDequeue() method. Once the completion function supplied
//  to StartDequeue() is invoked, KDomPipe::DequeueOperation's GetDom() method may be used to extract
//  the dom reference related to that completion. Beyond these minor differences, all other KAsyncQueue
//  behaviors apply. See KAsyncQueue.h
//

class KDomPipe;

//** Private class for transport of DOMs thru a KDomPipe - is completed once
//   Dom is scheduled for dequeue KDomPipe::DequeueOperation completion
//   on the consumer side of a KDomPipe.
//
class KDomPipeQueuedWriteOp : public KAsyncContextBase
{
    K_FORCE_SHARED(KDomPipeQueuedWriteOp);

private:
    friend class KDomPipe;              // NOTE: Class is meant to be completly private to KDomPipe
    using KAsyncContextBase::Start;
    using KAsyncContextBase::Complete;

    void OnStart();

private:
    KListEntry              _QueueEntry;
    KIDomNode::SPtr         _Dom;
};

//** KDomPipe class
class KDomPipe : public KAsyncQueue<KDomPipeQueuedWriteOp, 1>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KDomPipe);

private:
    // Hide some of KAsyncQueue's public interface
    using KAsyncQueue::Create;
    using KAsyncQueue::Enqueue;
    using KAsyncQueue::CancelEnqueue;
    using KAsyncQueue::CreateDequeueOperation;

public:
    //* Factory for creation of KDomPipe uService instances.
    //
    //  Parameters:
    //      Result                          - SPtr that points to an allocated KDomPipe<IType> instance
    //      AllocationTag, Allocator        - Standard memory allocation component and tag
    //
    //  Returns:
    //      STATUS_SUCCESS                  - Context points to an allocated KDomPipe instance
    //      STATUS_INSUFFICIENT_RESOURCES   - Out of memory
    //
    //  NOTE: See KAsyncQueue::Activate() and Deactivate() to control the life cycle of a KDomPipe
    //
    static NTSTATUS
    Create(__out KDomPipe::SPtr& Result, __in KAllocator& Allocator, __in ULONG AllocationTag);

    //* Start the write of a KIDomNode into a KDomPipe
    //
    //  Parameters:
    //      Dom                             - SPtr of the Dom to be queued thru the pipe
    //      ParentAsyncContext, CallbackPtr - Standard KAsync arrangment
    //
    //  Returns:
    //      STATUS_SUCCESS:                 - Dom is queued into the pipe
    //      STATUS_INSUFFICIENT_RESOURCES   - Out of memory
    //
    //  Completion status:
    //      STATUS_SUCCESS:                 - Dom was/is being consumed
    //      K_STATUS_SHUTDOWN_PENDING       - Write did not occur - the pipe has been deactivated
    //
    NTSTATUS
    StartWrite(
        __in KIDomNode& Dom,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt CompletionCallback CallbackPtr);

    //* KDomPipe specific dequeue operations
    class DequeueOperation : public KAsyncQueue::DequeueOperation
    {
        K_FORCE_SHARED(DequeueOperation);

    private:
        // Hide some of the KAsyncQueue::DequeueOperation public API
        using KAsyncQueue::DequeueOperation::GetDequeuedItem;

    public:
        //* See KAsyncQueue::StartDequeue() to understand the dequeue cycle - including the completion
        //  status'
        //
        //  NOTE: At the time the completion callback provided to StartDequeue() is invoked, the related
        //        StartWrite operation has been scheduled for completion.

        __inline KIDomNode::SPtr
        GetDom()
        {
            return _Dom;
        }

    private:
        friend class KDomPipe;
        DequeueOperation(KDomPipe& Owner, ULONG const OwnerVersion);

        void    /* override */
        CompleteDequeue(KDomPipeQueuedWriteOp& ItemDequeued);

        void
        OnReuse();

    private:
        KIDomNode::SPtr             _Dom;
    };

    //* DequeueOperation Factory
    //
    //  Parameters:
    //      Result                          - SPtr that points to the allocated DequeueOperation instance
    //
    //  Returns:
    //      STATUS_SUCCESS                  - Context points to an allocated DequeueOperation instance
    //      STATUS_INSUFFICIENT_RESOURCES   - Out of memory
    //
    //  NOTE: Instances of DequeueOperation created by this factory are bound to the producing KDomPipe
    //        instance AND the specific Activation-version of that KDomPipe and may only be used with
    //        that binding.
    //
    NTSTATUS
    CreateDequeueOperation(__out KDomPipe::DequeueOperation::SPtr& Result);
};

