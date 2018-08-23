/*++

    (c) 2012 by Microsoft Corp. All Rights Reserved.

    KDom.cpp

    Description:
      KTL Document Object Model framework implementation for Windows Fabric

    History:
      richhas          12-Jan-2012         Initial version.


    Notes:
    To add new data types, do the following
    [1] Add a new KXmlTypeName* to the "Supported type names" immediately below.
        These will be the xsi:type attributes to be recognized in the XML itself

    [2] Add the required type to KVariant
        -- Ensure there is a "ToString" and "FromString" technique for that type.

    [3] Add the required type to KIOutputStream (KStreams.h)
        -- Add the implemention to the KVariant::ToStream implementation
        -- Add in UnicodeOutputStreamImp for that type

    [4] To support reading back the value, add an addition recognizer
        -- in DomNode::ParserHookBase::AddAttribute (see the other types)
        -- in DomNode::ParserHookBase::AddContent


--*/

#include <ktl.h>

//** Forward type definitions
class DomNode;
class DomRoot;



//*** Defintions of reserved XML text values
#define KXmlNs L"xsi"
    #define SizeOfKXmlNs 3
#define KXmlTypeName L"type"
    #define SizeOfKXmlTypeName 4

#define KXmlCommonTypeAttr L" " KXmlNs L":" KXmlTypeName L"=\""

#define KtlXmlTypePrefix        L"ktl:"
    #define SizeOfKtlXmlTypePrefix 4

//** Supported type names
#define KXmlTypeNameLONG        L"ktl:LONG"
    #define SizeOfKXmlTypeNameLONG 8

#define KXmlTypeNameULONG       L"ktl:ULONG"
    #define SizeOfKXmlTypeNameULONG 9

#define KXmlTypeNameLONGLONG    L"ktl:LONGLONG"
    #define SizeOfKXmlTypeNameLONGLONG 12

#define KXmlTypeNameULONGLONG   L"ktl:ULONGLONG"
    #define SizeOfKXmlTypeNameULONGLONG 13

#define KXmlTypeNameGUID        L"ktl:GUID"
    #define SizeOfKXmlTypeNameGUID 8

#define KXmlTypeNameSTRING      L"ktl:STRING"
    #define SizeOfKXmlTypeNameSTRING 10

#define KXmlTypeNameURI           L"ktl:URI"
    #define SizeOfKXmlTypeNameURI 7

#define KXmlTypeNameDURATION      L"ktl:DURATION"
    #define SizeOfKXmlTypeNameDURATION 12

#define KXmlTypeNameDATETIME      L"ktl:DATETIME"
    #define SizeOfKXmlTypeNameDATETIME 12

#define KXmlTypeNameBOOLEAN      L"ktl:BOOLEAN"
    #define SizeOfKXmlTypeNameBOOLEAN 11



//** Supported type attribute strings
#define KXmlTypeAttrLONG (KXmlCommonTypeAttr KXmlTypeNameLONG L"\"")
#define KXmlTypeAttrULONG (KXmlCommonTypeAttr KXmlTypeNameULONG L"\"")
#define KXmlTypeAttrLONGLONG (KXmlCommonTypeAttr KXmlTypeNameLONGLONG L"\"")
#define KXmlTypeAttrULONGLONG (KXmlCommonTypeAttr KXmlTypeNameULONGLONG L"\"")
#define KXmlTypeAttrGUID (KXmlCommonTypeAttr KXmlTypeNameGUID L"\"")
#define KXmlTypeAttrSTRING (KXmlCommonTypeAttr KXmlTypeNameSTRING L"\"")

#define KXmlTypeAttrURI      (KXmlCommonTypeAttr KXmlTypeNameURI L"\"")
#define KXmlTypeAttrDURATION (KXmlCommonTypeAttr KXmlTypeNameDURATION L"\"")
#define KXmlTypeAttrDATETIME (KXmlCommonTypeAttr KXmlTypeNameDATETIME L"\"")
#define KXmlTypeAttrBOOLEAN  (KXmlCommonTypeAttr KXmlTypeNameBOOLEAN L"\"")




//*** Unicode in-memory output stream class
class UnicodeOutputStreamImp : public KDom::UnicodeOutputStream
{
    K_FORCE_SHARED(UnicodeOutputStreamImp);

public:
    NTSTATUS
    static Create(
        __out KDom::UnicodeOutputStream::SPtr& Result,
        __in KAllocator& Allocator,
        __in ULONG Tag,
        __in ULONG InitialSize);

private:        // KIOutoutStream
    NTSTATUS
    Put(__in WCHAR Value) override;

    NTSTATUS
    Put(__in LONG Value) override;

    NTSTATUS
    Put(__in ULONG Value) override;

    NTSTATUS
    Put(__in LONGLONG Value) override;

    NTSTATUS
    Put(__in ULONGLONG Value) override;

    NTSTATUS
    Put(__in GUID& Value) override;

    NTSTATUS
    Put(__in_z const WCHAR* Value) override;

    virtual
    NTSTATUS
    Put(__in KUri::SPtr Value) override;

    NTSTATUS
    Put(__in KString::SPtr Value) override;

    NTSTATUS
    Put(__in KDateTime Value) override;

    NTSTATUS
    Put(__in KDuration Value) override;

    NTSTATUS
    Put(__in BOOLEAN Value) override;

    KBuffer::SPtr
    GetBuffer() override;

    ULONG
    GetStreamSize() override;

private:
    UnicodeOutputStreamImp(ULONG InitialSize);

    __inline NTSTATUS
    GuaranteeSpace(ULONG WCSizeNeeded);

    __inline void*
    NextBufferAddr();

    __inline NTSTATUS
    AppendToBuffer(__in_ecount(WCSize) WCHAR* Source, __in ULONG WCSize);

private:
    KBuffer::SPtr       _Buffer;
    ULONG               _NextOffset;
};


//*** Utility shared string class used to limit duplicate strings within a DOM
class SharedWString : public KShared<SharedWString>
{
    K_FORCE_SHARED(SharedWString);

public:
    NTSTATUS
    static Create(
        __out SharedWString::SPtr& Result,
        __in_z const WCHAR* String,
        __in KAllocator& Allocator);

    NTSTATUS
    static Create(
        __out SharedWString::SPtr& Result,
        __in_ecount_z(StringSize) const WCHAR* String,
        __in ULONG StringSize,
        __in KAllocator& Allocator);

    __inline LPCWSTR
    GetValue()
    {
        return &_Data[0];
    }

    __inline USHORT
    GetSize()
    {
        return _Size;
    }

    __inline BOOLEAN
    IsEqual(LPCWSTR To)
    {
        if (To == &_Data[0])
        {
            return TRUE;
        }

        if (To == nullptr)
        {
            return _Size == 0;
        }

        return wcscmp(To, &_Data[0]) == 0;
    }

    __inline BOOLEAN
    IsEqual(SharedWString& To)
    {
        if (_Size == To._Size)
        {
            if (_Size == 0)
            {
                return TRUE;
            }
            return memcmp(&_Data[0], &To._Data[0], _Size * sizeof(WCHAR)) == 0;
        }

        return FALSE;
    }

private:
    USHORT          _Size;
    WCHAR           _Data[1];
};

//*** Class used to key SharedWString dictionaries
class SharedWStringKey
{
public:
    SharedWStringKey();

    SharedWStringKey(__in_z WCHAR* Init);

    SharedWStringKey(__in WCHAR* Init, ULONG Size);

    SharedWStringKey(SharedWString& SharedString);

    BOOLEAN
    operator== (const SharedWStringKey& Other) const;

    __inline SharedWStringKey&
    operator= (SharedWStringKey* Src)
    {
        return this->operator=(*Src);
    }

    __inline SharedWStringKey&
    operator= (const SharedWStringKey& Src)
    {
        _String = Src._String;
        _Size = Src._Size;
        return *this;
    }

    ULONG
    static HashFunc(const SharedWStringKey& Key);

    __inline WCHAR*
    GetString()
    {
        return _String;
    }

    __inline ULONG
    GetStringSize()
    {
        return _Size;
    }

private:
    WCHAR*          _String;
    USHORT          _Size;
};


//*** DOM API Wrapper class
class DomNodeApi : public KIMutableDomNode, public KWeakRefType<DomNodeApi>
{
    K_FORCE_SHARED(DomNodeApi);

public:     // KIMutableDomNode and KIDomNode override
    BOOLEAN
    IsMutable() override;

    KSharedPtr<KIMutableDomNode>
    ToMutableForm() override;

    QName
    GetName() override;

    KVariant&
    GetValue() override;

    BOOLEAN
    GetValueKtlType() override;

    NTSTATUS
    GetAttribute(__in const QName& AttributeName, __out KVariant& Value) override;

    NTSTATUS
    GetAllAttributeNames(__out KArray<QName>& Names) override;

    NTSTATUS
    GetChild(__in const QName& ChildNodeName, __out KIDomNode::SPtr& Child, __in LONG Index) override;

    NTSTATUS
    GetChildValue(__in const QName& ChildNodeName, __out KVariant& Value, __in LONG Index) override;

    NTSTATUS
    GetAllChildren(__out KArray<KIDomNode::SPtr>& Children) override;

    NTSTATUS
    GetChildren(__in const QName& ChildNodeName, __out KArray<KIDomNode::SPtr>& Children) override;

    NTSTATUS
    Select(
        __out KArray<KIDomNode::SPtr>& Results,
        __in WherePredicate Where,
        __in_opt void* Context ) override;

    NTSTATUS
    SetName(__in const QName& NodeName) override;

    NTSTATUS
    SetValue(__in const KVariant& Value) override;

    NTSTATUS
    SetValueKtlType(__in BOOLEAN IsKtlType) override;

    NTSTATUS
    SetAttribute(__in const QName& AttributeName, __in const KVariant& Value) override;

    NTSTATUS
    DeleteAttribute(__in const QName& AttributeName) override;

    NTSTATUS
    AddChild(__in const QName& ChildNodeName, __out KIMutableDomNode::SPtr& UpdatableChildAdded, __in LONG Index) override;

    NTSTATUS
    DeleteChild(__in const QName& ChildNodeName, __in LONG Index) override;

    NTSTATUS
    DeleteChildren(__in QName& ChildNodeName) override;

    NTSTATUS
    AppendChild(__in const QName& ChildNodeName, __out KIMutableDomNode::SPtr& UpdatableChildCreated) override;

    NTSTATUS
    SetChildValue(__in const QName& ChildNodeName, __in KVariant& Value, __in LONG Index) override;

    NTSTATUS
    AddChildDom(__in KIMutableDomNode& DomToAdd, __in LONG Index = -1) override;

    NTSTATUS
    GetValue(__in const KDomPath& Path, __out KVariant& Value);

    NTSTATUS
    GetNode(__in const KDomPath& Path, __out KIDomNode::SPtr& Value);

    NTSTATUS
    GetNodes(__in const KDomPath& Path, __out KArray<KIDomNode::SPtr>& ResultSet);

    NTSTATUS
    GetCount(__in const KDomPath& Path, __out ULONG& Count);

    NTSTATUS
    GetPath(__out KString::SPtr& PathToThis);

public:
    NTSTATUS
    static Create(DomNode& GraphRoot, DomNode& BackingNode, DomNodeApi::SPtr& Result, KAllocator& Allocator);

    __inline DomNode*
    GetBackingNode()
    {
        return _BackingNode;
    }

    __inline void
    ClearBackingNode()
    {
        _BackingNode = nullptr;
        _Graph = nullptr;
    }

    __inline void
    SetBackingState(DomNode& GraphRoot, DomNode& BackingNode)
    {
        _Graph = &GraphRoot;
        _BackingNode = &BackingNode;
    }

private:
    DomNodeApi(DomNode& GraphRoot, DomNode& BackingNode);

private:
    KSharedPtr<DomNode>     _Graph;              // This is what keeps the underlying DomNode-based graph in memory
    DomNode*                _BackingNode;
    KVariant                _NullValue;
};


//*** DOM graph class definitions

//*** Common NamedDomValue base class for all attribute and graph node types
class NamedDomValue
{
public:
    // Fully Qualaified Name for graph nodes and attributes
    class FQN
    {
    public:
        _inline FQN()
        {
        }

        __inline FQN(FQN& From)
        {
            _Namespace = From._Namespace;
            _Name = From._Name;
        }

        __inline BOOLEAN
        IsEqual(FQN& To)
        {
            return (_Namespace->IsEqual(*(To._Namespace))) && (_Name->IsEqual(*(To._Name)));
        }

        __inline BOOLEAN
        IsEqual(const KIDomNode::QName& To)
        {
            return (_Namespace->IsEqual(To.Namespace) && (_Name->IsEqual(To.Name)));
        }

        __inline FQN&
        operator=(FQN& From)
        {
            _Namespace = From._Namespace;
            _Name = From._Name;

            return *this;
        }

        NTSTATUS
        ToStream(KIOutputStream& Output);

    public:
        SharedWString::SPtr    _Namespace;
        SharedWString::SPtr    _Name;
    };

    __inline FQN&
    GetName()
    {
        return _FQN;
    }

    __inline void
    SetName(FQN& Fqn)
    {
        _FQN = Fqn;
    }

    __inline KVariant&
    GetValue()
    {
        return _Value;
    }

    __inline void
    SetValue(const KVariant& Value)
    {
        _Value = Value;
    }

    __inline
    NamedDomValue(FQN& Fqn, const KVariant& Value)
        :   _FQN(Fqn),
            _Value(Value)
    {
    }

protected:
    __inline
    NamedDomValue()
    {
    }

private:
    FQN                 _FQN;
    KVariant            _Value;
};


//*** Dom node attribute class
class DomElementAttribute : public KShared<DomElementAttribute>, public NamedDomValue
{
    K_FORCE_SHARED(DomElementAttribute);

public:
    DomElementAttribute(FQN& Fqn, const KVariant& Value);

    __inline LONG
    static GetSiblingListOffset()
    {
        return FIELD_OFFSET(DomElementAttribute, _SiblingListEntry);
    }

private:
    KListEntry      _SiblingListEntry;
};


//*** DOM graph node class
class DomNode : public KObject<DomNode>, public KShared<DomNode>, public NamedDomValue
{
    friend class DomRoot;
    friend class DomNodeApi;
    K_FORCE_SHARED_WITH_INHERITANCE(DomNode);

protected:
    class ParserHookBase;

    DomNode(BOOLEAN IsMutable);
    DomNode(BOOLEAN IsMutable, FQN& Fqn, DomNode::SPtr Parent);
    DomNode(DomNode&& From, DomNode& Parent);

    NTSTATUS
    static Create(BOOLEAN IsMutable, FQN& Fqn, DomNode& Parent, DomNode::SPtr& Result, KAllocator& Allocator);

    NTSTATUS
    CreateFromBuffer(__in KBuffer::SPtr& Source, __in DomNode::ParserHookBase& ParserHook);

    virtual BOOLEAN
    IsDomRoot()
    {
        return FALSE;
    }

public:
    NTSTATUS
    ToStream(__in KIOutputStream& Output);

    NTSTATUS
    CloneAs(BOOLEAN Mutable, DomNode::SPtr& Result);

    NTSTATUS
    GetApiObject(KIDomNode::SPtr& Result);

    NTSTATUS
    GetApiObject(KIMutableDomNode::SPtr& Result);

    typedef KDelegate<NTSTATUS(
        BOOLEAN,        // IsElementOpenNotClose
        DomNode&,       // Subject
        ULONG           // Graph depth
    )> GraphWalker;

    NTSTATUS
    WalkGraph(GraphWalker Walker);

public:     // KIDomNode
    __inline BOOLEAN
    IsMutable()
    {
        return _IsMutable;
    }

    KIDomNode::QName
    GetName();

    KVariant&
    GetValue();

    BOOLEAN
    GetValueKtlType();

    NTSTATUS
    GetAttribute(__in const KIDomNode::QName& AttributeName, __out KVariant& Value);

    NTSTATUS
    GetAllAttributeNames(__out KArray<KIDomNode::QName>& Names);

    NTSTATUS
    GetChild(__in const KIDomNode::QName& ChildNodeName, __out KIDomNode::SPtr& Child, __in LONG Index);

    NTSTATUS
    GetChildValue(__in const KIDomNode::QName& ChildNodeName, __out KVariant& Value, __in LONG Index);

    NTSTATUS
    GetAllChildren(__out KArray<KIDomNode::SPtr>& Children);

    NTSTATUS
    GetChildren(__in const KIDomNode::QName& ChildNodeName, __out KArray<KIDomNode::SPtr>& Children);

    NTSTATUS
    Select(__out KArray<KIDomNode::SPtr>& Results, __in KIDomNode::WherePredicate Where, __in_opt VOID* Context);

    NTSTATUS
    GetValue(__in KDomPath& Path, __out KVariant& Value);

    NTSTATUS
    GetNode(__in KDomPath& Path, __out KIDomNode::SPtr& Value);

    NTSTATUS
    GetNodes(__in KDomPath& Path, __out KArray<KIDomNode::SPtr>& ResultSet);

    NTSTATUS
    GetCount(__in KDomPath& Path, __out ULONG& Count);

    NTSTATUS
    GetPath(__out KString::SPtr& PathToThis);

public:
    //      KIMutableDomNode
    NTSTATUS
    SetName(__in const KIDomNode::QName& NodeName);

    NTSTATUS
    SetValue(__in const KVariant& Value);

    NTSTATUS
    SetValueKtlType(BOOLEAN IsKtlType);

    NTSTATUS
    SetAttribute(__in const KIDomNode::QName& AttributeName, __in const KVariant& Value);

    NTSTATUS
    DeleteAttribute(__in const KIDomNode::QName& AttributeName);

    NTSTATUS
    AddChild(__in const KIDomNode::QName& ChildNodeName, __out KIMutableDomNode::SPtr& UpdatableChildAdded, __in LONG Index);

    NTSTATUS
    DeleteChild(__in const KIDomNode::QName& ChildNodeName, __in LONG Index);

    NTSTATUS
    DeleteChildren(__in KIDomNode::QName& ChildNodeName);

    NTSTATUS
    AppendChild(
        __in const KIDomNode::QName& ChildNodeName,
        __out KIMutableDomNode::SPtr& UpdatableChildCreated);

    NTSTATUS
    SetChildValue(__in const KIDomNode::QName& ChildNodeName, __in KVariant& Value, __in LONG Index);

    NTSTATUS
    AddChildDom(__in DomNode& DomToAdd, __in LONG Index);

protected:
    NTSTATUS
    FindChild(
        __in const KIDomNode::QName& ChildNodeName,
        __out DomNode::SPtr& Child,
        __in LONG Index);

    NTSTATUS
    FindChildCount(
        __in  KIDomNode::QName& ChildNodeName,
        __out ULONG& Count);

    NTSTATUS
    FindIndexOfChild(
        __in  DomNode::SPtr Target,
        __out ULONG& Index
        );

    NTSTATUS
    AddChild(__in DomNode& NewChild, __in LONG AtOrBefore);

    NTSTATUS
    RemoveChild(__in const KIDomNode::QName& ChildNodeName, __in LONG At = -1);

    DomElementAttribute::SPtr
    FindAttribute(__in const KIDomNode::QName& AttributeName);

    virtual NTSTATUS
    GetOrCreateQName(__in const KIDomNode::QName& QName, __out FQN& Fqn);

    NTSTATUS
    AddAttribute(DomElementAttribute::SPtr NewAttr);

    NTSTATUS
    InternalSetAttribute(__in const KIDomNode::QName& AttributeName, __in const KVariant& Value);

    NTSTATUS
    RemoveAttribute(__in const KIDomNode::QName& QName);

    NTSTATUS
    AttributuesToStream(__in KIOutputStream& Output);

    NTSTATUS
    GetApiObject(DomNodeApi::SPtr& Result);

    NTSTATUS
    CommonAddChild(__in const KIDomNode::QName& ChildNodeName, __out KIMutableDomNode::SPtr& UpdatableChildAdded, __in LONG Index = -1);

private:
    BOOLEAN const                   _IsMutable;
#if !defined(PLATFORM_UNIX)
    DomNode* const                  _Parent;
#else
    DomNode*                        _Parent;
#endif
    KArray<DomNode::SPtr>           _Children;
    KNodeList<DomElementAttribute>  _Attributes;
    KWeakRef<DomNodeApi>::SPtr      _ApiObj;
    BOOLEAN                         _IsValueDefaulted;

protected:
    __inline DomNode::SPtr
    GetParent()
    {
        return _Parent;
    }

    __inline DomRoot&
    GetRoot()
    {
        DomNode*       currentLevel = this;
        while (currentLevel->_Parent != nullptr)
        {
            currentLevel = currentLevel->_Parent;
        }

        return *((DomRoot*)currentLevel);
    }
};


//*** Graph root node class
class DomRoot : public DomNode
{
    K_FORCE_SHARED(DomRoot);

public:
    NTSTATUS
    static Create(BOOLEAN IsMutable, DomRoot::SPtr& Result, KAllocator& Allocator, ULONG Tag);

    NTSTATUS
    CreateFromBuffer(__in KBuffer::SPtr& Source);

    NTSTATUS
    GetOrCreateQName(
        __in WCHAR* Namespace,
        __in ULONG NamespaceSize,
        __in WCHAR* Name,
        __in ULONG NameSize,
        __out NamedDomValue::FQN& Fqn);

    NTSTATUS
    GetOrCreateQName(__in const KIDomNode::QName& QName, __out NamedDomValue::FQN& Fqn);

    BOOLEAN
    IsDomRoot()
    {
        return TRUE;
    }

    __inline KSpinLock& GetGraphLock() { return _GraphLock; }

private:
    DomRoot(BOOLEAN IsMutable);

    NTSTATUS
    GetOrCreateQName(
        __in SharedWStringKey& NamespaceKey,
        __in SharedWStringKey& NameKey,
        __out NamedDomValue::FQN& Fqn);

private:
    KHashTable<SharedWStringKey, SharedWString::SPtr>   _DomStrings;
    KSpinLock                                           _GraphLock;
};


//*** KXmlParser extension class - generates KDom* components as parsing progresses
class DomNode::ParserHookBase : public KXmlParser::IHook
{
    K_DENY_COPY(ParserHookBase);

public:
    typedef KDelegate<DomNode::SPtr(
        DomRoot&,              // Root
        DomNode*,              // Parent
        NamedDomValue::FQN&,    // FQN
        KAllocator&             // Allocator
    )> ElementFactory;

    ParserHookBase(KAllocator& Allocator, DomRoot& Root, ElementFactory Factory);
    ~ParserHookBase();

private:
    KAllocator&             _Allocator;
    DomRoot&                _DomRoot;
    DomNode::SPtr           _CurrentNode;
    KVariant::KVariantType  _CurrentNodeType;
    BOOLEAN                 _CurrentNodeTypeDefaulted;
    ElementFactory          _ElementFactory;

private:    // IHook implementation
    NTSTATUS
    OpenElement(
        __in_ecount(ElementNsLength) WCHAR* ElementNs, 
        __in ULONG  ElementNsLength, 
        __in_ecount(ElementNameLength) WCHAR* ElementName, 
        __in ULONG  ElementNameLength, 
        __in_z const WCHAR* StartElementSection);

    NTSTATUS
    AddAttribute(
        __in BOOLEAN HeaderAttributes,    // Set to true if these are the <?xml header attributes
        __in_ecount(AttributeNsLength) WCHAR* AttributeNs,
        __in ULONG  AttributeNsLength,
        __in_ecount(AttributeNameLength) WCHAR* AttributeName,
        __in ULONG  AttributeNameLength,
        __in_ecount(ValueLength) WCHAR* Value,
        __in ULONG  ValueLength);

    NTSTATUS
    CloseAllAttributes();

    NTSTATUS AddContent(
        __in ULONG  ContentIndex,
        __in ULONG  ContentType,
        __in_ecount(ContentLength) WCHAR* ContentText,
        __in ULONG  ContentLength);

    NTSTATUS
    CloseElement(__in ULONG ElementSectionLength);

    NTSTATUS
    Done(__in BOOLEAN Error);

    BOOLEAN
    ElementWhitespacePreserved();
};




//*** SharedWString implmentation
SharedWString::SharedWString()
    :   _Size(0)
{
}

NTSTATUS
SharedWString::Create(__out SharedWString::SPtr& Result, __in_z const WCHAR* String, __in KAllocator& Allocator)
{
    ULONG       chars = 0;

    // BUG: RLH: Not Kernel friendly - possible overflow
    KInvariant(SizeTToULong(wcslen(String), &chars) == 0);
    KInvariant(chars <= MAXUSHORT);

    ULONG       size = (ULONG)((chars + 1) * sizeof(WCHAR));
    HRESULT hr;
    ULONG result;
    hr = ULongAdd(size, sizeof(SharedWString), &result);
    KInvariant(SUCCEEDED(hr));
    void*       space = _new(KTL_TAG_DOM, Allocator) UCHAR[result];
    
    if (space == nullptr)
    {
        Result = nullptr;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Result = new(space) SharedWString();

    KMemCpySafe(&Result->_Data[0], size, String, size);
    KAssert(Result->_Data[chars] == 0);

    Result->_Size = (USHORT)chars;
    return STATUS_SUCCESS;
}

NTSTATUS
SharedWString::Create(
    __out SharedWString::SPtr& Result,
    __in_ecount_z(StringSize) const WCHAR* String,
    __in ULONG StringSize,
    __in KAllocator& Allocator)
{
    KInvariant(StringSize <= MAXUSHORT);
    ULONG       size = (StringSize + 1) * sizeof(WCHAR);

    HRESULT hr;
    ULONG result;
    hr = ULongAdd(size, sizeof(SharedWString), &result);
    KInvariant(SUCCEEDED(hr));
    void*       space = _new(KTL_TAG_DOM, Allocator) UCHAR[result];


    if (space == nullptr)
    {
        Result = nullptr;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Result = new(space) SharedWString();

    KMemCpySafe(&Result->_Data[0], size, String, size);
    Result->_Data[StringSize] = 0;

    Result->_Size = (USHORT)StringSize;
    return STATUS_SUCCESS;
}

SharedWString::~SharedWString()
{
}


//*** SharedWStringKey implementation
SharedWStringKey::SharedWStringKey()
{
}

SharedWStringKey::SharedWStringKey(__in_z WCHAR* Init)
{
    _String = Init;
    _Size = (USHORT)wcslen(Init);
}

SharedWStringKey::SharedWStringKey(__in WCHAR* Init, ULONG Size)
{
    KFatal(Size <= MAXUSHORT);
    _String = Init;
    _Size = (USHORT)Size;
}

SharedWStringKey::SharedWStringKey(SharedWString& SharedString)
{
    _String = (WCHAR*)(SharedString.GetValue());
    _Size = SharedString.GetSize();
}

BOOLEAN
SharedWStringKey::operator== (const SharedWStringKey& Other) const
{
    if (_String == Other._String)
    {
        return TRUE;
    }

    if (_Size == Other._Size)
    {
        if (_Size == 0)
        {
            return TRUE;
        }
        return memcmp(_String, Other._String, _Size * sizeof(WCHAR)) == 0;
    }

    return FALSE;
}

ULONG
SharedWStringKey::HashFunc(const SharedWStringKey& Key)
{
    KMemRef     mRef(Key._Size * sizeof(WCHAR), Key._String);
    return K_DefaultHashFunction(mRef);
}


//*** NamedDomValue::FQN implementation

//* Serialize an FQN instance to an KIOutputStream (as text)
NTSTATUS
NamedDomValue::FQN::ToStream(KIOutputStream& Output)
{
    NTSTATUS        status;

    if ((_Namespace != nullptr) && !_Namespace->IsEqual(L""))
    {
        status = Output.Put((WCHAR*)(_Namespace->GetValue()));
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        status = Output.Put(L':');
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }

    return Output.Put((WCHAR*)(_Name->GetValue()));
}


//*** DomElementAttribute implementation
DomElementAttribute::DomElementAttribute(FQN& Fqn, const KVariant& Value)
    :   NamedDomValue(Fqn, Value)
{
}

DomElementAttribute::~DomElementAttribute()
{
}


//*** DomRoot implementation
DomRoot::DomRoot(BOOLEAN IsMutable)
    :   DomNode(IsMutable),
        _DomStrings(251, SharedWStringKey::HashFunc, GetThisAllocator())
{
}

DomRoot::~DomRoot()
{
}

//* Return an FQN for the supplied namespace prefix and name while maintaining a common dictionary (_DomStrings) of
// (read-only) strings - to avoid duplicate string within a DOM.
NTSTATUS
DomRoot::GetOrCreateQName(
    __in SharedWStringKey& NamespaceKey,
    __in SharedWStringKey& NameKey,
    __out NamedDomValue::FQN& Fqn)
{
    NTSTATUS                status = STATUS_SUCCESS;
    SharedWString::SPtr     sSPtr;

    Fqn._Name = nullptr;
    Fqn._Namespace = nullptr;

    status = _DomStrings.Get(NamespaceKey, sSPtr);
    if (!NT_SUCCESS(status))
    {
        if (status != STATUS_NOT_FOUND)
        {
            return status;
        }

        // New namespace prefix string value -- add it
        status = SharedWString::Create(sSPtr, NamespaceKey.GetString(), NamespaceKey.GetStringSize() , GetThisAllocator());
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        status = _DomStrings.Put(NamespaceKey, sSPtr, FALSE);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }
    Fqn._Namespace = Ktl::Move(sSPtr);

    status = _DomStrings.Get(NameKey, sSPtr);
    if (!NT_SUCCESS(status))
    {
        if (status != STATUS_NOT_FOUND)
        {
            return status;
        }

        // New name string value -- add it
        status = SharedWString::Create(sSPtr, NameKey.GetString(), NameKey.GetStringSize(), GetThisAllocator());
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        status = _DomStrings.Put(NameKey, sSPtr, FALSE);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }
    Fqn._Name = Ktl::Move(sSPtr);

    return STATUS_SUCCESS;
}

NTSTATUS
DomRoot::GetOrCreateQName(
    __in WCHAR* Namespace,
    __in ULONG NamespaceSize,
    __in WCHAR* Name,
    __in ULONG NameSize,
    __out NamedDomValue::FQN& Fqn)
{
    WCHAR*              ns = (Namespace == nullptr) ? (WCHAR*)L"" : Namespace;
    SharedWStringKey    namespaceKey(ns, NamespaceSize);
    SharedWStringKey    nameKey(Name, NameSize);

    return GetOrCreateQName(namespaceKey, nameKey, Fqn);
}

NTSTATUS
DomRoot::GetOrCreateQName(
    __in const KIDomNode::QName& QName,
    __out FQN& Fqn)
{
    WCHAR*              ns = (QName.Namespace == nullptr) ? (WCHAR*)L"" : (WCHAR*)&QName.Namespace[0];
    SharedWStringKey    namespaceKey(ns);
    SharedWStringKey    nameKey((WCHAR*)QName.Name);

    return GetOrCreateQName(namespaceKey, nameKey, Fqn);
}

NTSTATUS
DomRoot::Create(BOOLEAN IsMutable, DomRoot::SPtr& Result, KAllocator& Allocator, ULONG Tag)
{
    Result = _new(Tag, Allocator) DomRoot(IsMutable);
    if (Result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS        status = Result->Status();
    if (!NT_SUCCESS(status))
    {
        Result = nullptr;
    }

    return (status);
}

NTSTATUS
DomRoot::CreateFromBuffer(__in KBuffer::SPtr& Source)
{
    struct Local
    {
        DomNode::SPtr
        static ElementFactory(DomRoot& Root, DomNode* Parent, NamedDomValue::FQN& Fqn, KAllocator& Allocator)
        {
            if (Parent == nullptr)
            {
                // ctoring root node - just re-cast the root
                Root.NamedDomValue::SetName(Fqn);
                return &Root;
            }

            NTSTATUS                status = STATUS_SUCCESS;
            DomNode::SPtr           result;

            status = DomNode::Create(Root.IsMutable(), Fqn, *Parent, result, Allocator);
            if (!NT_SUCCESS(status))
            {
                result = nullptr;
            }

            return result.RawPtr();
        }
    };

    ParserHookBase  parserHook(GetThisAllocator(), *this, Local::ElementFactory);

    return __super::CreateFromBuffer(Source, parserHook);
}



//*** DomNode class implementation
DomNode::DomNode()
    :    NamedDomValue(),
        _IsMutable(FALSE),
        _Parent(nullptr),
        _Children(GetThisAllocator()),
        _Attributes(DomElementAttribute::GetSiblingListOffset()),
        _IsValueDefaulted(FALSE)
{
    KFatal(FALSE);      // Never called
}

DomNode::DomNode(BOOLEAN IsMutable)
    :   NamedDomValue(),
        _IsMutable(IsMutable),
        _Parent(nullptr),
        _Children(GetThisAllocator()),
        _Attributes(DomElementAttribute::GetSiblingListOffset()),
        _IsValueDefaulted(FALSE)
{
}

DomNode::DomNode(BOOLEAN IsMutable, FQN& Fqn, DomNode::SPtr Parent)
    :   NamedDomValue(Fqn, KVariant()),
        _IsMutable(IsMutable),
        _Parent(Parent.RawPtr()),
        _Children(GetThisAllocator()),
        _Attributes(DomElementAttribute::GetSiblingListOffset()),
        _IsValueDefaulted(FALSE)
{
}

DomNode::DomNode(DomNode&& From, DomNode& Parent)
    :   NamedDomValue(From.NamedDomValue::GetName(), From.NamedDomValue::GetValue()),
        _Children(Ktl::Move(From._Children)),
        _Attributes(Ktl::Move(From._Attributes)),
        _Parent(&Parent),
        _IsMutable(From._IsMutable),
        _IsValueDefaulted(From._IsValueDefaulted),
        _ApiObj(nullptr)
{
    // If there is an existing API obj for From, steal it and fix up the actual ApiObj to ref
    // "this"
    if (From._ApiObj != nullptr)
    {
        KWeakRef<DomNodeApi>::SPtr  apiObjWRef = Ktl::Move(From._ApiObj);
        DomNodeApi::SPtr            api = apiObjWRef->TryGetTarget();
        if (api != nullptr)
        {
            _ApiObj = Ktl::Move(apiObjWRef);
            api->SetBackingState(Parent.GetRoot(), *this);
        }
    }

    for (ULONG ix = 0; ix < _Children.Count(); ix++)
    {
        // Override const to update existing children's parent
        #pragma warning(disable:4213)   // C4213: nonstandard extension used : cast on l-value
#if !defined(PLATFORM_UNIX)
        const_cast<DomNode*>(_Children[ix]->_Parent) = this;
#else
        _Children[ix]->_Parent = this;
#endif
    }
}

DomNode::~DomNode()
{
    while (_Attributes.Count() > 0)
    {
        _Attributes.RemoveHead()->Release();    // #1: Reverses #1 in AddAttribute()
    }

    DomNodeApi::SPtr  apiObj;
    if (_ApiObj != nullptr)
    {
        apiObj = _ApiObj->TryGetTarget();
    }

    if (apiObj != nullptr)
    {
        apiObj->ClearBackingNode();
    }
}

NTSTATUS
DomNode::Create(BOOLEAN IsMutable, FQN& Fqn, DomNode& Parent, DomNode::SPtr& Result, KAllocator& Allocator)
{
    Result = _new(KTL_TAG_DOM, Allocator) DomNode(IsMutable, Fqn, &Parent);
    if (Result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS        status = Result->Status();
    if (!NT_SUCCESS(status))
    {
        Result = nullptr;
    }

    return status;
}

//* Deserialize a read-only DOM graph node from the contents of a supplied source KBuffer
NTSTATUS
DomNode::CreateFromBuffer(__in KBuffer::SPtr& Source, __in DomNode::ParserHookBase& ParserHook)
{
     if (Source->QuerySize() < sizeof(WCHAR))
     {
         return STATUS_BUFFER_TOO_SMALL;
     }

     if (*(WCHAR*)(Source->GetBuffer()) != KTextFile::UnicodeBom)
     {
         // Not flagged as a UNICODE stream
         return STATUS_OBJECT_TYPE_MISMATCH;
     }

     KXmlParser::SPtr       parser;
     NTSTATUS               status = KXmlParser::Create(parser, GetThisAllocator(), KTL_TAG_DOM);
     if (!NT_SUCCESS(status))
     {
         return status;
     }

     return parser->Parse(((WCHAR*)(Source->GetBuffer())) + 1, Source->QuerySize() - sizeof(WCHAR), ParserHook);
}

//* Depth first graph enumerator
NTSTATUS
DomNode::WalkGraph(GraphWalker Walker)
{
    // Stack imp - avoids recursive call-stack usage
    class StackFrame
    {
    public:
        DomNode&        _Node;
        ULONG           _ChildIx;
        BOOLEAN         _IsDomRootNode;

    public:
        StackFrame(DomNode& Node)
            :   _Node(Node),
                _ChildIx(0),
                _IsDomRootNode(Node.IsDomRoot())
        {
        }

    private:
        StackFrame()                                
            :   _Node(*((DomNode*)nullptr))
        { 
            KInvariant(FALSE); 
        }

        StackFrame(StackFrame const&)               
            :   _Node(*((DomNode*)nullptr))
        { 
            KInvariant(FALSE); 
        }
        
        StackFrame& operator = (StackFrame const& Other)    
        { 
            KInvariant(FALSE); return const_cast<StackFrame&>(Other); 
        }
    };

    // Allocate local stack
    // BUG: RLH: possible overflow
    StackFrame *const   stack = (StackFrame*)_new(KTL_TAG_DOM, GetThisAllocator())
                                                 UCHAR[sizeof(StackFrame) * KXmlParser::MaxNestedElements];
                        KFinally([stack]() {_delete(stack); });

    if (stack == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    StackFrame*         csf = stack + (KXmlParser::MaxNestedElements - 1);  // current stack frame pointer
    ULONG               stackDepth = 0;                                     // Used for indentation generation
    NTSTATUS            status;

    new(csf) StackFrame(*this);         // Init TOS with this DomNode

    ProcessCurrentNode:
    {
        status = Walker(TRUE, csf->_Node, stackDepth);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        BOOLEAN     hasChildren = csf->_Node._Children.Count() > 0;
        BOOLEAN     hasValue = csf->_Node.NamedDomValue::GetValue().Type() != KVariant::KVariantType::Type_EMPTY;

        if (!hasChildren && hasValue)
        {
            goto NodeComplete;
        }

        StackFrame*       tempCsf;
        if (hasChildren)
        {
            // Walk children - depth-first
            csf->_ChildIx = 0;
            while (csf->_ChildIx < csf->_Node._Children.Count())
            {
                // Create new stack frame for next child
                tempCsf = csf;
                csf--;
                stackDepth++;

                KAssert(csf > stack);
                if (csf == stack)
                {
                    return K_STATUS_XML_PARSER_LIMIT;           // Stack overflow
                }

                new(csf) StackFrame(*(tempCsf->_Node._Children[tempCsf->_ChildIx].RawPtr()));
                goto ProcessCurrentNode;

    ContinueWithCurrentNode:
                csf->_ChildIx++;
            }
        }

    NodeComplete:
        status = Walker(FALSE, csf->_Node, stackDepth);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        csf++;
        if (csf != (stack + (KXmlParser::MaxNestedElements)))
        {
            KAssert(stackDepth > 0);
            stackDepth--;
            goto ContinueWithCurrentNode;
        }
    }

    return STATUS_SUCCESS;
}


//* Serialize a DOM graph node into a supplied KIOutputStream - does not recurse on the
//  call stack - kernel friendly
//
NTSTATUS
DomNode::ToStream(__in KIOutputStream& Output)
{
    // Local helper methods
    struct Local
    {
        KIOutputStream*     _Output;

        NTSTATUS
        OutputTabs(ULONG Count)
        {
            NTSTATUS        status = STATUS_SUCCESS;
            for (ULONG ix = 0; ix < Count; ix++)
            {
                status = _Output->Put(L"   ");
                if (!NT_SUCCESS(status))
                {
                    return status;
                }
            }

            return STATUS_SUCCESS;
        }

        NTSTATUS
        OutputElementClosure(NamedDomValue::FQN& Name)
        {
            NTSTATUS status = _Output->Put(L"</");
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            status = Name.ToStream(*_Output);
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            return _Output->Put(L'>');
        }


        NTSTATUS
        Walker(BOOLEAN IsOpenElement, DomNode& Node, ULONG GraphDepth)
        {
            NTSTATUS        status = STATUS_SUCCESS;
            BOOLEAN         hasChildren = Node._Children.Count() > 0;
            BOOLEAN         hasValue = Node.NamedDomValue::GetValue().Type() != KVariant::KVariantType::Type_EMPTY;

            if (IsOpenElement)
            {
                // Put out element indentation and start-of-element for TOS
                status = OutputTabs(GraphDepth);
                if (!NT_SUCCESS(status))
                {
                    return status;
                }

                status = _Output->Put(L'<');
                if (!NT_SUCCESS(status))
                {
                    return status;
                }

                status = Node.NamedDomValue::GetName().ToStream(*_Output);
                if (!NT_SUCCESS(status))
                {
                    return status;
                }

                status = Node.AttributuesToStream(*_Output);
                if (!NT_SUCCESS(status))
                {
                    return status;
                }

                status = _Output->Put(L'>');
                if (!NT_SUCCESS(status))
                {
                    return status;
                }

                if (hasValue)
                {
                    status = Node.NamedDomValue::GetValue().ToStream(*_Output);
                    if (!NT_SUCCESS(status))
                    {
                        return status;
                    }
                }

                if (hasChildren)
                {
                    return _Output->Put(L'\n');
                }
            }
            else
            {
                // Close of element
                if (hasChildren)
                {
                    status = OutputTabs(GraphDepth);
                    if (!NT_SUCCESS(status))
                    {
                        return status;
                    }
                }

                status = OutputElementClosure(Node.NamedDomValue::GetName());
                if (!NT_SUCCESS(status))
                {
                    return status;
                }

                return _Output->Put(L'\n');
            }

            return STATUS_SUCCESS;
        }
    }                       local;

    local._Output = &Output;
    return WalkGraph(GraphWalker(&local, &Local::Walker));
}

//* Clone this graph branch as a Mutable or non-Mutable copy
NTSTATUS
DomNode::CloneAs(BOOLEAN Mutable, DomNode::SPtr& Result)
{
    // Local helper methods
    class Local
    {
    private:
        Local(Local const&)
            :   _Allocator(*(KAllocator*)nullptr)
        {
            KInvariant(FALSE);
        }

        Local const& operator = (Local const& Other)
        {
            KInvariant(FALSE);
            return const_cast<Local const&>(Other);
        }

    public:
        KAllocator&         _Allocator;
        BOOLEAN             _IsMutable;
        DomRoot::SPtr       _Root;
        DomNode::SPtr       _Parent;
        DomNode::SPtr       _LastNode;
        ULONG               _GraphDepth;

        Local(BOOLEAN IsMutable, KAllocator& Allocator)
            :   _Allocator(Allocator),
                _GraphDepth(0),
                _IsMutable(IsMutable)
        {
        }

        NTSTATUS
        Walker(BOOLEAN IsOpenElement, DomNode& Node, ULONG GraphDepth)
        {
            // NOTE: Generally the strings in Node are reused (i.e. no copies) for the clone
            NTSTATUS        status = STATUS_SUCCESS;

            if (IsOpenElement)
            {
                DomNode::SPtr       newNode;

                if (_Root == nullptr)
                {
                    // creating the root
                    status = DomRoot::Create(_IsMutable, _Root, _Allocator, KTL_TAG_DOM);
                    if (!NT_SUCCESS(status))
                    {
                        return status;
                    }

                    newNode = _Root.RawPtr();
                    _Parent = newNode;
                    _GraphDepth = GraphDepth;

                    // Copy name
                    newNode->NamedDomValue::SetName(Node.NamedDomValue::GetName());
                }
                else
                {
                    // push/pop parent logic
                    if (GraphDepth > _GraphDepth)
                    {
                        // Going down the graph - save last current as parent
                        KAssert((_GraphDepth + 1) == GraphDepth);
                        _Parent = _LastNode;
                        _GraphDepth = GraphDepth;
                    }
                    else if (GraphDepth < _GraphDepth)
                    {
                        // Going up the graph - compute what the parent was at depth GraphDepth
                        while (GraphDepth < _GraphDepth)
                        {
                            _Parent = _Parent->_Parent;
                            _GraphDepth--;
                        }
                    }

                    newNode = _new(KTL_TAG_DOM, _Allocator) DomNode(_IsMutable, Node.NamedDomValue::GetName(), _Parent);
                    if (newNode == nullptr)
                    {
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                }
                KAssert(_GraphDepth == GraphDepth);

                // Copy any value
                newNode->_IsValueDefaulted = Node._IsValueDefaulted;
                newNode->NamedDomValue::SetValue(Node.GetValue());
                status = newNode->NamedDomValue::GetValue().Status();
                if (!NT_SUCCESS(status))
                {
                    return status;
                }

                // Copy any attributes
                DomElementAttribute*    currAttr = Node._Attributes.PeekHead();
                while (currAttr != nullptr)
                {
                    DomElementAttribute::SPtr   newAttr;

                    newAttr = _new(KTL_TAG_DOM, _Allocator) DomElementAttribute(currAttr->GetName(), currAttr->GetValue());
                    if (newAttr == nullptr)
                    {
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    status = newNode->AddAttribute(newAttr);
                    if (!NT_SUCCESS(status))
                    {
                        return status;
                    }

                    currAttr = Node._Attributes.Successor(currAttr);
                }

                if (newNode->_Parent != nullptr)
                {
                    status = newNode->_Parent->AddChild(*newNode, -1);
                    if (!NT_SUCCESS(status))
                    {
                        return status;
                    }
                }

                _LastNode = Ktl::Move(newNode);
            }

            return STATUS_SUCCESS;
        }
    };

    Local               local(Mutable, GetThisAllocator());
    NTSTATUS            status = WalkGraph(GraphWalker(&local, &Local::Walker));

    if (!NT_SUCCESS(status))
    {
        Result = nullptr;
        return status;
    }

    Result = local._Root.RawPtr();
    return STATUS_SUCCESS;
}

//* Serialize a DOM graph node's attributes into a supplied KIOutputStream
NTSTATUS
DomNode::AttributuesToStream(__in KIOutputStream& Output)
{
    NTSTATUS                status = STATUS_SUCCESS;
    DomElementAttribute*    currentAttr = _Attributes.PeekHead();
    while (currentAttr != nullptr)
    {
        status = Output.Put(L' ');
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        status = currentAttr->GetName().ToStream(Output);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        status = Output.Put(L"=\"");
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        KVariant&       attr = currentAttr->GetValue();
        NTSTATUS        status1 = Output.Put((WCHAR*)attr);
        if (!NT_SUCCESS(status1))
        {
            return status1;
        }

        status1 = Output.Put(L'"');
        if (!NT_SUCCESS(status1))
        {
            return status1;
        }

        currentAttr = _Attributes.Successor(currentAttr);
    }

    // Generate a reserved (ktlns) type attributes for any well-known KVariant types
    if (!_IsValueDefaulted)
    {
        switch (NamedDomValue::GetValue().Type())
        {
            case KVariant::Type_LONG:
                return Output.Put(KXmlTypeAttrLONG);

            case KVariant::Type_ULONG:
                return Output.Put(KXmlTypeAttrULONG);

            case KVariant::Type_LONGLONG:
                return Output.Put(KXmlTypeAttrLONGLONG);

            case KVariant::Type_ULONGLONG:
                return Output.Put(KXmlTypeAttrULONGLONG);

            case KVariant::Type_GUID:
                return Output.Put(KXmlTypeAttrGUID);

            case KVariant::Type_KString_SPtr:
                return Output.Put(KXmlTypeAttrSTRING);

            case KVariant::Type_KUri_SPtr:
                return Output.Put(KXmlTypeAttrURI);

            case KVariant::Type_KDateTime:
                return Output.Put(KXmlTypeAttrDATETIME);

            case KVariant::Type_KDuration:
                return Output.Put(KXmlTypeAttrDURATION);

            case KVariant::Type_BOOLEAN:
                return Output.Put(KXmlTypeAttrBOOLEAN);

            default:
                break;
        }
    }

    return STATUS_SUCCESS;
}

KIDomNode::QName
DomNode::GetName()
{
    return KIDomNode::QName(NamedDomValue::GetName()._Namespace->GetValue(), NamedDomValue::GetName()._Name->GetValue());
}

KVariant&
DomNode::GetValue()
{
    return NamedDomValue::GetValue();
}

BOOLEAN
DomNode::GetValueKtlType()
{
    return !_IsValueDefaulted;
}

NTSTATUS
DomNode::GetAttribute(__in const KIDomNode::QName& AttributeName, __out KVariant& Value)
{
    DomElementAttribute::SPtr    da = FindAttribute(AttributeName);
    if (da != nullptr)
    {
        Value = da->GetValue();
        return STATUS_SUCCESS;
    }

    Value = KVariant();
    return STATUS_OBJECT_NAME_NOT_FOUND;
}

NTSTATUS
DomNode::GetAllAttributeNames(__out KArray<KIDomNode::QName>& Names)
{
    Names.Clear();
    DomElementAttribute*    currentAttr = _Attributes.PeekHead();
    while (currentAttr != nullptr)
    {
        FQN                 fqn = currentAttr->GetName();
        KIDomNode::QName    qn;

        qn.Name = fqn._Name->GetValue();
        qn.Namespace = fqn._Namespace->GetValue();

        NTSTATUS status = Names.Append(qn);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        currentAttr = _Attributes.Successor(currentAttr);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
DomNode::GetChild(__in const KIDomNode::QName& ChildNodeName, __out KIDomNode::SPtr& Child, __in LONG Index)
{
    DomNode::SPtr       child;
    NTSTATUS            status = FindChild(ChildNodeName, child, Index);

    if (!NT_SUCCESS(status))
    {
        Child = nullptr;
        return status;
    }

    return child->GetApiObject(Child);
}

NTSTATUS
DomNode::GetChildValue(__in const KIDomNode::QName& ChildNodeName, __out KVariant& Value, __in LONG Index)
{
    DomNode::SPtr   child;
    NTSTATUS        status = FindChild(ChildNodeName, child, Index);

    if (!NT_SUCCESS(status))
    {
        Value = KVariant();
        return status;
    }

    Value = child->NamedDomValue::GetValue();
    status = Value.Status();
    if (!NT_SUCCESS(status))
    {
        Value = KVariant();
    }

    return status;
}

NTSTATUS
DomNode::GetAllChildren(__out KArray<KIDomNode::SPtr>& Children)
{
    Children.Clear();

    for (ULONG ix = 0; ix < _Children.Count(); ix++)
    {
        KIDomNode::SPtr     childApiObj;
        NTSTATUS            status = _Children[ix]->GetApiObject(childApiObj);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        status = Children.Append(Ktl::Move(childApiObj));
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
DomNode::GetChildren(__in const KIDomNode::QName& ChildNodeName, __out KArray<KIDomNode::SPtr>& Children)
{
    Children.Clear();

    for (ULONG ix = 0; ix < _Children.Count(); ix++)
    {
        if (_Children[ix]->NamedDomValue::GetName().IsEqual(ChildNodeName))
        {
            KIDomNode::SPtr     childApiObj;
            NTSTATUS            status = _Children[ix]->GetApiObject(childApiObj);
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            status = Children.Append(childApiObj);
            if (!NT_SUCCESS(status))
            {
                return status;
            }
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
DomNode::Select(__out KArray<KIDomNode::SPtr>& Results, __in KIDomNode::WherePredicate Where, __in_opt VOID* Context)
{
    UNREFERENCED_PARAMETER(Results);            
    UNREFERENCED_PARAMETER(Where);            
    UNREFERENCED_PARAMETER(Context);       
    
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
DomNode::FindChild(__in const KIDomNode::QName& ChildNodeName, __out DomNode::SPtr& Child, __in LONG Index)
{
    if (Index >= 0)
    {
        ULONG       currentIndex = 0;

        for (ULONG i = 0; i < _Children.Count(); i++)
        {
            if (_Children[i]->NamedDomValue::GetName().IsEqual(ChildNodeName))
            {
                if (currentIndex == (ULONG)Index)
                {
                    Child = _Children[i];
                    return STATUS_SUCCESS;
                }

                currentIndex++;
            }
        }
    }
    else if (Index == -1)
    {
        // Finding last named Node
        for (ULONG i = _Children.Count(); i > 0; i--)
        {
            if (_Children[i-1]->NamedDomValue::GetName().IsEqual(ChildNodeName))
            {
                Child = _Children[i-1];
                return STATUS_SUCCESS;
            }
        }
    }

    Child = nullptr;
    return STATUS_OBJECT_NAME_NOT_FOUND;
}

NTSTATUS
DomNode::FindChildCount(__in KIDomNode::QName& ChildNodeName, __out ULONG& Count)
{
    Count = 0;

    for (ULONG i = 0; i < _Children.Count(); i++)
    {
        if (_Children[i]->NamedDomValue::GetName().IsEqual(ChildNodeName))
        {
            Count++;
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
DomNode::FindIndexOfChild(__in DomNode::SPtr Target, __out ULONG& Index)
{
    LONG count = -1;

    for (ULONG i = 0; i < _Children.Count(); i++)
    {
        if (_Children[i]->NamedDomValue::GetName().IsEqual(Target->NamedDomValue::GetName()))
        {
            count++;
        }

        if (_Children[i] == Target)
        {
            Index = (ULONG)count;
            return STATUS_SUCCESS;
        }
    }

    return STATUS_NOT_FOUND;
}


NTSTATUS
DomNode::AddChild(__in DomNode& NewChild, __in LONG AtOrBefore)
{
    if (AtOrBefore != -1)
    {
        ULONG       currentIndex = 0;

        for (ULONG i = 0; i < _Children.Count(); i++)
        {
            if (_Children[i]->NamedDomValue::GetName().IsEqual(NewChild.NamedDomValue::GetName()))
            {
                if (currentIndex == (ULONG)AtOrBefore)
                {
                    return _Children.InsertAt(i, DomNode::SPtr(&NewChild));
                }

                currentIndex++;
            }
        }
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    // No existing children w/the same name or indexing is not specifed
    return _Children.Append(&NewChild);
}

NTSTATUS
DomNode::RemoveChild(__in const KIDomNode::QName& ChildNodeName, __in LONG At)
{
    if (At >= 0)
    {
        ULONG       currentIndex = 0;

        for (ULONG i = 0; i < _Children.Count(); i++)
        {
            if (_Children[i]->NamedDomValue::GetName().IsEqual(ChildNodeName))
            {
                if ((ULONG)At == currentIndex)
                {
                    _Children.Remove(i);
                    return STATUS_SUCCESS;
                }

                currentIndex++;
            }
        }
    }
    else if (At == -1)
    {
        // Remove last named Node
        for (ULONG i = _Children.Count(); i > 0; i--)
        {
            if (_Children[i-1]->NamedDomValue::GetName().IsEqual(ChildNodeName))
            {
                _Children.Remove(i-1);
                return STATUS_SUCCESS;
            }
        }
    }

    return STATUS_OBJECT_NAME_NOT_FOUND;
}



DomElementAttribute::SPtr
DomNode::FindAttribute(__in const KIDomNode::QName& AttributeName)
{
    DomElementAttribute*    currentAttr = _Attributes.PeekHead();
    while (currentAttr != nullptr)
    {
        if (currentAttr->GetName().IsEqual(AttributeName))
        {
            return currentAttr;
        }
        currentAttr = _Attributes.Successor(currentAttr);
    }

    return nullptr;
}

NTSTATUS
DomNode::GetOrCreateQName(__in const KIDomNode::QName& QName, __out FQN& Fqn)
{
    return GetRoot().GetOrCreateQName(QName, Fqn);
}

NTSTATUS
DomNode::AddAttribute(DomElementAttribute::SPtr NewAttr)
{
    _Attributes.AppendTail(NewAttr.RawPtr());
    NewAttr->AddRef();                              // #1: Reversed in RemoveAttribute() and ctor()
    return STATUS_SUCCESS;
}

NTSTATUS
DomNode::InternalSetAttribute(__in const KIDomNode::QName& AttributeName, __in const KVariant& Value)
{
    FQN         fqn;
    NTSTATUS    status = GetOrCreateQName(AttributeName, fqn);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    DomElementAttribute::SPtr   da = FindAttribute(AttributeName);
    if (da != nullptr)
    {
        da->SetValue(Value);
        return STATUS_SUCCESS;
    }

    // Create new Attribute instance and add to the collection
    da = _new(KTL_TAG_DOM, GetThisAllocator()) DomElementAttribute(fqn, Value);
    if (da == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return AddAttribute(da);
}

NTSTATUS
DomNode::RemoveAttribute(__in const KIDomNode::QName& QName)
{
    DomElementAttribute*    currentAttr = _Attributes.PeekHead();
    while (currentAttr != nullptr)
    {
        if (currentAttr->GetName().IsEqual(QName))
        {
            KFatal(_Attributes.Remove(currentAttr) != nullptr);
            currentAttr->Release();             // #1: Reverses #1 in AddAttribute()
            return STATUS_SUCCESS;
        }
        currentAttr = _Attributes.Successor(currentAttr);
    }

    return STATUS_OBJECT_NAME_NOT_FOUND;
}

NTSTATUS
DomNode::GetApiObject(DomNodeApi::SPtr& Result)
{
    DomNodeApi::SPtr  apiObj;
    if (_ApiObj != nullptr)
    {
        apiObj = _ApiObj->TryGetTarget();
    }

    if (apiObj == nullptr)
    {
        NTSTATUS status = DomNodeApi::Create(GetRoot(), *this, apiObj, GetThisAllocator());
        if (!NT_SUCCESS(status))
        {
            Result = nullptr;
            return status;
        }

        KSpinLock&  rootLock = GetRoot().GetGraphLock();
        K_LOCK_BLOCK(rootLock)
        {
            if (_ApiObj == nullptr)
            {
                _ApiObj = apiObj->GetWeakRef();
            }
        }
    }

    Result = Ktl::Move(apiObj);
    return STATUS_SUCCESS;
}

NTSTATUS
DomNode::GetApiObject(KIDomNode::SPtr& Result)
{
    DomNodeApi::SPtr    apiObj;
    NTSTATUS            status = GetApiObject(apiObj);
    if (!NT_SUCCESS(status))
    {
        Result = nullptr;
        return status;
    }

    Result = apiObj.RawPtr();
    return STATUS_SUCCESS;
}

NTSTATUS
DomNode::GetApiObject(KIMutableDomNode::SPtr& Result)
{
    DomNodeApi::SPtr    apiObj;
    NTSTATUS            status = GetApiObject(apiObj);
    if (!NT_SUCCESS(status))
    {
        Result = nullptr;
        return status;
    }

    Result = apiObj.RawPtr();
    return STATUS_SUCCESS;
}

NTSTATUS
DomNode::SetName(__in const KIDomNode::QName& NodeName)
{
    if (!_IsMutable)
    {
        return STATUS_NOT_SUPPORTED;
    }

    FQN         fqn;
    NTSTATUS    status = GetOrCreateQName(NodeName, fqn);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    NamedDomValue::SetName(fqn);
    return STATUS_SUCCESS;
}

NTSTATUS
DomNode::SetValue(__in const KVariant& Value)
{
    if (!_IsMutable)
    {
        return STATUS_NOT_SUPPORTED;
    }

    NamedDomValue::SetValue(Value);
    return NamedDomValue::GetValue().Status();
}

NTSTATUS
DomNode::SetValueKtlType(BOOLEAN IsKtlType)
{
    if (!_IsMutable)
    {
        return STATUS_NOT_SUPPORTED;
    }

    _IsValueDefaulted = !IsKtlType;
    return STATUS_SUCCESS;
}

NTSTATUS
DomNode::SetAttribute(__in const KIDomNode::QName& AttributeName, __in const KVariant& Value)
{
    if (!_IsMutable)
    {
        return STATUS_NOT_SUPPORTED;
    }
    return InternalSetAttribute(AttributeName, Value);
}

NTSTATUS
DomNode::DeleteAttribute(__in const KIDomNode::QName& AttributeName)
{
    if (!_IsMutable)
    {
        return STATUS_NOT_SUPPORTED;
    }
    return RemoveAttribute(AttributeName);
}

NTSTATUS
DomNode::AddChild(__in const KIDomNode::QName& ChildNodeName, __out KIMutableDomNode::SPtr& UpdatableChildAdded, __in LONG Index)
{
    if (!_IsMutable)
    {
        return STATUS_NOT_SUPPORTED;
    }
    return CommonAddChild(ChildNodeName, UpdatableChildAdded, Index);
}

NTSTATUS
DomNode::DeleteChild(__in const KIDomNode::QName& ChildNodeName, __in LONG Index)
{
    if (!_IsMutable)
    {
        return STATUS_NOT_SUPPORTED;
    }
    return RemoveChild(ChildNodeName, Index);
}

NTSTATUS
DomNode::DeleteChildren(__in KIDomNode::QName& ChildNodeName)
{
    if (!_IsMutable)
    {
        return STATUS_NOT_SUPPORTED;
    }

    for (LONG ix = (_Children.Count() - 1); ix >= 0; ix--)
    {
        if (_Children[ix]->NamedDomValue::GetName().IsEqual(ChildNodeName))
        {
            _Children.Remove(ix);
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
DomNode::AppendChild(
    __in const KIDomNode::QName& ChildNodeName,
    __out KIMutableDomNode::SPtr& UpdatableChildCreated)
{
    if (!_IsMutable)
    {
        return STATUS_NOT_SUPPORTED;
    }
    return CommonAddChild(ChildNodeName, UpdatableChildCreated);
}

NTSTATUS
DomNode::AddChildDom(__in DomNode& DomToAdd, __in LONG Index)
{
    if (!_IsMutable)
    {
        return STATUS_NOT_SUPPORTED;
    }

    if (!DomToAdd.IsDomRoot() || !DomToAdd._IsMutable)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    struct Local
    {
        NTSTATUS
        static CreateAndMove(DomNode& From, DomNode& Parent, DomNode::SPtr& Result)
        {
            Result = _new(KTL_TAG_DOM, From.GetThisAllocator()) DomNode((DomNode&&)From, Parent);
            if (Result == nullptr)
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            NTSTATUS        status = Result->Status();
            if (!NT_SUCCESS(status))
            {
                Result = nullptr;
            }

            return status;
        }
    };

    DomNode::SPtr       newChild;
    if (Index != -1)
    {
        ULONG       currentIndex = 0;

        for (ULONG i = 0; i < _Children.Count(); i++)
        {
            if (_Children[i]->NamedDomValue::GetName().IsEqual(DomToAdd.NamedDomValue::GetName()))
            {
                if (currentIndex == (ULONG)Index)
                {
                    NTSTATUS    status = Local::CreateAndMove(DomToAdd, *this, newChild);
                    if (!NT_SUCCESS(status))
                    {
                        return status;
                    }

                    return _Children.InsertAt(i, Ktl::Move(newChild));
                }

                currentIndex++;
            }
        }
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    // No existing children w/the same name or indexing is not specifed
    NTSTATUS    status = Local::CreateAndMove(DomToAdd, *this, newChild);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return _Children.Append(Ktl::Move(newChild));
}

NTSTATUS
DomNode::SetChildValue(__in const KIDomNode::QName& ChildNodeName, __in KVariant& Value, __in LONG Index)
{
    if (!_IsMutable)
    {
        return STATUS_NOT_SUPPORTED;
    }

    DomNode::SPtr   child;
    NTSTATUS        status = FindChild(ChildNodeName, child, Index);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    child->NamedDomValue::SetValue(Value);
    return child->NamedDomValue::GetValue().Status();
}

NTSTATUS
DomNode::CommonAddChild(__in const KIDomNode::QName& ChildNodeName, __out KIMutableDomNode::SPtr& UpdatableChildAdded, __in LONG Index)
{
    UpdatableChildAdded = nullptr;

    FQN         fqn;
    NTSTATUS    status = GetOrCreateQName(ChildNodeName, fqn);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    DomNode::SPtr   newChild;

    status = Create(TRUE, fqn, *this, newChild, GetThisAllocator());
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = DomNode::AddChild(*newChild, Index);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return newChild->GetApiObject(UpdatableChildAdded);
}


//*** DomNode::ParserHookBase implementation
DomNode::ParserHookBase::ParserHookBase(KAllocator& Allocator, DomRoot& Root, ElementFactory Factory)
    :   _Allocator(Allocator),
        _DomRoot(Root),
        _CurrentNode(nullptr),
        _CurrentNodeType(KVariant::Type_EMPTY),
        _ElementFactory(Factory)
{
}

DomNode::ParserHookBase::~ParserHookBase()
{
}

NTSTATUS
DomNode::ParserHookBase::OpenElement(
    __in_ecount(ElementNsLength) WCHAR* ElementNs, 
    __in ULONG  ElementNsLength, 
    __in_ecount(ElementNameLength) WCHAR* ElementName, 
    __in ULONG  ElementNameLength, 
    __in_z const WCHAR* StartElementSection)
{
    NTSTATUS                status = STATUS_SUCCESS;
    NamedDomValue::FQN      elementName;

    UNREFERENCED_PARAMETER(StartElementSection);            

    status = _DomRoot.GetOrCreateQName(
        ElementNs,
        ElementNsLength,
        ElementName,
        ElementNameLength,
        elementName);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    DomNode::SPtr    newEle = _ElementFactory(_DomRoot, _CurrentNode.RawPtr(), elementName, _Allocator);

    if (newEle == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (_CurrentNode != nullptr)
    {
        status = _CurrentNode->AddChild(*newEle, -1);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }

    _CurrentNode = Ktl::Move(newEle);
    _CurrentNodeType = KVariant::KVariantType::Type_KString_SPtr;
    _CurrentNodeTypeDefaulted = TRUE;
    return STATUS_SUCCESS;
}

NTSTATUS
DomNode::ParserHookBase::AddAttribute(
    __in BOOLEAN HeaderAttributes,    // Set to true if these are the <?xml header attributes
    __in_ecount(AttributeNsLength) WCHAR* AttributeNs,
    __in ULONG  AttributeNsLength,
    __in_ecount(AttributeNameLength) WCHAR* AttributeName,
    __in ULONG  AttributeNameLength,
    __in_ecount(ValueLength) WCHAR* Value,
    __in ULONG  ValueLength)
{
    // Drop header attributes
    if (HeaderAttributes)
    {
        // BUG richhas, xxxxxx, consider what special support there should be for header attributes
        return STATUS_SUCCESS;
    }

    // Detect reserved attributes
    if ((AttributeNsLength == SizeOfKXmlNs) &&
        (AttributeNameLength == SizeOfKXmlTypeName) &&
        (memcmp(KXmlNs, AttributeNs, SizeOfKXmlNs * sizeof(WCHAR)) == 0) &&
        (memcmp(KXmlTypeName, AttributeName, SizeOfKXmlTypeName * sizeof(WCHAR)) == 0))
    {
        // NOTE: Imp is aimed at perf
        if ((ValueLength == SizeOfKXmlTypeNameGUID) &&
            (memcmp(KXmlTypeNameGUID, Value, SizeOfKXmlTypeNameGUID * sizeof(WCHAR)) == 0))
        {
            _CurrentNodeType = KVariant::KVariantType::Type_GUID;
            _CurrentNodeTypeDefaulted = FALSE;
            return STATUS_SUCCESS;
        }
        else if ((ValueLength == SizeOfKXmlTypeNameLONG) &&
                 (memcmp(KXmlTypeNameLONG, Value, SizeOfKXmlTypeNameLONG * sizeof(WCHAR)) == 0))
        {
            _CurrentNodeType = KVariant::KVariantType::Type_LONG;
            _CurrentNodeTypeDefaulted = FALSE;
            return STATUS_SUCCESS;
        }
        else if ((ValueLength == SizeOfKXmlTypeNameULONG) &&
                 (memcmp(KXmlTypeNameULONG, Value, SizeOfKXmlTypeNameULONG * sizeof(WCHAR)) == 0))
        {
            _CurrentNodeType = KVariant::KVariantType::Type_ULONG;
            _CurrentNodeTypeDefaulted = FALSE;
            return STATUS_SUCCESS;
        }
        else if ((ValueLength == SizeOfKXmlTypeNameLONGLONG) &&
                 (memcmp(KXmlTypeNameLONGLONG, Value, SizeOfKXmlTypeNameLONGLONG * sizeof(WCHAR)) == 0))
        {
            _CurrentNodeType = KVariant::KVariantType::Type_LONGLONG;
            _CurrentNodeTypeDefaulted = FALSE;
            return STATUS_SUCCESS;
        }
        else if ((ValueLength == SizeOfKXmlTypeNameULONGLONG) &&
                 (memcmp(KXmlTypeNameULONGLONG, Value, SizeOfKXmlTypeNameULONGLONG * sizeof(WCHAR)) == 0))
        {
            _CurrentNodeType = KVariant::KVariantType::Type_ULONGLONG;
            _CurrentNodeTypeDefaulted = FALSE;
            return STATUS_SUCCESS;
        }
        else if ((ValueLength == SizeOfKXmlTypeNameBOOLEAN) &&
                 (memcmp(KXmlTypeNameBOOLEAN, Value, SizeOfKXmlTypeNameBOOLEAN * sizeof(WCHAR)) == 0))
        {
            _CurrentNodeType = KVariant::KVariantType::Type_BOOLEAN;
            _CurrentNodeTypeDefaulted = FALSE;
            return STATUS_SUCCESS;
        }
        else if ((ValueLength == SizeOfKXmlTypeNameDURATION) &&
                 (memcmp(KXmlTypeNameDURATION, Value, SizeOfKXmlTypeNameDURATION * sizeof(WCHAR)) == 0))
        {
            _CurrentNodeType = KVariant::KVariantType::Type_KDuration;
            _CurrentNodeTypeDefaulted = FALSE;
            return STATUS_SUCCESS;
        }
        else if ((ValueLength == SizeOfKXmlTypeNameDATETIME) &&
                 (memcmp(KXmlTypeNameDATETIME, Value, SizeOfKXmlTypeNameDATETIME * sizeof(WCHAR)) == 0))
        {
            _CurrentNodeType = KVariant::KVariantType::Type_KDateTime;
            _CurrentNodeTypeDefaulted = FALSE;
            return STATUS_SUCCESS;
        }
        else if ((ValueLength == SizeOfKXmlTypeNameURI) &&
                 (memcmp(KXmlTypeNameURI, Value, SizeOfKXmlTypeNameURI * sizeof(WCHAR)) == 0))
        {
            _CurrentNodeType = KVariant::KVariantType::Type_KUri_SPtr;
            _CurrentNodeTypeDefaulted = FALSE;
            return STATUS_SUCCESS;
        }
        else if ((ValueLength == SizeOfKXmlTypeNameSTRING) &&
                 (memcmp(KXmlTypeNameSTRING, Value, SizeOfKXmlTypeNameSTRING * sizeof(WCHAR)) == 0))
        {
            _CurrentNodeType = KVariant::KVariantType::Type_KString_SPtr;
            _CurrentNodeTypeDefaulted = FALSE;
            return STATUS_SUCCESS;
        }

        __analysis_assume(wcslen(KtlXmlTypePrefix) >= SizeOfKXmlTypeNameLONG);
        if ((ValueLength >= SizeOfKXmlTypeNameLONG) &&
            (memcmp(KtlXmlTypePrefix, Value, SizeOfKXmlTypeNameLONG * sizeof(WCHAR)) == 0))
        {
            // invalid reserved ktl:<type>
            return STATUS_INVALID_INFO_CLASS;
        }

        // let all other types default to string
    }

    NamedDomValue::FQN      attrName;
    NTSTATUS                status = STATUS_SUCCESS;


    KVariant tmp = KVariant::Create(KStringView(Value, ValueLength, ValueLength), KVariant::Type_KString_SPtr,  _Allocator);
    if (!tmp)
    {
        return tmp.Status();
    }
    status = _DomRoot.GetOrCreateQName(
        AttributeNs,
        AttributeNsLength,
        AttributeName,
        AttributeNameLength,
        attrName);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = _CurrentNode->InternalSetAttribute(
        KIDomNode::QName(attrName._Namespace->GetValue(), attrName._Name->GetValue()),
        tmp);

    return status;
}

NTSTATUS
DomNode::ParserHookBase::CloseAllAttributes()
{
    return STATUS_SUCCESS;
}

NTSTATUS
DomNode::ParserHookBase::AddContent(
    __in ULONG  ContentIndex,
    __in ULONG  ContentType,
    __in_ecount(ContentLength) WCHAR* ContentText,
    __in ULONG  ContentLength)
{
    NTSTATUS        status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(ContentIndex);            
    UNREFERENCED_PARAMETER(ContentType);            

    _CurrentNode->_IsValueDefaulted = _CurrentNodeTypeDefaulted;

    switch (_CurrentNodeType)
    {
        case KVariant::KVariantType::Type_GUID:
        {
            KStringView     input(ContentText, ContentLength, ContentLength);
            GUID            value;

            if (!input.ToGUID(value))
            {
                status = STATUS_INVALID_PARAMETER_1;
            }
            else
            {
                _CurrentNode->NamedDomValue::SetValue(KVariant(value));
            }
            break;
        }
        case KVariant::KVariantType::Type_LONG:
        {
            KStringView     input(ContentText, ContentLength, ContentLength);
            LONG            value;

            if (!input.ToLONG(value))
            {
                status = STATUS_INVALID_PARAMETER_1;
            }
            else
            {
                _CurrentNode->NamedDomValue::SetValue(KVariant(value));
            }
            break;
        }
        case KVariant::KVariantType::Type_LONGLONG:
        {
            KStringView     input(ContentText, ContentLength, ContentLength);
            LONGLONG        value;

            if (!input.ToLONGLONG(value))
            {
                status = STATUS_INVALID_PARAMETER_1;
            }
            else
            {
                _CurrentNode->NamedDomValue::SetValue(KVariant(value));
            }
            break;
        }
        case KVariant::KVariantType::Type_ULONG:
        {
            KStringView     input(ContentText, ContentLength, ContentLength);
            ULONG           value;

            if (!input.ToULONG(value))
            {
                status = STATUS_INVALID_PARAMETER_1;
            }
            else
            {
                _CurrentNode->NamedDomValue::SetValue(KVariant(value));
            }
            break;
        }
        case KVariant::KVariantType::Type_ULONGLONG:
        {
            KStringView     input(ContentText, ContentLength, ContentLength);
            ULONGLONG       value;

            if (!input.ToULONGLONG(value))
            {
                status = STATUS_INVALID_PARAMETER_1;
            }
            else
            {
                _CurrentNode->NamedDomValue::SetValue(KVariant(value));
            }
            break;
        }
        case KVariant::KVariantType::Type_BOOLEAN:
        {
            KVariant vTmp = KVariant::Create(KStringView(ContentText, ContentLength, ContentLength), KVariant::Type_BOOLEAN, _Allocator);
            if (!vTmp)
            {
                return vTmp.Status();
            }
            else
            {
                _CurrentNode->NamedDomValue::SetValue(vTmp);
            }
            break;
        }
        case KVariant::KVariantType::Type_KDateTime:
        {
            KVariant vTmp = KVariant::Create(KStringView(ContentText, ContentLength, ContentLength), KVariant::Type_KDateTime, _Allocator);
            if (!vTmp)
            {
                return vTmp.Status();
            }
            else
            {
                _CurrentNode->NamedDomValue::SetValue(vTmp);
            }
            break;
        }
        case KVariant::KVariantType::Type_KDuration:
        {
            KVariant vTmp = KVariant::Create(KStringView(ContentText, ContentLength, ContentLength), KVariant::Type_KDuration, _Allocator);
            if (!vTmp)
            {
                return vTmp.Status();
            }
            else
            {
                _CurrentNode->NamedDomValue::SetValue(vTmp);
            }
            break;
        }
        case KVariant::KVariantType::Type_KUri_SPtr:
        {
            KVariant vTmp = KVariant::Create(KStringView(ContentText, ContentLength, ContentLength), KVariant::Type_KUri_SPtr, _Allocator);
            if (!vTmp)
            {
                return vTmp.Status();
            }
            else
            {
                _CurrentNode->NamedDomValue::SetValue(vTmp);
            }
            break;
        }

        case KVariant::KVariantType::Type_KString_SPtr:
        {
            KVariant vTmp = KVariant::Create(KStringView(ContentText, ContentLength, ContentLength), KVariant::Type_KString_SPtr, _Allocator);
            if (!vTmp)
            {
                return vTmp.Status();
            }
            _CurrentNode->NamedDomValue::SetValue(vTmp);
            break;
        }

        default:
            KFatal(FALSE);
    }

    _CurrentNodeType = KVariant::KVariantType::Type_EMPTY;
    return status;
}

NTSTATUS
DomNode::ParserHookBase::CloseElement(ULONG ElementSectionLength)
{
    UNREFERENCED_PARAMETER(ElementSectionLength);        
    
    // Make allowance for null strings - AddContent() not called for empty elements

    if (_CurrentNodeType == KVariant::KVariantType::Type_KString_SPtr)
    {
        _CurrentNode->_IsValueDefaulted = _CurrentNodeTypeDefaulted;
        _CurrentNode->GetValue() = KVariant::Create(L"", _Allocator);
        NTSTATUS  status = _CurrentNode->GetValue().Status();
        if (!NT_SUCCESS(status))
        {
            return status;
        }
        _CurrentNodeType = KVariant::KVariantType::Type_EMPTY;
    }

    if (_CurrentNodeType != KVariant::KVariantType::Type_EMPTY)
    {
        return STATUS_INVALID_PARAMETER_1;          // Indicate element value error
    }

    _CurrentNode = _CurrentNode->GetParent();
    return STATUS_SUCCESS;
}

NTSTATUS
DomNode::ParserHookBase::Done(BOOLEAN Error)
{
    UNREFERENCED_PARAMETER(Error);            
    return STATUS_SUCCESS;
}

BOOLEAN
DomNode::ParserHookBase::ElementWhitespacePreserved()
{
    return FALSE;
}


//*** DomNodeApi implementation
NTSTATUS
DomNodeApi::Create(DomNode& GraphRoot, DomNode& BackingNode, DomNodeApi::SPtr& Result, KAllocator& Allocator)
{
    // Create a API wrapper object
    Result = _new(KTL_TAG_DOM, Allocator) DomNodeApi(GraphRoot, BackingNode);
    if (Result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}

DomNodeApi::DomNodeApi(DomNode& GraphRoot, DomNode& BackingNode)
    :   _Graph(&GraphRoot),
        _BackingNode(&BackingNode),
        _NullValue()
{
}

DomNodeApi::~DomNodeApi()
{
}

BOOLEAN
DomNodeApi::IsMutable()
{
    if (_BackingNode == nullptr)
    {
        return FALSE;
    }

    return _BackingNode->IsMutable();
}

KSharedPtr<KIMutableDomNode>
DomNodeApi::ToMutableForm()
{
    if (_BackingNode == nullptr)
    {
        return nullptr;
    }

    if (_BackingNode->IsMutable())
    {
        return this;
    }
    return nullptr;
}

DomNodeApi::QName
DomNodeApi::GetName()
{
    if (_BackingNode == nullptr)
    {
        return DomNodeApi::QName();
    }

    return _BackingNode->GetName();
}

KVariant&
DomNodeApi::GetValue()
{
    if (_BackingNode == nullptr)
    {
        return _NullValue;
    }

    return _BackingNode->GetValue();
}

BOOLEAN
DomNodeApi::GetValueKtlType()
{
    if (_BackingNode == nullptr)
    {
        return FALSE;
    }

    return _BackingNode->GetValueKtlType();
}

NTSTATUS
DomNodeApi::GetAttribute(__in const QName& AttributeName, __out KVariant& Value)
{
    if (_BackingNode == nullptr)
    {
        return STATUS_INVALID_HANDLE;
    }

    return _BackingNode->GetAttribute(AttributeName, Value);
}

NTSTATUS
DomNodeApi::GetAllAttributeNames(__out KArray<QName>& Names)
{
    if (_BackingNode == nullptr)
    {
        return STATUS_INVALID_HANDLE;
    }

    return _BackingNode->GetAllAttributeNames(Names);
}

NTSTATUS
DomNodeApi::GetChild(__in const QName& ChildNodeName, __out KIDomNode::SPtr& Child, __in LONG Index)
{
    if (_BackingNode == nullptr)
    {
        return STATUS_INVALID_HANDLE;
    }

    return _BackingNode->GetChild(ChildNodeName, Child, Index);
}

NTSTATUS
DomNodeApi::GetChildValue(__in const QName& ChildNodeName, __out KVariant& Value, __in LONG Index)
{
    if (_BackingNode == nullptr)
    {
        return STATUS_INVALID_HANDLE;
    }

    return _BackingNode->GetChildValue(ChildNodeName, Value, Index);
}

NTSTATUS
DomNodeApi::GetAllChildren(__out KArray<KIDomNode::SPtr>& Children)
{
    if (_BackingNode == nullptr)
    {
        return STATUS_INVALID_HANDLE;
    }

    return _BackingNode->GetAllChildren(Children);
}

NTSTATUS
DomNodeApi::GetChildren(__in const QName& ChildNodeName, __out KArray<KIDomNode::SPtr>& Children)
{
    if (_BackingNode == nullptr)
    {
        return STATUS_INVALID_HANDLE;
    }

    return _BackingNode->GetChildren(ChildNodeName, Children);
}

NTSTATUS
DomNodeApi::Select(
    __out KArray<KIDomNode::SPtr>& Results,
    __in WherePredicate Where,
    __in_opt void* Context )
{
    if (_BackingNode == nullptr)
    {
        return STATUS_INVALID_HANDLE;
    }

    return _BackingNode->Select(Results, Where, Context);
}

NTSTATUS
DomNodeApi::SetName(__in const QName& NodeName)
{
    if (_BackingNode == nullptr)
    {
        return STATUS_INVALID_HANDLE;
    }

    return _BackingNode->SetName(NodeName);
}

NTSTATUS
DomNodeApi::SetValue(__in const KVariant& Value)
{
    if (_BackingNode == nullptr)
    {
        return STATUS_INVALID_HANDLE;
    }

    return _BackingNode->SetValue(Value);
}

NTSTATUS
DomNodeApi::SetValueKtlType(__in BOOLEAN IsKtlType)
{
    if (_BackingNode == nullptr)
    {
        return STATUS_INVALID_HANDLE;
    }

    return _BackingNode->SetValueKtlType(IsKtlType);
}

NTSTATUS
DomNodeApi::SetAttribute(__in const QName& AttributeName, __in const KVariant& Value)
{
    if (_BackingNode == nullptr)
    {
        return STATUS_INVALID_HANDLE;
    }

    return _BackingNode->SetAttribute(AttributeName, Value);
}

NTSTATUS
DomNodeApi::DeleteAttribute(__in const QName& AttributeName)
{
    if (_BackingNode == nullptr)
    {
        return STATUS_INVALID_HANDLE;
    }

    return _BackingNode->DeleteAttribute(AttributeName);
}

NTSTATUS
DomNodeApi::AddChild(__in const QName& ChildNodeName, __out KIMutableDomNode::SPtr& UpdatableChildAdded, __in LONG Index)
{
    if (_BackingNode == nullptr)
    {
        return STATUS_INVALID_HANDLE;
    }

    return _BackingNode->AddChild(ChildNodeName, UpdatableChildAdded, Index);
}

NTSTATUS
DomNodeApi::DeleteChild(__in const QName& ChildNodeName, __in LONG Index)
{
    if (_BackingNode == nullptr)
    {
        return STATUS_INVALID_HANDLE;
    }

    return _BackingNode->DeleteChild(ChildNodeName, Index);
}

NTSTATUS
DomNodeApi::DeleteChildren(__in QName& ChildNodeName)
{
    if (_BackingNode == nullptr)
    {
        return STATUS_INVALID_HANDLE;
    }

    return _BackingNode->DeleteChildren(ChildNodeName);
}

NTSTATUS
DomNodeApi::AppendChild(__in const QName& ChildNodeName, __out KIMutableDomNode::SPtr& UpdatableChildCreated)
{
    if (_BackingNode == nullptr)
    {
        return STATUS_INVALID_HANDLE;
    }

    return _BackingNode->AppendChild(ChildNodeName, UpdatableChildCreated);
}

NTSTATUS
DomNodeApi::SetChildValue(__in const QName& ChildNodeName, __in KVariant& Value, __in LONG Index)
{
    if (_BackingNode == nullptr)
    {
        return STATUS_INVALID_HANDLE;
    }

    return _BackingNode->SetChildValue(ChildNodeName, Value, Index);
}

NTSTATUS
DomNodeApi::AddChildDom(__in KIMutableDomNode& DomToAdd, __in LONG Index)
{
    if ((_BackingNode == nullptr) || (((DomNodeApi*)&DomToAdd)->_BackingNode == nullptr))
    {
        return STATUS_INVALID_HANDLE;
    }

    return _BackingNode->AddChildDom(*((DomNodeApi*)(&DomToAdd))->_BackingNode, Index);
}


NTSTATUS
DomNodeApi::GetValue(__in const KDomPath& Path, __out KVariant& Value)
{
    KArray<KDomPath::PToken> ParseOutput(GetThisAllocator());
    NTSTATUS Res = Path.Parse(GetThisAllocator(), ParseOutput);
    LONG CurrentArrayIndex = 0;
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    // Since the current node is part of the path, verify that the current
    // name matches the first token in the path.
    DomNode::SPtr CurrentNode = _BackingNode;
    QName Qn;
    Qn.Name = (LPWSTR) *ParseOutput[0]._Ident;
    if (ParseOutput[0]._NsPrefix)
    {
        Qn.Namespace = (LPWSTR) *ParseOutput[0]._NsPrefix;
    }
    if (!CurrentNode->NamedDomValue::GetName().IsEqual(Qn))
    {
        return STATUS_OBJECT_PATH_INVALID;
    }

    // Check the remaining tokens
    for (ULONG i = 1; i < ParseOutput.Count(); i++)
    {
        Qn.Name = (LPWSTR) *ParseOutput[i]._Ident;
        if (ParseOutput[i]._NsPrefix)
        {
            Qn.Namespace = (LPWSTR) *ParseOutput[i]._NsPrefix;
        }

        switch (ParseOutput[i]._IdentType)
        {
            case KDomPath::eNone:
                return STATUS_INTERNAL_ERROR;
                break;

            case KDomPath::eElement:
            {
                DomNode::SPtr Child;
                NTSTATUS Res1 = CurrentNode->FindChild(Qn, Child, 0);
                if (!NT_SUCCESS(Res1))
                {
                    return Res1;
                }
                CurrentNode = Child;
                continue;
            }
            break;

            case KDomPath::eArrayElement:
            {
                if (CurrentArrayIndex >= KDomPath::MaxIndices)
                {
                    return STATUS_RANGE_NOT_FOUND;
                }
                LONG IndexValue = -1;
                if (ParseOutput[i]._LiteralIndex != -1)
                {
                    IndexValue = ParseOutput[i]._LiteralIndex;
                }
                else
                {
                    IndexValue = Path._Indices[CurrentArrayIndex++];
                    if (IndexValue == -1)
                    {
                        return STATUS_INVALID_PARAMETER;
                    }
                }

                DomNode::SPtr Child;
                NTSTATUS Res1 = CurrentNode->FindChild(Qn, Child, IndexValue);
                if (!NT_SUCCESS(Res1))
                {
                    return Res1;
                }
                CurrentNode = Child;
                continue;
            }
            break;


            case KDomPath::eAttribute:
            {
                return CurrentNode->GetAttribute(Qn, Value);
            }
            break;
        }
    }

    Value = CurrentNode->GetValue();
    return STATUS_SUCCESS;
}

NTSTATUS
DomNodeApi::GetNode(__in const KDomPath& Path, KIDomNode::SPtr& TargetNode)
{
    KArray<KDomPath::PToken> ParseOutput(GetThisAllocator());
    NTSTATUS Res = Path.Parse(GetThisAllocator(), ParseOutput);
    LONG CurrentArrayIndex = 0;

    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    DomNode::SPtr CurrentNode = _BackingNode;
    QName Qn;
    Qn.Name = (LPWSTR) *ParseOutput[0]._Ident;
    if (ParseOutput[0]._NsPrefix)
    {
        Qn.Namespace = (LPWSTR) *ParseOutput[0]._NsPrefix;
    }
    if (!CurrentNode->NamedDomValue::GetName().IsEqual(Qn))
    {
        return STATUS_OBJECT_PATH_INVALID;
    }

    for (ULONG i = 1; i < ParseOutput.Count(); i++)
    {
        Qn.Name = (LPWSTR) *ParseOutput[i]._Ident;
        if (ParseOutput[i]._NsPrefix)
        {
            Qn.Namespace = (LPWSTR) *ParseOutput[i]._NsPrefix;
        }

        switch (ParseOutput[i]._IdentType)
        {
            case KDomPath::eNone:
                return STATUS_INTERNAL_ERROR;
                break;

            case KDomPath::eElement:
            {
                DomNode::SPtr Child;
                NTSTATUS Res1 = CurrentNode->FindChild(Qn, Child, 0);
                if (!NT_SUCCESS(Res1))
                {
                    return Res1;
                }
                CurrentNode = Child;
                continue;
            }
            break;

            case KDomPath::eArrayElement:
            {
                if (CurrentArrayIndex >= KDomPath::MaxIndices)
                {
                    return STATUS_RANGE_NOT_FOUND;
                }
                LONG IndexValue = -1;
                if (ParseOutput[i]._LiteralIndex != -1)
                {
                    IndexValue = ParseOutput[i]._LiteralIndex;
                }
                else
                {
                    IndexValue = Path._Indices[CurrentArrayIndex++];
                    if (IndexValue == -1)
                    {
                        return STATUS_INVALID_PARAMETER;
                    }
                }

                DomNode::SPtr Child;
                NTSTATUS Res1 = CurrentNode->FindChild(Qn, Child, IndexValue);
                if (!NT_SUCCESS(Res1))
                {
                    return Res1;
                }
                CurrentNode = Child;
                continue;
            }
            break;


            case KDomPath::eAttribute:
            {
                // This occurs if the path mistakenly includes an attribute accessor
                return STATUS_OBJECT_TYPE_MISMATCH;
            }
            break;
        }
    }

    return CurrentNode->GetApiObject(TargetNode);
}

NTSTATUS
DomNodeApi::GetNodes(__in const KDomPath& Path, __out KArray<KIDomNode::SPtr>& ResultSet)
{
    KArray<KDomPath::PToken> ParseOutput(GetThisAllocator());
    NTSTATUS Res = Path.Parse(GetThisAllocator(), ParseOutput);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    DomNode::SPtr CurrentNode = _BackingNode;
    QName Qn;
    Qn.Name = (LPWSTR) *ParseOutput[0]._Ident;
    if (ParseOutput[0]._NsPrefix)
    {
        Qn.Namespace = (LPWSTR) *ParseOutput[0]._NsPrefix;
    }
    if (!CurrentNode->NamedDomValue::GetName().IsEqual(Qn))
    {
        return STATUS_OBJECT_PATH_INVALID;
    }

    if (ParseOutput.Count() < 2)
    {
        return STATUS_OBJECT_PATH_INVALID;
    }

    // Verify the last referenced token is a simple  element name
    if (ParseOutput[ParseOutput.Count()-1]._IdentType != KDomPath::eElement)
    {
        return STATUS_OBJECT_PATH_INVALID;
    }

    // Get the node parent node by getting the target
    // and then getting its parent.
    KIDomNode::SPtr Temp;
    Res = DomNodeApi::GetNode(Path, Temp);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    DomNodeApi::SPtr Alias = up_cast<DomNodeApi>(Temp);
    DomNode::SPtr Parent = Alias->GetBackingNode()->_Parent;

    // Now we just need the last token name in the path
    Qn.Name = (LPWSTR) *ParseOutput[ParseOutput.Count()-1]._Ident;
    if (ParseOutput[ParseOutput.Count()-1]._NsPrefix)
    {
        Qn.Namespace = (LPWSTR) *ParseOutput[ParseOutput.Count()-1]._NsPrefix;
    }

    // Now return the children based on the last token name in the path
    return Parent->GetChildren(Qn, ResultSet);
}

NTSTATUS
DomNodeApi::GetCount(__in const KDomPath& Path, __out ULONG& Count)
{
    KArray<KDomPath::PToken> ParseOutput(GetThisAllocator());
    NTSTATUS Res = Path.Parse(GetThisAllocator(), ParseOutput);
    LONG CurrentArrayIndex = 0;
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    DomNode::SPtr CurrentNode = _BackingNode;
    QName Qn;
    Qn.Name = (LPWSTR) *ParseOutput[0]._Ident;
    if (ParseOutput[0]._NsPrefix)
    {
        Qn.Namespace = (LPWSTR) *ParseOutput[0]._NsPrefix;
    }
    if (!CurrentNode->NamedDomValue::GetName().IsEqual(Qn))
    {
        return STATUS_OBJECT_PATH_INVALID;
    }

    ULONG i = 1;
    for ( ; i < ParseOutput.Count(); i++)
    {
        Qn.Name = (LPWSTR) *ParseOutput[i]._Ident;
        if (ParseOutput[i]._NsPrefix)
        {
            Qn.Namespace = (LPWSTR) *ParseOutput[i]._NsPrefix;
        }

        switch (ParseOutput[i]._IdentType)
        {
            case KDomPath::eNone:
                return STATUS_INTERNAL_ERROR;
                break;

            case KDomPath::eElement:
            {
                DomNode::SPtr Child;
                NTSTATUS Res1 = CurrentNode->FindChild(Qn, Child, 0);
                if (!NT_SUCCESS(Res1))
                {
                    return Res1;
                }
                CurrentNode = Child;
                continue;
            }
            break;

            case KDomPath::eArrayElement:
            {
                if (CurrentArrayIndex >= KDomPath::MaxIndices)
                {
                    return STATUS_RANGE_NOT_FOUND;
                }
                LONG IndexValue = -1;
                if (ParseOutput[i]._LiteralIndex != -1)
                {
                    IndexValue = ParseOutput[i]._LiteralIndex;
                }
                else
                {
                    IndexValue = Path._Indices[CurrentArrayIndex++];
                    if (IndexValue == -1)
                    {
                        return STATUS_INVALID_PARAMETER;
                    }
                }

                DomNode::SPtr Child;
                NTSTATUS Res1 = CurrentNode->FindChild(Qn, Child, IndexValue);
                if (!NT_SUCCESS(Res1))
                {
                    return Res1;
                }
                CurrentNode = Child;
                continue;
            }
            break;

            // If an attribute is in the path, the count will be zero or one.
            case KDomPath::eAttribute:
            {
                KVariant DummyValue;
                NTSTATUS Res1 = CurrentNode->GetAttribute(Qn, DummyValue);
                if (NT_SUCCESS(Res1))
                {
                    Count = 1;
                }
                else
                {
                    Count = 0;
                }
                return Res1;
            }
            break;
        }
    }

    // If here, we have reached the target node.  There are two cases.
    // If the target node was an array element, then the count is one.
    if (ParseOutput[--i]._IdentType == KDomPath::eArrayElement)
    {
        Count = 1;
        return STATUS_SUCCESS;
    }

    // If the target node was just a name, then we have to go back to the parent
    // and use the last name we found and get the count for that name.
    DomNode::SPtr Parent = CurrentNode->_Parent;
    return Parent->FindChildCount(Qn, Count);
}

NTSTATUS
DomNodeApi::GetPath(__out KString::SPtr& PathToThis)
{
    NTSTATUS result;

    PathToThis = KString::Create(GetThisAllocator(), 256);
    if (!PathToThis)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DomNode::SPtr currentNode = _BackingNode;
    while (currentNode)
    {
        QName currentName = currentNode->GetName();
        DomNode::SPtr parent = currentNode->_Parent;
        ULONG ix = 0;

        if (parent)
        {
            // If here, we test to see if the current element might be
            // a repeated element in an array.
            result = parent->FindIndexOfChild(currentNode, ix);
            if (!NT_SUCCESS(result))
            {
                return STATUS_INTERNAL_ERROR;
            }

            if (ix > 0)
            {
                KLocalString<KStringView::Max64BitNumString + 2> Tmp;
                if (!Tmp.FromULONG(ix))
                {
                    return STATUS_INTERNAL_ERROR;
                }

                Tmp.PrependChar(L'[');   // Can't fail
                Tmp.AppendChar(L']');    // Nor this

                if (!PathToThis->Prepend(Tmp))
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
            }
        }
        if (!PathToThis->Prepend(KStringView(currentName.Name)))
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        if (!KStringView(currentName.Namespace).IsEmpty())
        {
            if (!PathToThis->PrependChar(L':'))
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            if (!PathToThis->Prepend(KStringView(currentName.Namespace)))
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        currentNode = currentNode->_Parent;
        if (currentNode)
        {
            if (!PathToThis->PrependChar(L'/'))
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

    if (!PathToThis->SetNullTerminator())
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
KIDomNode::ValidateNamespaceAttribute(__in_z LPCWSTR Namespace)
{
    NTSTATUS status = STATUS_SUCCESS;
    KVariant ns;

    status = GetAttribute(QName(L"xmlns"), ns);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (wcscmp((PWSTR)ns, Namespace) != 0)
    {
        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    return STATUS_SUCCESS;
}

KIDomNode::KIDomNode()
{
}

KIDomNode::~KIDomNode()
{
}

KIMutableDomNode::KIMutableDomNode()
{
}

KIMutableDomNode::~KIMutableDomNode()
{
}


//*** UnicodeOutputStream(Imp) static class implementation
UnicodeOutputStreamImp::UnicodeOutputStreamImp()
    :   _NextOffset(0)
{
}

UnicodeOutputStreamImp::UnicodeOutputStreamImp(ULONG InitialSize)
    :   _NextOffset(0)
{
    NTSTATUS        status = KBuffer::Create(InitialSize, _Buffer, GetThisAllocator(), KTL_TAG_DOM);

    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    status = Put(KTextFile::UnicodeBom);
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }
}

KDom::UnicodeOutputStream::UnicodeOutputStream()
{
}

KDom::UnicodeOutputStream::~UnicodeOutputStream()
{
}

UnicodeOutputStreamImp::~UnicodeOutputStreamImp()
{
}

NTSTATUS
UnicodeOutputStreamImp::Create(
    __out KDom::UnicodeOutputStream::SPtr& Result,
    __in KAllocator& Allocator,
    __in ULONG Tag,
    __in ULONG InitialSize)
{
    Result = _new(Tag, Allocator) UnicodeOutputStreamImp(InitialSize);
    if (Result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS    status = Result->Status();
    if (!NT_SUCCESS(status))
    {
        Result = nullptr;
    }

    return status;
}

__inline NTSTATUS
UnicodeOutputStreamImp::GuaranteeSpace(ULONG WCSizeNeeded)
{
    ULONG   sizeNeeded = (WCSizeNeeded * sizeof(WCHAR)) + _NextOffset;

    if (sizeNeeded > _Buffer->QuerySize())
    {
        NTSTATUS    status = _Buffer->SetSize(__max(sizeNeeded, (_Buffer->QuerySize() * 2)), TRUE);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }

    return STATUS_SUCCESS;
}

__inline void*
UnicodeOutputStreamImp::NextBufferAddr()
{
    return (UCHAR*)(_Buffer->GetBuffer()) + _NextOffset;
}

__inline NTSTATUS
UnicodeOutputStreamImp::AppendToBuffer(__in_ecount(WCSize) WCHAR* Source, __in ULONG WCSize)
{
    NTSTATUS    status = GuaranteeSpace(WCSize + 1);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    WCHAR*      base = (WCHAR*)NextBufferAddr();
    ULONGLONG       bytesToDo = WCSize * sizeof(WCHAR);
    KAssert(bytesToDo <= MAXULONG);

    KMemCpySafe(base, (ULONG)bytesToDo, Source, (ULONG)bytesToDo);
    _NextOffset += (ULONG)bytesToDo;
    base[WCSize] = 0;

    return STATUS_SUCCESS;
}

KBuffer::SPtr
UnicodeOutputStreamImp::GetBuffer()
{
    return _Buffer;
}

ULONG
UnicodeOutputStreamImp::GetStreamSize()
{
    return _NextOffset;
}



NTSTATUS
UnicodeOutputStreamImp::Put(__in WCHAR Value)
{
    NTSTATUS    status = GuaranteeSpace(1);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    KStringView         output((WCHAR*)NextBufferAddr(), 1);
    KFatal(output.AppendChar(Value));

    _NextOffset += (output.Length() * sizeof(WCHAR));
    KAssert(_NextOffset <= _Buffer->QuerySize());

    return STATUS_SUCCESS;
}

NTSTATUS
UnicodeOutputStreamImp::Put(__in LONG Value)
{
    NTSTATUS    status = GuaranteeSpace(12);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    KStringView         output((WCHAR*)NextBufferAddr(), 12);
    KFatal(output.FromLONG(Value));

    _NextOffset += (output.Length() * sizeof(WCHAR));
    KAssert(_NextOffset <= _Buffer->QuerySize());

    return STATUS_SUCCESS;
}

NTSTATUS
UnicodeOutputStreamImp::Put(__in ULONG Value)
{
    NTSTATUS    status = GuaranteeSpace(12);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    KStringView         output((WCHAR*)NextBufferAddr(), 12);
    KFatal(output.FromULONG(Value));

    _NextOffset += (output.Length() * sizeof(WCHAR));
    KAssert(_NextOffset <= _Buffer->QuerySize());
    return STATUS_SUCCESS;
}

NTSTATUS
UnicodeOutputStreamImp::Put(__in LONGLONG Value)
{
    NTSTATUS    status = GuaranteeSpace(21);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    KStringView         output((WCHAR*)NextBufferAddr(), 21);
    KFatal(output.FromLONGLONG(Value));

    _NextOffset += (output.Length() * sizeof(WCHAR));
    KAssert(_NextOffset <= _Buffer->QuerySize());
    return STATUS_SUCCESS;
}

NTSTATUS
UnicodeOutputStreamImp::Put(__in ULONGLONG Value)
{
    NTSTATUS    status = GuaranteeSpace(21);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    KStringView         output((WCHAR*)NextBufferAddr(), 21);
    KFatal(output.FromULONGLONG(Value));

    _NextOffset += (output.Length() * sizeof(WCHAR));
    KAssert(_NextOffset <= _Buffer->QuerySize());
    return STATUS_SUCCESS;
}

NTSTATUS
UnicodeOutputStreamImp::Put(__in GUID& Value)
{
    NTSTATUS    status = GuaranteeSpace(39);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    KStringView         output((WCHAR*)NextBufferAddr(), 39);
    KFatal(output.FromGUID(Value));

    _NextOffset += (output.Length() * sizeof(WCHAR));
    KAssert(_NextOffset <= _Buffer->QuerySize());
    return STATUS_SUCCESS;
}


NTSTATUS
UnicodeOutputStreamImp::Put(__in_z const WCHAR* Value)
{
    return AppendToBuffer(const_cast<WCHAR*>(Value), (ULONG)wcslen(Value));
}

NTSTATUS
UnicodeOutputStreamImp::Put(__in KUri::SPtr Value)
{
    KStringView view = *Value;
    return AppendToBuffer(PWCHAR(view), view.Length());
}


NTSTATUS
UnicodeOutputStreamImp::Put(__in KString::SPtr Value)
{
    KStringView view = *Value;
    return AppendToBuffer(PWCHAR(view), view.Length());
}

NTSTATUS
UnicodeOutputStreamImp::Put(__in KDateTime Value)
{
    KString::SPtr tmp;
    NTSTATUS status = Value.ToString(GetThisAllocator(), tmp);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return AppendToBuffer(PWCHAR(*tmp), tmp->Length());
}

NTSTATUS
UnicodeOutputStreamImp::Put(__in KDuration Value)
{
    KString::SPtr tmp;
    NTSTATUS status = Value.ToString(GetThisAllocator(), tmp);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return AppendToBuffer(PWCHAR(*tmp), tmp->Length());
}

NTSTATUS
UnicodeOutputStreamImp::Put(__in BOOLEAN Value)
{
    KVariant v(Value); // Use the string converter in KVariant for this
    KString::SPtr tmp;
    BOOLEAN b = v.ToString(GetThisAllocator(), tmp);
    if (!b)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return AppendToBuffer(PWCHAR(*tmp), tmp->Length());
}

//*** KDom static class implementation
NTSTATUS
KDom::CreateEmpty(__out KIMutableDomNode::SPtr& Result, __in KAllocator& Allocator, __in ULONG Tag)
{
    DomRoot::SPtr           result;
    NTSTATUS                status = DomRoot::Create(TRUE, result, Allocator, Tag);
    if (!NT_SUCCESS(status))
    {
        result = nullptr;
    }

    return result->GetApiObject(Result);
}

NTSTATUS
KDom::CreateFromBuffer(
    __in KBuffer::SPtr Source,
    __in KAllocator& Allocator,
    __in ULONG Tag,
    __out KIMutableDomNode::SPtr& Result)
{
    DomRoot::SPtr           result;
    NTSTATUS                status = DomRoot::Create(TRUE, result, Allocator, Tag);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = result->CreateFromBuffer(Source);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return result->GetApiObject(Result);
}

NTSTATUS
KDom::CreateReadOnlyFromBuffer(
    __in KBuffer::SPtr Source,
    __in KAllocator& Allocator,
    __in ULONG Tag,
    __out KIDomNode::SPtr& Result)
{
    DomRoot::SPtr           result;
    NTSTATUS                status = DomRoot::Create(FALSE, result, Allocator, Tag);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = result->CreateFromBuffer(Source);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return result->GetApiObject(Result);
}

NTSTATUS
KDom::CloneAs(__in const KIDomNode::SPtr& Source, __out KIMutableDomNode::SPtr& Result)
{
    DomNode::SPtr   result = nullptr;
    NTSTATUS        status = STATUS_SUCCESS;

    status = ((DomNodeApi*)(Source.RawPtr()))->GetBackingNode()->CloneAs(TRUE, result);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return result->GetApiObject(Result);
}

NTSTATUS
KDom::CloneAs(__in KIDomNode::SPtr& Source, __out KIDomNode::SPtr& Result)
{
    DomNode::SPtr   result = nullptr;
    NTSTATUS        status;

    status = ((DomNodeApi*)(Source.RawPtr()))->GetBackingNode()->CloneAs(FALSE, result);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return result->GetApiObject(Result);
}

NTSTATUS
KDom::CloneAs(__in KIMutableDomNode::SPtr& Source, __out KIMutableDomNode::SPtr& Result)
{
    KIDomNode::SPtr proxy = Source.RawPtr();
    return CloneAs(proxy, Result);
}


NTSTATUS
KDom::ToStream(__in const KIDomNode::SPtr& Source, __out KIOutputStream& Result)
{
    return ((DomNodeApi*)(Source.RawPtr()))->GetBackingNode()->ToStream(Result);
}

NTSTATUS
KDom::ToStream(__in KIMutableDomNode::SPtr& Source, __out KIOutputStream& Result)
{
    return ((DomNodeApi*)(Source.RawPtr()))->GetBackingNode()->ToStream(Result);
}

NTSTATUS
KDom::CreateOutputStream(
    __out KSharedPtr<UnicodeOutputStream>& Result,
    __in KAllocator& Allocator,
    __in ULONG Tag,
    __in ULONG InitialStreamSize)
{
    return UnicodeOutputStreamImp::Create(Result, Allocator, Tag, InitialStreamSize);
}


NTSTATUS
KDom::ToString(
    __in  KIDomNode::SPtr Source,
    __in  KAllocator& Allocator,
    __out KString::SPtr& String)
{
    if (!Source)
    {
        return STATUS_INVALID_PARAMETER;
    }

    KDom::UnicodeOutputStream::SPtr uioStream;
    NTSTATUS status = KDom::CreateOutputStream(uioStream, Allocator, KTL_TAG_TEST);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = KDom::ToStream(Source, *uioStream);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    KBuffer::SPtr uioStreamBuffer = uioStream->GetBuffer();
    if (uioStreamBuffer->QuerySize() == 0)
    {
        return STATUS_INTERNAL_ERROR;
    }

    status = uioStreamBuffer->SetSize(uioStream->GetStreamSize(), TRUE);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    KStringView xmlStr(PWCHAR(uioStreamBuffer->GetBuffer()), uioStreamBuffer->QuerySize()/2, uioStreamBuffer->QuerySize()/2);
    xmlStr.SkipBOM();

    String = KString::Create(xmlStr, Allocator);
    if (!String)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
KDom::ToString(
    __in  KIMutableDomNode::SPtr Source,
    __in  KAllocator& Allocator,
    __out KString::SPtr& String)
{
    KIDomNode::SPtr Root = Source.RawPtr();
    return ToString(Root, Allocator, String);
}



NTSTATUS
KDom::FromString(
    __in  KStringView& Src,
    __in  KAllocator& Allocator,
    __out KIMutableDomNode::SPtr& Dom)
{
    NTSTATUS status;
    if (Src.IsEmpty())
    {
        return STATUS_INVALID_PARAMETER;
    }

    KBuffer::SPtr buf;

    // ULONG allocLength = (Src.Length()+2) * 2; // Room for BOM + NULL
    HRESULT hr;
    ULONG allocLength;
    
    hr = ULongAdd(Src.Length() + 2, Src.Length(), &allocLength);
    KInvariant(SUCCEEDED(hr));

    hr = ULongMult(allocLength, 2, &allocLength);
    KInvariant(SUCCEEDED(hr));
    
    status = KBuffer::Create(allocLength, buf,  Allocator);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    KStringView bufferOverlay(PWCHAR(buf->GetBuffer()), allocLength / 2);
    bufferOverlay.EnsureBOM();
    bufferOverlay.SkipBOM();
    bufferOverlay.CopyFrom(Src);

    status = KDom::CreateFromBuffer(buf, Allocator, KTL_TAG_DOM, Dom);
    return status;
}


NTSTATUS
KDom::FromString(
    __in  KStringView& Src,
    __in  KAllocator& Allocator,
    __out KIDomNode::SPtr& Dom)
{
    KIMutableDomNode::SPtr tmp;
    NTSTATUS res = FromString(Src, Allocator, tmp);
    if (!NT_SUCCESS(res))
    {
        return res;
    }
    Dom = tmp.RawPtr();
    return STATUS_SUCCESS;
}

