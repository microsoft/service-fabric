/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    kpointer.h

    Description:
      Kernel Tempate Library (KTL) 'smart pointer' types

      KSharedPtr     Thread-safe smart pointer
      KUniquePtr     Single-owner smart poitner. Similar to unique_ptr in STL

    History:
      raymcc          16-Aug-2010         Initial version.

--*/

#pragma once

#include <kspinlock.h>

// isSharedClassImp detects the presence of KIsSharedClassImp in T.
template <class T, class = int>
struct isSharedClassImp
{
    static constexpr bool value = false;
};
template <class T>
struct isSharedClassImp<T, decltype((void)T::KIsSharedClassImp, 0)>
{
    static constexpr bool value = true;
};

// isSharedInterface detects the presence of KIsSharedInterface in T.
template <class T, class = int>
struct isSharedInterface
{
    static constexpr bool value = false;
};
template <class T>
struct isSharedInterface<T, decltype((void)T::KIsSharedInterface, 0)>
{
    static constexpr bool value = true;
};


// 4 cases to consider for SharedForwarder
// 1) KIsSharedClassImp is present by itself
// 2) KIsSharedInterface is present by itself
// 3) Both are present
// 4) Neither are present
// We only want special behavior for #2

// SharedForwarder is used by KSharedPtr to call the proper methods based on T,
// specifically if T has K_SHARED_INTERFACE
template <typename T, typename = void>
struct SharedForwarder
{
    static LONG AddRef(T& t)
    {
        return t.AddRef();
    }

    static LONG Release(T& t)
    {
        return t.Release();
    }

#if DBG
    static void AddTrackedSPtr(T& t, LIST_ENTRY* Linkage)
    {
        return t.AddTrackedSPtr(Linkage);
    }
    static void RemoveTrackedSPtr(T& t, LIST_ENTRY* Linkage)
    {
        return t.RemoveTrackedSPtr(Linkage);
    }
#endif

};

// specialization for isSharedInterface and NOT isSharedClassImp
template <typename T>
struct SharedForwarder<T, typename Ktl::enable_if<isSharedInterface<T>::value && !isSharedClassImp<T>::value>::type>
{
    static LONG AddRef(T& t)
    {
        return t.KIfAddRef();
    }

    static LONG Release(T& t)
    {
        return t.KIfRelease();
    }

#if DBG
    static void AddTrackedSPtr(T& t, LIST_ENTRY* Linkage)
    {
        return t.KIfAddTrackedSPtr(Linkage);
    }
    static void RemoveTrackedSPtr(T& t, LIST_ENTRY* Linkage)
    {
        return t.KIfRemoveTrackedSPtr(Linkage);
}
#endif
};

//
// KSharedPtr Definition
//
template <class T>
class KSharedPtr
{
    #if !DBG
        KIs_BlittableType();
        KIs_BlittableType_DefaultIsZero();
    #endif

public:
    //
    // Default constructor
    //
    NOFAIL KSharedPtr();

    //
    // Constructor which takes a real pointer
    //
    // Parameters:
    //
    //  Obj    A pointer to the object to be bound.  This must support AddRef and Release.
    //
    NOFAIL KSharedPtr(
        __in_opt T* Obj
        );

    //
    // Copy constructor
    //
    // This cannot fail, since it doesn't allocate
    //
    NOFAIL KSharedPtr(
        __in KSharedPtr const & Src
        );

    //
    // Move constructor
    //
    // This cannot fail, since it doesn't allocate
    //
    NOFAIL KSharedPtr(
        __in KSharedPtr&& Src
        );

    //
    //  Destructor
    //
   ~KSharedPtr();

    //
    // Assignment operator
    //
    NOFAIL KSharedPtr& operator=(
        __in KSharedPtr const & Src
        );

    //
    // Move assignment operator
    //
    NOFAIL KSharedPtr& operator=(
        __in KSharedPtr&& Src
        );

    //
    // Assignment operator from a raw pointer
    //
    NOFAIL KSharedPtr& operator=(
        __in_opt T* Src
        );

    //
    // Dereference
    //
    T& operator*() const;

    //
    // Field dereference
    //
    T* operator->() const;

    //
    // Explict 'get-pointer' method.  No automatic conversion.
    //
    T* RawPtr() const;

    //  Reset
    //
    //  A clean Release() and detach from the underlying object.
    //
    //  Returns: TRUE iif the Reset() resulted in the underlying object being destructed.
    //
    BOOLEAN Reset();

    //  Detach
    //
    //  Detaches from the underlying object without changing the ref count.
    //  This is used in places where the raw pointer has to be extracted without the object
    //  being released so that a KSharedPtr can be reattached later.
    //
    T* Detach();


    // Attach
    //
    // Attaches to an object without changing the ref count.  Note that the constructor
    // does an AddRef() at construct-time.
    //
    void Attach(
        __in T* Src
        );

    // Operator ==
    //
    // Tests two pointers for equality.  This tests to see if the address of the underlying object
    // is the same.
    //
    BOOLEAN operator ==(
        __in KSharedPtr const & Comparand
        )  const;

    // Operator ==
    //
    // Tests this smart pointer against a raw pointer for equality.  This tests to see if the address of
    // the underlying object is the same.
    //
    BOOLEAN operator ==(
        __in_opt T* const Comparand
        ) const;

    // Operator !=
    //
    // Tests two pointers for inequality.  This tests to see if the address of the underlying object
    // is different.
    //
    BOOLEAN operator !=(
        __in KSharedPtr const & Comparand
        ) const;

    // Operator !=
    //
    // Tests this smart pointer against a raw pointer for inequality.  This tests to see if the address of
    // the underlying object is different.
    //
    BOOLEAN operator !=(
        __in_opt T* const Comparand
        ) const;

    // Operator BOOLEAN
    //
    // Used to test for an empty/NULL shared pointer.
    //
    operator BOOLEAN() const;

    // DownCast
    //
    // Downcasts this shared ptr to a pointer to its child type.
    //
    // Example:
    //
    // KSharedPtr<BaseType> baseSPtr;
    //
    // ...  // baseSPtr holds a pointer to a DerivedType object
    //
    // DerivedType* derivedPtr = baseSPtr.DownCast<DerivedType>();
    //
    template<class DerivedType>
    DerivedType* DownCast();

protected:
    T* _Proxy;

private:
    //* Inline wrapper methods to support shared-interfaces

    __forceinline LONG AddRef()
    {
        typedef typename Ktl::RemoveConst<T>::type NonConstT;
        return SharedForwarder<NonConstT>::AddRef((NonConstT&)*_Proxy);
    }

    __forceinline LONG Release()
    {
        typedef typename Ktl::RemoveConst<T>::type NonConstT;
        return SharedForwarder<NonConstT>::Release((NonConstT&)*_Proxy);
    }

#if DBG
    __forceinline void AddTrackedSPtr(LIST_ENTRY* Linkage)
    {
        typedef typename Ktl::RemoveConst<T>::type NonConstT;
        SharedForwarder<NonConstT>::AddTrackedSPtr((NonConstT&)*_Proxy, Linkage);
    }

    __forceinline void RemoveTrackedSPtr(LIST_ENTRY* Linkage)
    {
        typedef typename Ktl::RemoveConst<T>::type NonConstT;
        SharedForwarder<NonConstT>::RemoveTrackedSPtr((NonConstT&)*_Proxy, Linkage);
    }

    //
    // _AllocationAddress is the return address of the code segment
    // that invokes a KSharedPtr<> method to acquire a ref count.
    //
    // This address can be used in debugger to pinpoint where the ref count
    // is acquired (e.g. file name, line number etc. with symbol support).
    // It is useful for tracking ref count leaks.
    //

    PVOID _AllocationAddress;
    LIST_ENTRY _Linkage;

    #define ADD_REF() \
        AddRef(),\
        _AllocationAddress = _ReturnAddress(), \
        AddTrackedSPtr(&_Linkage)

    #define RELEASE() \
        RemoveTrackedSPtr(&_Linkage), \
        _AllocationAddress = nullptr, \
        Release()

#else

    #define ADD_REF() AddRef()
    #define RELEASE() Release()

#endif
};


//
// KSharedPtr Implementation
//
template <class T> inline
KSharedPtr<T>::KSharedPtr()
{
    _Proxy = nullptr;

#if DBG
    _Linkage.Flink = _Linkage.Blink = nullptr;
    _AllocationAddress = nullptr;
#endif
}

template <class T> inline
KSharedPtr<T>::KSharedPtr(
    __in_opt T* Obj
    )
{
    _Proxy = Obj;
#if DBG
    _Linkage.Flink = _Linkage.Blink = nullptr;
    _AllocationAddress = nullptr;
#endif
    if (_Proxy)
    {
        ADD_REF();
    }
}

template <class T> inline
KSharedPtr<T>::KSharedPtr(
    __in KSharedPtr const & Src
    )
{
    _Proxy = Src._Proxy;

#if DBG
    _Linkage.Flink = _Linkage.Blink = nullptr;
    _AllocationAddress = nullptr;
#endif
    if (_Proxy)
    {
        ADD_REF();
    }
}

// Move constructor
template <class T> inline
KSharedPtr<T>::KSharedPtr(
    __in KSharedPtr&& Src
    )
{

    _Proxy = Src._Proxy;
#if DBG
    if (_Proxy)
    {
         _AllocationAddress = Src._AllocationAddress;
         Src.RemoveTrackedSPtr(&Src._Linkage);
         AddTrackedSPtr(&_Linkage);
    } else {
        _Linkage.Flink = _Linkage.Blink = nullptr;
        _AllocationAddress = nullptr;
    }
#endif
    Src._Proxy = nullptr;

}

template <class T> inline
KSharedPtr<T>::~KSharedPtr()
{
    if (_Proxy)
    {
        RELEASE();
    }
}

template <class T> inline
KSharedPtr<typename T>& KSharedPtr<T>::operator=(
    __in KSharedPtr const & Src
    )
{
    if (&Src != this)
    {
        if (_Proxy)
        {
            RELEASE();
        }
        _Proxy = Src._Proxy;
        if (_Proxy)
        {
            ADD_REF();
        }
    }

    return *this;
}

template <class T> inline
KSharedPtr<typename T>& KSharedPtr<T>::operator=(
    __in_opt T* Src
    )
{
  if (_Proxy != Src)
  {
        if (_Proxy)
        {
            RELEASE();
        }
        _Proxy = Src;
        if (_Proxy)
        {
            ADD_REF();
        }
  }

    return *this;
}

// Move assignment
template <class T> inline
KSharedPtr<typename T>& KSharedPtr<T>::operator=(
    __in KSharedPtr&& Src
    )
{
    if (&Src != this)
    {
        if (_Proxy)
        {
            RELEASE();
        }
        _Proxy = Src._Proxy;
#if DBG
        if (_Proxy)
        {
             _AllocationAddress = Src._AllocationAddress;
             Src.RemoveTrackedSPtr(&Src._Linkage);
             AddTrackedSPtr(&_Linkage);
        }
#endif
        Src._Proxy = nullptr;
    }
    return *this;
}

template <class T> inline
T& KSharedPtr<T>::operator*() const
{
#if !defined(PLATFORM_UNIX)
    // Do not need this check on Linux because the Clang compiler does this check already.
    KAssert(_Proxy != nullptr);
#endif
    return *_Proxy;
}

template <class T> inline
T* KSharedPtr<T>::operator->() const
{
    return _Proxy;
}

template <class T> inline
T* KSharedPtr<T>::RawPtr() const
{
    return _Proxy;
}

template <class T> inline
void KSharedPtr<T>::Attach(
    __in T* Src
    )
{
    if (_Proxy)
    {
        RELEASE();
    }
    _Proxy = Src;

#if DBG
    if (_Proxy)
    {
        _AllocationAddress = _ReturnAddress();
        AddTrackedSPtr(&_Linkage);
    }
#endif
}

template <class T> inline
T* KSharedPtr<T>::Detach()
{
    T* Obj = _Proxy;

#if DBG
    if (_Proxy)
    {
        RemoveTrackedSPtr(&_Linkage);
        _AllocationAddress = nullptr;
    }
#endif

    _Proxy = nullptr;
    return(Obj);
}

template <class T> inline
BOOLEAN KSharedPtr<T>::Reset()
{
    BOOLEAN wasDestructed = FALSE;

    if (_Proxy)
    {
        wasDestructed = (RELEASE() == 0);
    }
    _Proxy = nullptr;
    return wasDestructed;
}

template <class T> inline
BOOLEAN KSharedPtr<T>::operator==(
    __in KSharedPtr const & Comparand
    ) const
{
    return Comparand._Proxy == _Proxy ? TRUE: FALSE;
}

template <class T> inline
BOOLEAN KSharedPtr<T>::operator==(
    __in_opt T* const Comparand
    ) const
{
    return Comparand == _Proxy ? TRUE: FALSE;
}

template <class T> inline
BOOLEAN KSharedPtr<T>::operator!=(
    __in KSharedPtr const & Comparand
    ) const
{
    return !(*this == Comparand);
}

template <class T> inline
BOOLEAN KSharedPtr<T>::operator!=(
    __in_opt T* const Comparand
    ) const
{
    return !(*this == Comparand);
}

template <class T> inline
KSharedPtr<T>::operator BOOLEAN() const
{
    return _Proxy ? TRUE : FALSE;
}

template <class T> template <class DerivedType> inline
DerivedType* KSharedPtr<T>::DownCast()
{
    return _Proxy ? static_cast<DerivedType*>(_Proxy) : nullptr;
}


//
// KUniquePtr Default Deleter Definition
//
template <class T>
struct DefaultDeleter
{
    static void Delete(T * pointer)
    {
        if (pointer != nullptr)
        {
            _delete(pointer);
        }
    }
};

//
// KUniquePtr Array Deleter Definition
//
template <class T>
struct ArrayDeleter
{
    static void Delete(T * pointer)
    {
        if (pointer != nullptr)
        {
            _deleteArray(pointer);
        }
    }
};

//
// KUniquePtr non-ownership deleter definition.  Allows unique-ptr semantics on externally-owned memory.
// Will failfast if the deleter is called on a non-null ptr.  Before allowing a KUniquePtr using this deleter to
// destruct, the KUniquePtr must be explicitly detached.
//
template <class T>
struct FailFastIfNonNullDeleter
{
    static void Delete(T * pointer)
    {
        KInvariant(pointer == nullptr);
    }
};

//
// KUniquePtr Definition
//
template <class T, class Deleter = DefaultDeleter<T>>
class KUniquePtr
{
    K_DENY_COPY(KUniquePtr);
    //K_NO_DYNAMIC_ALLOCATE;
    void* operator&();

public:
    //
    // Default constructor
    //
    NOFAIL KUniquePtr();

    // Constructor
    //
    // Acquires the underlying pointer and deletes it when this object goes out of scope.
    //
    // Parameters:
    //  Obj     A raw pointer to the object to be 'acquired'.  All further use of this pointer should
    //          be via KUniquePtr.
    //
    NOFAIL KUniquePtr(
        __in T* Obj
        );

    //
    // Move constructor
    //
    // This cannot fail, since it doesn't allocate
    //
    NOFAIL KUniquePtr(
        __in KUniquePtr&& Src
        );

    //
    // Move assignment operator
    //
    NOFAIL KUniquePtr& operator=(
        __in KUniquePtr&& Src
        );

    // Dereference
    //
    T& operator*() const;

    // Field dereference
    //
    T* operator->() const;

    // Conversion
    //
    operator T*();

    //
    // Assignment operator from a raw pointer.
    //
    NOFAIL KUniquePtr& operator=(
        __in_opt T* Src
        );

    // operator BOOLEAN
    //
    // Used to test for a null pointer.
    //
    operator BOOLEAN() const;

    // Detach
    //
    // This directly detaches KUniquePtr from its underlying pointer.
    // At detruct, KUniquePtr will not delete the object.
    //
    T* Detach();

    // Operator ==
    //
    // Tests two pointers for equality.  This tests to see if the address of the underlying object
    // is the same.
    //
    BOOLEAN operator ==(
        __in KUniquePtr const & Comparand
        ) const;

    // Operator ==
    //
    // Tests this unique pointer against a raw pointer for equality.  This tests to see if the address of
    // the underlying object is the same.
    //
    BOOLEAN operator ==(
        __in_opt T* const Comparand
        ) const;

    // Operator !=
    //
    // Tests two pointers for inequality.  This tests to see if the address of the underlying object
    // is different.
    //
    BOOLEAN operator !=(
        __in KUniquePtr const & Comparand
        ) const;

    // Operator !=
    //
    // Tests this smart pointer against a raw pointer for inequality.  This tests to see if the address of
    // the underlying object is different.
    //
    BOOLEAN operator !=(
        __in_opt T* const Comparand
        ) const;

    // Destructor
    //
    // This deletes the underlying object during destruction.
    //
    ~KUniquePtr();

private:

    // MoveUniquePtr
    //
    // Transfers one KUniquePtr to another KUniquePtr.  Note that is distinct from copy construction & assignment
    // in that the old object 'loses' the pointer.
    //
    void MoveUniquePtr(
        __in KUniquePtr& Src
        );

    T* _Obj;
};



// KUniquePtr Implementation
template<class T, class Deleter> inline
KUniquePtr<T, Deleter>::KUniquePtr(
    )
    : _Obj(nullptr)
{
}

template<class T, class Deleter> inline
KUniquePtr<T, Deleter>::KUniquePtr(
    __in T* Obj
    )
    : _Obj(Obj)
{
}

// Move constructor
template<class T, class Deleter> inline
KUniquePtr<T, Deleter>::KUniquePtr(
    __in KUniquePtr&& Src
    )
    : _Obj(nullptr)
{
    MoveUniquePtr(Src);
}

// Move assignment
template<class T, class Deleter> inline
KUniquePtr<T, Deleter>& KUniquePtr<T, Deleter>::operator=(
    __in KUniquePtr&& Src
    )
{
    MoveUniquePtr(Src);
    return *this;
}

template<class T, class Deleter> inline
void KUniquePtr<T, Deleter>::MoveUniquePtr(
    __in KUniquePtr& Src
    )
{
    KInvariant(!_Obj || _Obj != Src._Obj);

    Deleter::Delete(_Obj);

    _Obj = Src._Obj;
    Src._Obj = nullptr;
}

template<class T, class Deleter> inline
T& KUniquePtr<T, Deleter>::operator *() const
{
    return *_Obj;
}

template<class T, class Deleter> inline
T* KUniquePtr<T, Deleter>::operator->() const
{
    return _Obj;
}

template<class T, class Deleter> inline
KUniquePtr<T, Deleter>::operator T*()
{
    return _Obj;
}

template<class T, class Deleter> inline
KUniquePtr<T, Deleter>& KUniquePtr<T, Deleter>::operator=(
    __in_opt T* Src
    )
{
    KInvariant(!_Obj || _Obj != Src);

    Deleter::Delete(_Obj);
    _Obj = Src;
    return *this;
}

template<class T, class Deleter> inline
KUniquePtr<T, Deleter>::operator BOOLEAN() const
{
    return _Obj ? TRUE : FALSE;
}

template<class T, class Deleter> inline
T* KUniquePtr<T, Deleter>::Detach()
{
    T *Obj;

    Obj = _Obj;
    _Obj = nullptr;
    return(Obj);
}

template<class T, class Deleter> inline
BOOLEAN KUniquePtr<T, Deleter>::operator ==(
    __in KUniquePtr const & Comparand
    ) const
{
    return (_Obj == Comparand._Obj) ? TRUE : FALSE;
}

template<class T, class Deleter> inline
BOOLEAN KUniquePtr<T, Deleter>::operator !=(
    __in KUniquePtr const & Comparand
    ) const
{
    return !(*this == Comparand);
}

template<class T, class Deleter> inline
BOOLEAN KUniquePtr<T, Deleter>::operator ==(
    __in_opt T* const Comparand
    ) const
{
    return (_Obj == Comparand) ? TRUE : FALSE;
}

template<class T, class Deleter> inline
BOOLEAN KUniquePtr<T, Deleter>::operator !=(
    __in_opt T* const Comparand
    ) const
{
    return !(*this == Comparand);
}

template<class T, class Deleter> inline
KUniquePtr<T, Deleter>::~KUniquePtr()
{
    Deleter::Delete(_Obj);
}

//
// This is the concrete base class of all KShared<> types.
// Any KShared<T> ptr can be casted to a ptr to this base class.
//
// Example:
// class Foo : public KShared<Foo>
// {
//      ...
// }
//
// class Bar : public KShared<Bar>
// {
//      ...
// }
//
// KSharedBase::SPtr baseSPtr;
// baseSPtr = fooSPtr.RawPtr();
// baseSPtr = barSPtr.RawPtr();
//

template <class T>
class KWeakRefType;

class KSharedBase
{

public:

    typedef KSharedPtr<KSharedBase> SPtr;
    typedef KSharedPtr<KSharedBase const> CSPtr;

    LONG AddRef()
    {
        return InterlockedIncrement(&_RefCount);
    }

    LONG Release()
    {
        LONG result = InterlockedDecrement(&_RefCount);
        KAssert(result >= 0);
        if (result == 0)
        {
            OnDelete();
        }
        return result;
    }

    LONG AddRefs(ULONG ToAdd)
    {
        ULONG oldValue = 0;
        ULONG newValue = 0;
        ULONG resultValue = 0;
        do
        {
            oldValue = _RefCount;
            KInvariant(((ULONGLONG)ToAdd + oldValue) <= MAXULONG);
            newValue = oldValue + ToAdd;

            resultValue = _InterlockedCompareExchange(&_RefCount, newValue, oldValue);
        } while (resultValue != oldValue);

        return newValue;
    }

    LONG ReleaseRefs(ULONG ToRelease)
    {
        ULONG oldValue = 0;
        ULONG newValue = 0;
        ULONG resultValue = 0;
        do
        {
            oldValue = _RefCount;
            KInvariant(oldValue >= ToRelease);
            newValue = oldValue - ToRelease;

            resultValue = _InterlockedCompareExchange(&_RefCount, newValue, oldValue);
        } while (resultValue != oldValue);

        if (newValue == 0)
        {
            OnDelete();
        }
        return newValue;
    }

    BOOLEAN IsUnique() const
    {
        return (_RefCount <= 1);
    }

    LONG GetRefCount() { return _RefCount; }

#if DBG

    VOID AddTrackedSPtr(__inout PLIST_ENTRY SptrLinkage)
    {
        _ListLock.Acquire();
        InsertTailList(&_SharedPtrList, SptrLinkage);
        _ListLock.Release();
    }

    VOID RemoveTrackedSPtr(__inout PLIST_ENTRY SptrLinkage)
    {
        _ListLock.Acquire();
        RemoveEntryList(SptrLinkage);
        _ListLock.Release();

        SptrLinkage->Flink = SptrLinkage->Blink = nullptr;
    }

#endif

protected:

    //
    // Hide ctor and dtor so that standalone KSharedBase cannot be constructed or destroyed.
    //
    NOFAIL KSharedBase() : _RefCount(0)
    {
        #if DBG
        InitializeListHead(&_SharedPtrList);
        #endif
    }

    ~KSharedBase() { KAssert(!_RefCount); }

    virtual VOID OnDelete() = 0;

    template <class T> friend class KWeakRefType;
    volatile LONG   _RefCount;

    #if DBG

    //
    // The list of KSharedPtr objects that reference this object.
    //
    LIST_ENTRY _SharedPtrList;
    KSpinLock _ListLock;

    #endif
};

//
// A KShared<T> object has a shared ownership semantic and is intended
// to be used with KSharedPtr<T> only. It should be inherited using the CRTP pattern.
// A standalone KShared<> object cannot be constructed.
//
// To declare a class T as sharable, follow these steps:
//  (1) Inherit from KShared<T>, e.g. class T : public KShared<T>
//  (2) Declare K_FORCE_SHARED(T) or K_FORCE_SHARED_WITH_INHERITANCE(T) macro in the class declaration.
//  (3) Define one or more factory methods. A factory method should look like the following:
//          KSharedPtr<T> CreateShared(KAllocator& Allocator, ULONG Tag, ...)
//  The ... list is a set of parameters that an user can pass to the factory method.
//  For example, these parameters are passed to the constructor of T.
//

template <class T>
class KShared : public KSharedBase
{
public:
    KAllocator&
    GetThisAllocator() const
    {
        return KAllocatorSupport::GetAllocator(static_cast<T const *>(this));
    }

    KtlSystem&
    GetThisKtlSystem() const
    {
        return KAllocatorSupport::GetAllocator(static_cast<T const *>(this)).GetKtlSystem();
    }

    ULONG
    GetThisAllocationTag() const
    {
        return KAllocatorSupport::GetAllocationTag(static_cast<T const *>(this));
    }

protected:
    friend class KSharedMixInBase;

    //
    // Hide ctor and dtor so that standalone KShared<> cannot be constructed or destroyed.
    //
    NOFAIL KShared()
    {
        // No KShared<> should be ctors w/o a KAllocator
        KAssert((&GetThisAllocator()) != nullptr);
    }

    ~KShared() {}

private:

    VOID OnDelete()
    {
        ::_delete<T>(static_cast<T*>(this));
    }
};

//
// This macro should be applied to a derivation of KShared<class_name>
// that is NOT intended to be inherited.
//
// By using this macro, an instance of class "class_name"
// can only be constructed using a factory method. It cannot be
// constructed on the stack, or created using the new operator.
// It cannot be destroyed using the delete operator either.
//
// Class "class_name" cannot be inherited.
//

// VC doesn't support ::_delete right now, so use this to workaround
#if defined(PLATFORM_UNIX)
    #define K_DELETE ::_delete
#else
    #define K_DELETE _delete
#endif

#define K_FORCE_SHARED(class_name) \
    private: \
        friend KShared<class_name>; \
        friend VOID K_DELETE<class_name>(class_name* HeapObj); \
        class_name(); \
        class_name(const class_name&); \
        class_name& operator = (class_name const&); \
        ~class_name(); \
    public: \
        const static BOOLEAN KIsSharedClassImp = TRUE;\
        typedef KSharedPtr<class_name> SPtr; \
        typedef KSharedPtr<class_name const> CSPtr; \
    private:

//
// This macro should be applied to a derivation of KShared<class_name>
// that is intended to be inherited.
// It prevents an instance of class "class_name" from being constructed on the stack.
//
#define K_FORCE_SHARED_WITH_INHERITANCE(class_name) \
    protected: \
        friend KShared<class_name>; \
        friend VOID K_DELETE<class_name>(class_name* HeapObj); \
        class_name(); \
        class_name(const class_name&); \
        class_name& operator = (class_name const&); \
        virtual ~class_name(); \
    public: \
        const static BOOLEAN KIsSharedClassImp = TRUE;\
        typedef KSharedPtr<class_name> SPtr; \
        typedef KSharedPtr<class_name const> CSPtr; \
    private:

//
//  Default (no dependencies and default constructor) Create() declaration for a KShared derivation
//
#define KDeclareDefaultCreate(className, outputArgName, defaultTag) \
    static NTSTATUS \
    Create( \
        __out className::SPtr& outputArgName, \
        __in KAllocator& Allocator, \
        __in ULONG AllocationTag = defaultTag \
        );

//
//  Default (no dependencies and default constructor) Create() definition for a KShared derivation
//
#define KDefineDefaultCreate(className, outputArgName) \
    NTSTATUS \
    className::Create( \
        __out className::SPtr& outputArgName, \
        __in KAllocator& Allocator, \
        __in ULONG AllocationTag \
        ) \
    { \
        NTSTATUS status; \
        className::SPtr output; \
        \
        outputArgName = nullptr; \
        \
        output = _new(AllocationTag, Allocator) className(); \
        \
        if (! output) \
        { \
            status = STATUS_INSUFFICIENT_RESOURCES; \
            KTraceOutOfMemory(0, status, nullptr, 0, 0); \
            return status; \
        } \
        \
        if (! NT_SUCCESS(output->Status())) \
        { \
            return output->Status(); \
        } \
        \
        outputArgName = Ktl::Move(output); \
        return STATUS_SUCCESS; \
    }

//
//  Create() declaration with a single argument passed to the constructor for a KShared derivation
//
#define KDeclareSingleArgumentCreate(outputClassName, implementationClassName, outputArgName, defaultTag, argType, argName) \
    static NTSTATUS \
    Create( \
        __out outputClassName##::SPtr& outputArgName, \
        __in argType argName, \
        __in KAllocator& Allocator, \
        __in ULONG AllocationTag = defaultTag \
        );

//
//  Create() definition with a single argument passed to the constructor for a KShared derivation
//
#define KDefineSingleArgumentCreate(outputClassName, implementationClassName, outputArgName, argType, argName) \
    NTSTATUS \
    implementationClassName##::Create( \
        __out outputClassName##::SPtr& outputArgName, \
        __in argType argName, \
        __in KAllocator& Allocator, \
        __in ULONG AllocationTag\
        ) \
    { \
        NTSTATUS status; \
        outputClassName##::SPtr output; \
        \
        outputArgName = nullptr; \
        \
        output = _new(AllocationTag, Allocator) implementationClassName##(argName); \
        \
        if (! output) \
        { \
            status = STATUS_INSUFFICIENT_RESOURCES; \
            KTraceOutOfMemory(0, status, nullptr, 0, 0); \
            return status; \
        } \
        \
        if (! NT_SUCCESS(output->Status())) \
        { \
            return output->Status(); \
        } \
        \
        outputArgName = Ktl::Move(output); \
        return STATUS_SUCCESS; \
    }


//* KSPtrBroker - Safe SPtr "server" that only uses two ICE128s on fast path
template<typename TShared>
class KSPtrBroker
{
    K_DENY_COPY(KSPtrBroker);

    // no move support at this time
    KSPtrBroker(KSPtrBroker&&) = delete;
    KSPtrBroker& operator=(KSPtrBroker&&) = delete;

public:
    KSPtrBroker() {}

    ~KSPtrBroker()
    {
        Put(nullptr);
    }

    //* Store a thread-safe reference value to the supplied KShared<> derivation. Once 
    //  set via a Put() method, the Get() method may be used to retrieve safe copies of
    //  the value set. Racing Put()s and Get()s are guarenteed to be thread safe; Get()s
    //  will always return a valid SPtr value that was present at some point in time.
    //
    //  Parameters:
    //      NewCurrent - reference to be stored OR nullptr to release the current reference
    //      AllocBlkSize - optional - defaults to 100. This is the number of batched reference
    //                     counts to acquire from NewCurrent during the safe store.             
    //  
    void Put(__in_opt TShared* const NewCurrent, __in_opt LONG AllocBlkSize = 100);
    void Put(__in KSharedPtr<TShared>& NewCurrent, __in_opt LONG AllocBlkSize = 100);
    void Put(__in KSharedPtr<TShared>&& NewCurrent, __in_opt LONG AllocBlkSize = 100);

    //* Retrieve a safe and consistent snapshot SPtr for a currently stored KShared<> derivation object.
    //  
    //  Parameters:
    //      AllocBlkSize - optional - defaults to 100. This is the number of batched reference
    //                     counts to acquire from the current KShared during the safe Get if the count 
    //                     runs low.             
    //
    KSharedPtr<TShared> Get(__in_opt LONG AllocBlkSize = 100);

public:
    #if UnitTest_KSPtrBroker
        static LONG volatile Test_AtomicRead128SpinCount;
        static LONG volatile Test_PutSpinCount;
        static LONG volatile Test_GetSpinCount;
        static LONG volatile Test_GetBackoutCount;
        static LONG volatile Test_GetHoldAllocateCount;
    #endif

    struct Int128
    {
        volatile LONG64 _Value[2];
    };

    struct BatchSPtr
    {
        ULONG       _OwnedRefs;
        TShared*    _Obj;
    };

    union AtomicRef
    {
        Int128      _Raw;
        BatchSPtr   _BSPtr;

        //* The intel x64 processor only support one atomic 128-bit memory instruction - ICE128.
        //  This primitive uses that ICE128 to effect an atomic 128 memory read.
        //
        __forceinline static void AtomicRead128(__in const AtomicRef& From, __out AtomicRef& Dest)
        {
            #if UnitTest_KSPtrBroker
            int         spinCount = 0;
            #endif

            do
            {
                #if UnitTest_KSPtrBroker
                    spinCount++;
                #endif

                // Snap 128 bits - potentially inconsistent read
                Dest._Raw._Value[0] = From._Raw._Value[0];
                Dest._Raw._Value[1] = From._Raw._Value[1];

            // Use ICE128 in a non-modifing form to detect inconsistent reads from above
            } while (InterlockedCompareExchange128(
                const_cast<volatile LONG64*>(&From._Raw._Value[0]),     // Dest
                Dest._Raw._Value[1], Dest._Raw._Value[0],               // New value
                (LONG64*)&Dest._Raw._Value[0]) == 0);                   // Comp / result value

            #if UnitTest_KSPtrBroker
            if (spinCount > 1)
            {
                InterlockedIncrement(&Test_AtomicRead128SpinCount);
            }
            #endif
        }

        __forceinline AtomicRef()
        {
            _Raw._Value[0] = 0;
            _Raw._Value[1] = 0;
        }

        __forceinline AtomicRef(const AtomicRef& Src)
        {
            AtomicRead128(Src, *this);
        }
        __forceinline AtomicRef& operator=(const AtomicRef& Src)
        {
            AtomicRead128(Src, *this);
            return *this;
        }

        __forceinline void UnsafeCopy(const AtomicRef& Src)
        {
            _Raw._Value[0] = Src._Raw._Value[0];
            _Raw._Value[1] = Src._Raw._Value[1];
        }
    };
    static_assert(sizeof(Int128)    == 16, "Int128 size mismatch");
    static_assert(sizeof(BatchSPtr) == 16, "BatchSPtr size mismatch");
    static_assert(sizeof(AtomicRef) == 16, "AtomicRef size mismatch");

private:
    __declspec(align(16))
    AtomicRef      _Current;
};

#if UnitTest_KSPtrBroker
    template<typename TShared>
    LONG volatile KSPtrBroker<TShared>::Test_AtomicRead128SpinCount = 0;

    template<typename TShared>
    LONG volatile KSPtrBroker<TShared>::Test_PutSpinCount = 0;

    template<typename TShared>
    LONG volatile KSPtrBroker<TShared>::Test_GetSpinCount = 0;

    template<typename TShared>
    LONG volatile KSPtrBroker<TShared>::Test_GetBackoutCount = 0;

    template<typename TShared>
    LONG volatile KSPtrBroker<TShared>::Test_GetHoldAllocateCount = 0;
#endif


template<typename TShared>
void KSPtrBroker<TShared>::Put(__in_opt TShared* const NewCurrent, __in_opt LONG AllocBlkSize)
{
    AllocBlkSize = __max(AllocBlkSize, 2);      // deadlock avoidance

    AtomicRef   newValue;
    AtomicRef   currValue;

    // Compute new current value and batch allocate refs if not clearing current value
    newValue._BSPtr._OwnedRefs = (NewCurrent == nullptr) ? 0 : AllocBlkSize;
    newValue._BSPtr._Obj = NewCurrent;

    if (NewCurrent != nullptr)
    {
        // Acquire a batch of refs for NewCurrent
        NewCurrent->AddRefs(AllocBlkSize);
    }

    #if UnitTest_KSPtrBroker
    int         spinCount = 0;
    #endif

    do
    {
        AtomicRef::AtomicRead128(_Current, currValue);          // snap consistent current value

        #if UnitTest_KSPtrBroker
            spinCount++;
        #endif
    } while (InterlockedCompareExchange128(
                const_cast<volatile LONG64*>(&_Current._Raw._Value[0]),     // Dest
                newValue._Raw._Value[1], newValue._Raw._Value[0],           // New value
                (LONG64*)&currValue._Raw._Value[0]) == 0);                  // Comp / result value

    if (currValue._BSPtr._Obj != nullptr)
    {
        // The old current had refs - give them back
        currValue._BSPtr._Obj->ReleaseRefs(currValue._BSPtr._OwnedRefs);
    }

    #if UnitTest_KSPtrBroker
        if (spinCount > 1)
        {
            InterlockedIncrement(&Test_PutSpinCount);
        }
    #endif
}

template<typename TShared>
void KSPtrBroker<TShared>::Put(__in KSharedPtr<TShared>&& NewCurrent, __in_opt LONG AllocBlkSize)
{
    KSharedPtr<TShared>   newCurrent = Ktl::Move(NewCurrent);
    Put(newCurrent.RawPtr(), AllocBlkSize + 1);             // Include any ref-count being Moved in  
    newCurrent.Detach();
}

template<typename TShared>
void KSPtrBroker<TShared>::Put(__in KSharedPtr<TShared>& NewCurrent, __in_opt LONG AllocBlkSize)
{
    Put(NewCurrent.RawPtr(), AllocBlkSize);
}

template<typename TShared>
KSharedPtr<TShared> KSPtrBroker<TShared>::Get(__in_opt LONG AllocBlkSize)
{
    AllocBlkSize = __max(AllocBlkSize, 2);      // deadlock avoidance

    AtomicRef   currValue;
    AtomicRef   newValue;

    #if UnitTest_KSPtrBroker
        int     holdCount = 0;
        int     spinCount = 0;
    #endif

    do
    {
        // Wait while valid ptr and other Get() thread is rechanging _OwnedRefs
        #if UnitTest_KSPtrBroker
        holdCount = 0;
        #endif

        do
        {
            AtomicRef::AtomicRead128(_Current, currValue);          // snap consistent current value
            KAssert((currValue._BSPtr._Obj == nullptr) || (currValue._BSPtr._OwnedRefs != 0));

            #if UnitTest_KSPtrBroker
                holdCount++;
            #endif
        } while ((currValue._BSPtr._Obj != nullptr) && (currValue._BSPtr._OwnedRefs == 1));

        #if UnitTest_KSPtrBroker
            spinCount++;
            if (holdCount > 1)
            {
                InterlockedIncrement(&Test_GetHoldAllocateCount);
            }
        #endif

        // currValue holds atomic read of _Current
        if (currValue._BSPtr._Obj == nullptr)
        {
            KAssert(currValue._BSPtr._OwnedRefs == 0);

            // Current state is not ref'ing an KShared<TShared>
            return {};      // return SPtr(nullptr)
        }

        // compute new value
        newValue.UnsafeCopy(currValue);     // NOTE: Extra read in debug build - retail fixes it
        newValue._BSPtr._OwnedRefs--;       // The ref we want to take

    } while (InterlockedCompareExchange128(
        const_cast<volatile LONG64*>(&_Current._Raw._Value[0]),     // Dest
        newValue._Raw._Value[1], newValue._Raw._Value[0],           // New value
        (LONG64*)&currValue._Raw._Value[0]) == 0);                  // Comp / result value))

    #if UnitTest_KSPtrBroker
        if (spinCount > 1)
        {
            InterlockedIncrement(&Test_GetSpinCount);
        }
    #endif

    // We took a ref count from the current value - that will be the result
    if (newValue._BSPtr._OwnedRefs == 1)
    {
        // The thread that takes current ref count to 1 must 
        // refresh the current allocation of ref counts
        KAssert(currValue._BSPtr._OwnedRefs == 2);      // if at this point, _Current ref count must have been 2
        currValue._BSPtr._OwnedRefs = 1;                // and now ang other Get() threads must be stuck at 1 if 
                                                        // until this thread rechanges the count or changing puts 
                                                        // occured

        newValue._BSPtr._OwnedRefs = 1 + AllocBlkSize;  // Optimistic: Allocate a new block of ref counts
        newValue._BSPtr._Obj->AddRefs(AllocBlkSize);

        // Try and apply (add in) the new ref counts
        if (InterlockedCompareExchange128(
            const_cast<volatile LONG64*>(&_Current._Raw._Value[0]), // Dest
            newValue._Raw._Value[1], newValue._Raw._Value[0],       // New value
            (LONG64*)&currValue._Raw._Value[0]) == 0)               // Comp / result value)))
        {
            #if UnitTest_KSPtrBroker
                InterlockedIncrement(&Test_GetBackoutCount);
            #endif

            // Something changed under us - Give back the newly allocated block of ref counts
            newValue._BSPtr._Obj->ReleaseRefs(AllocBlkSize);
        }
    }

    // Return acquired sptr using the ref count we hold implicitly
    KSharedPtr<TShared> result;
    result.Attach(newValue._BSPtr._Obj);
    return Ktl::Move(result);
}

//
// This template converts a class T that is not derived from KShared<T>
// into another type KSharedType<T>. KSharedType<T> has the same public
// interface as T, but can be used with KSharedPtr<>.
// KSharedType<T>::SPtr can be safely upcasted to KSharedType<T>* then T*.
//
// While T may be inherited, KSharedType<T> cannot be inherited.
// Further, KSharedType<T> has no inheritance relationship with
// KSharedType<TypeDerivedFromT>.
//
template <class T, class KAllocatorNOTRequired = void>
class KSharedType : public KShared<KSharedType<T>>, public T
{
public:
    typedef KSharedPtr<KSharedType<T>> SPtr;
    typedef KSharedPtr<KSharedType<T> const> CSPtr;

    //
    // The constructor is public so that a KSharedType<T> object
    // can be created using the _new operator. Client of this class
    // is encouraged to immediately wrap a KSharedType<T>* in
    // a KSharedPtr<> object. Use of raw pointers is highly discouraged.
    //
    // Base classes that require initialization with a KAllocator are
    // automatically supported as long as the KAllocator_Required() attribute
    // pattern is followed (see KArray as an example and KAllocator.h for explaination).
    //
    // Example:
    //  KSharedType<Foo>::SPtr sptr(new KSharedType<Foo>);
    //
    KSharedType() {}

private:
    friend KShared<KSharedType<T>>;
    friend VOID ::_delete<KSharedType<T>>(KSharedType<T>*);
    //
    // Hide destructor of this class.
    // A KSharedType<T> object cannot be constructed on the stack,
    // or an embedded object in another object.
    //
    // A KSharedType<T>* raw pointer cannot be deleted using
    // the delete operator either.
    //
    ~KSharedType() {}
};

template <class T>
class KSharedType<T, typename Ktl::enable_if<KAllocatorRequired<T>::value>::type> : public KShared<KSharedType<T>>, public T
{
public:
    typedef KSharedPtr<KSharedType<T>> SPtr;
    typedef KSharedPtr<KSharedType<T> const> CSPtr;

    //
    // The constructor is public so that a KSharedType<T> object
    // can be created using the _new operator. Client of this class
    // is encouraged to immediately wrap a KSharedType<T>* in
    // a KSharedPtr<> object. Use of raw pointers is highly discouraged.
    //
    // Base classes that require initialization with a KAllocator are
    // automatically supported as long as the KAllocator_Required() attribute
    // pattern is followed (see KArray as an example and KAllocator.h for explaination).
    //
    // Example:
    //  KSharedType<Foo>::SPtr sptr(new KSharedType<Foo>);
    //
    KSharedType()   :   T(this->GetThisAllocator()) {}

private:
    friend KShared<KSharedType<T>>;
    friend VOID ::_delete<KSharedType<T>>(KSharedType<T>*);
    //
    // Hide destructor of this class.
    // A KSharedType<T> object cannot be constructed on the stack,
    // or an embedded object in another object.
    //
    // A KSharedType<T>* raw pointer cannot be deleted using
    // the delete operator either.
    //
    ~KSharedType() {}
};

//** Support for ref-counted interface and multiple inheritance for KShared<> derivations - aka shared-interfaces.
//
// Any KShared<> derivation can implement one or many base classes or pure interfaces so long as those base
// classes do not derive from KShared<> or KObject<> themselves by using the pattern and macros defined below.
//
// By using these tools, such shared-interfaces can be treated as ref-counted objects via the normal KSharedPtr<>
// KTL facilities transparently. The only restriction is that the AddRef() and Release() methods can't be
// called directly on such shared-interfaces - those semantics can be met using the KSharedPtr::Attach/Detach
// methods.
//
// Defining KShared<> implementable "shared-interface":
//
//      interface MyPublicIf
//      {
//          K_SHARED_INTERFACE(MyPublicIf);
//
//      public:
//          <My interface members>
//      };
//
//                  -- or --
//
//      interface MyPublicIfExt : MyPublicIf
//      {
//          K_SHARED_INTERFACE(MyPublicIfExt);
//
//      public:
//          <My interface ext members>
//      };
//
//
// Defining a KShared<> deriavation implementaing shared-interfaces is accomplished using the following pattern:
//
//      interface AnotherSharedIf : ...                     // Yet another example shared-interface for good measure
//      {                                                   // NOTE: Can derive from anything except KShared<> or KObject<>
//          K_SHARED_INTERFACE(AnotherSharedIf);
//
//      public:
//          .
//          .
//          .
//      };
//
//      class MyImpClass : public KShared<MyImpClass>, public KObject<MyImpClass>, public MyPublicIfExt, public AnotherSharedIf
//      {
//          K_FORCE_SHARED(TestImp);
//          K_SHARED_INTERFACE_IMP(MyPublicIf);
//          K_SHARED_INTERFACE_IMP(MyPublicIfExt);
//          K_SHARED_INTERFACE_IMP(AnotherSharedIf);
//
//          <my concrete class imp>
//      };
//
// Beyond this, all usage of MyPublicIfExt, MyPublicIf, and AnotherSharedIf as ref-counted objects is transparent from a coding
// perspective:
//
//      MyPublicIfExt::SPtr, MyPublicIf::SPtr, and AnotherSharedIf::SPtr (or their KSharedPtr<>) form) all work exactly as one
//      would expect with any KShared<> derivation SPtr. All actual reference counting is done only on the implementing KShared<>
//      derivation.
//
// NOTE: Performance: There is extra runtime overhead when using KSharedPtr<> to these shared interfaces for ref-counting
//                    activities. Where direct KShared<> derivations enjoy a direct call to AddRef/Release, such activity
//                    thru a shared-interface SPtr is one vir function call and then the direct call.
//

// Macro to declare an "interface" type or non-KShared<> derivation class as trackable via KSharedPtr<>
#if DBG
#define K_SHARED_INTERFACE(if_name) \
        private: \
            friend KShared<if_name>; \
            friend KSharedPtr<if_name>; \
            friend KSharedPtr<if_name const>; \
            template <typename, typename>\
            friend struct SharedForwarder;\
        /*protected:*/public:\
            virtual LONG if_name##_KIfAddRef() = 0;\
            __forceinline LONG KIfAddRef() {return if_name##_KIfAddRef();} \
            \
            virtual LONG if_name##_KIfRelease() = 0; \
            __forceinline LONG KIfRelease() {return if_name##_KIfRelease();} \
            \
            virtual VOID if_name##_KIfAddTrackedSPtr(PLIST_ENTRY SptrLinkage) = 0; \
            __forceinline VOID KIfAddTrackedSPtr(PLIST_ENTRY SptrLinkage) {if_name##_KIfAddTrackedSPtr(SptrLinkage);} \
            \
           virtual VOID if_name##_KIfRemoveTrackedSPtr(PLIST_ENTRY SptrLinkage) = 0; \
            __forceinline VOID KIfRemoveTrackedSPtr(PLIST_ENTRY SptrLinkage) {if_name##_KIfRemoveTrackedSPtr(SptrLinkage);} \
        public: \
            const static BOOLEAN KIsSharedInterface = TRUE;\
            typedef KSharedPtr<if_name> SPtr; \
            typedef KSharedPtr<if_name const> CSPtr; \
        private:
#else
#define K_SHARED_INTERFACE(if_name) \
        private: \
            friend KShared<if_name>; \
            friend KSharedPtr<if_name>; \
            friend KSharedPtr<if_name const>; \
            template <typename, typename>\
            friend struct SharedForwarder;\
        /*protected:*/public:\
            virtual LONG if_name##_KIfAddRef() = 0;\
            __forceinline LONG KIfAddRef() {return if_name##_KIfAddRef();} \
            \
            \
            virtual LONG if_name##_KIfRelease() = 0; \
            __forceinline LONG KIfRelease() {return if_name##_KIfRelease();} \
        public: \
            const static BOOLEAN KIsSharedInterface = TRUE;\
            typedef KSharedPtr<if_name> SPtr; \
            typedef KSharedPtr<if_name const> CSPtr; \
        private:
#endif

// Macro to declare a KShared<>-based implementation of a K_SHARED_INTERFACE-marked base class
#if DBG
#define K_SHARED_INTERFACE_IMP(IfName) \
        private: \
            \
            LONG IfName##_KIfAddRef() override \
            { \
                return KSharedBase::AddRef(); \
            } \
            LONG IfName##_KIfRelease() override \
            { \
                return KSharedBase::Release(); \
            } \
            VOID IfName##_KIfAddTrackedSPtr(PLIST_ENTRY SptrLinkage) override \
            { \
                return KSharedBase::AddTrackedSPtr(SptrLinkage); \
            } \
            VOID IfName##_KIfRemoveTrackedSPtr(PLIST_ENTRY SptrLinkage) override \
            { \
                return KSharedBase::RemoveTrackedSPtr(SptrLinkage); \
            } \
        private:

#else
#define K_SHARED_INTERFACE_IMP(IfName) \
        private: \
            \
            LONG IfName##_KIfAddRef() override \
            { \
                return KSharedBase::AddRef(); \
            } \
            LONG IfName##_KIfRelease() override \
            { \
                return KSharedBase::Release(); \
            } \
        private:
#endif




//** Weak Reference Support for KShared<> Types
//
// Any KShared<> derivation can enable support for weak references for its instances via the following mix-in
// template patterns:
//
//      class Derived : public KObject<Derive>, public KShared<Derived>, public KWeakRefType<Derived>
//      {
//          �
//      };
//
//      -- or �-
//
//      class Derived : public KObject<Derive>, public KShared<Derived>
//      {
//          �
//      };
//
//      class NextDerived : public Derived, public KWeakRefType<NextDerived>
//      {
//          �
//      };
//
//  Notice that the enabling KWeakRefType<> mix-in template can be applied either as a peer mix-in to
//  KShared<>/KObject<> or to a further derivation. From a declaration point of view applying the
//  KWeakRefType mix-in template is all that must be done to enable weak reference access to a KShared<>
//  derivative. A KWeakRefType<> derivation must also have KObject<> mixed into its inheritance aggregation.
//
//  NOTE: The KWeakRefType<> ctor is FAILABLE - check status after construction. The KWeakRefType<> constructor
//        attempts to allocate a parallel KWeakRef<> instance using the KAllocator used to to allocate the
//        corresponding KWeakRefType<> instance. If that allocation fails, that KWeakRefType instance's
//        KObject::Status() method will return at least a STATUS_INSUFFICIENT_RESOURCES failure status.
//
//
//* Weak Reference support for Shared Interfaces (K_SHARED_INTERFACE()):
//
//  When exposing a shared interface on a weakly reference-able KShared<> derivation, that concrete derivation
//  MUST include a K_WEAKREF_INTERFACE_IMP() macro call in it's defintion for each shareable interface implemented
//  on which weak ref support is desired. Likewise each shareable interface on which weak referencing is to be allowed
//  MUST include a K_WEAKREF_INTERFACE() macro call in its definition:
//
//      class MyIf
//      {
//          K_SHARED_INTERFACE(MyIf);
//          K_WEAKREF_INTERFACE(MyIf);
//          �
//      };
//
//      class Derived : public KObject<Derive>, public KShared<Derived>, public KWeakRefType<Derived>, public MyIf
//      {
//          K_FORCE_SHARED(Derived);
//          K_SHARED_INTERFACE_IMP(MyIf);
//          K_WEAKREF_INTERFACE_IMP(MyIf, Derived);
//          �
//      };
//
//  In order to use weak refs from an interface user's perspective a fixed method - GetWeakIfRef() - is used to create
//  and return a SPtr to a KWeakRef<> for the interface. NOTE: Unlike the concrete GetWeakRef() available on the implementation
//  classes, this call can return an empty SPtr indicating an out-of-memory condition. Beyond these differences, all other
//  weak reference behavior works the same for shared interfaces as it does for concrete imp class. There is however two levels
//  of call indirection for the IsActive() and TryGetTarget() more than for concrete imp classes.
//
//      NOTE: KWeakRefType<T> MUST be listed in T's base class list (directly or indirectly) BEFORE the KShared<T> declaration
//

#define K_WEAKREF_INTERFACE(IType) \
    public:  virtual NTSTATUS IType##_GetWeakIfRef(__out KSharedPtr<KWeakRef<IType>>& Result) = 0; \
             __forceinline NTSTATUS GetWeakIfRef(__out KSharedPtr<KWeakRef<IType>>& Result) {return IType##_GetWeakIfRef(Result); }


#define K_WEAKREF_INTERFACE_IMP(IfType,ImpType) \
    private: \
        NTSTATUS IfType##_GetWeakIfRef(__out KSharedPtr<KWeakRef<IfType>>& Result) override \
        { \
            KSharedPtr<KWeakRef<ImpType>>    realWeakRefSPtr = GetWeakRef(); \
            \
            auto interfaceMap = _new(KTL_TAG_WEAKREF, GetThisAllocator()) KWeakInterfaceRefImp<IfType, ImpType>(*realWeakRefSPtr, *this); \
            if (interfaceMap == nullptr) \
            { \
                Result = nullptr; \
                return STATUS_INSUFFICIENT_RESOURCES; \
            } \
            Result = (KWeakRef<IfType>*)interfaceMap; \
            return STATUS_SUCCESS; \
        }

//* Class used to generate strongly typed weak reference types to type KWeakRefType<T> derivations
//
template <class T>
class KWeakRef sealed : public KObject<KWeakRef<T>>, public KShared<KWeakRef<T>>
{
    K_FORCE_SHARED(KWeakRef);

public:
    // Validate a KWeakRef is associated with a KWeakRefType<> instance that is alive.
    //
    // Returns: TRUE    - Target is associated and alive
    //          FALSE   - Target has expired
    //
    BOOLEAN
    IsAlive();

    // Attempt to bind a KSharedPtr to an associated target instance of type T
    //
    // Returns: KSharedPtr<T>.RawPtr() != nullptr if Target is alive; else it has expired.
    //
    // Overloads to get different implementation called
    template <typename U = T>
    KSharedPtr<T> TryGetTarget(typename Ktl::enable_if<isSharedClassImp<U>::value>::type* p = nullptr);

    template <typename U = T>
    KSharedPtr<T> TryGetTarget(typename Ktl::enable_if<isSharedInterface<U>::value && !isSharedClassImp<U>::value>::type* p = nullptr);

private:
    friend class KWeakRefType<T>;

    NOFAIL
    KWeakRef(__in KWeakRefType<T>* TargetObj);

    __inline VOID
    TargetIsDestructing(KWeakRefType<T>* Target);

private:
    KSpinLock           _ThisLock;
    KWeakRefType<T>*    _TargetObj;

#if EnableKWeakRefUnitTest
private:
    volatile LONG       _TargetIsDestructing;
public:
    volatile BOOLEAN    _TargetDestructingRaceDetected;
#endif
};

//* Abstract class for an weak ref'ed Interface type mapping to an implementation type.
class KWeakInterfaceRef : public KObject<KWeakInterfaceRef>, public KShared<KWeakInterfaceRef>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KWeakInterfaceRef);

public:
    virtual BOOLEAN IsAlive() = 0;
    virtual void* TryGetTarget() = 0;       // returns i/f ptr or nullptr
};



//* template to generate type specfic (Interface AND Imp types) class mapping weak ref'ed Interface types
//  mapping to an implementation type.

template <class IfType, class ImpType>
class KWeakInterfaceRefImp : public KWeakInterfaceRef
{
    K_FORCE_SHARED(KWeakInterfaceRefImp);

public:
    KWeakInterfaceRefImp(KWeakRef<ImpType>& RealWeakRef, IfType& InterfaceBase)
        :   _WeakRef(&RealWeakRef),
            _IfRef(InterfaceBase)
    {
    }

private:
    BOOLEAN IsAlive() override
    {
        return _WeakRef->IsAlive();
    }

    void* TryGetTarget() override
    {
        IfType*                  result = nullptr;
        typename ImpType::SPtr   impSPtr = _WeakRef->TryGetTarget();
        if (impSPtr.RawPtr() != nullptr)
        {
            result = impSPtr.Detach();      // imp cast - cause compiler error if something wrong
        }

        return result;
    }

private:
    KSharedPtr<KWeakRef<ImpType>>   _WeakRef;
    IfType&                         _IfRef;
};

template <class IfType, class ImpType>
KWeakInterfaceRefImp<IfType, ImpType>::~KWeakInterfaceRefImp() {}


//* Class used to generate strongly typed weakly reference-able mix-in for KShared<>
//  derivations.
//
template <class T>
class KWeakRefType : private KObjectMixInBase
{
    // NOTE: No type dependent (T) layout differences can be introduced into this templated
    //       type due to casting that is needed for supporting weakly referenced shared interfaces
    //       (see K_WEAKREF_INTERFACE) documentation above.

public:
    // Gain (safe) access to the KWeakRef<T> instance for a given instance of type T.
    //
    // Returns: A SPtr to the corresponding KWeakRef<T> instance.
    //
    KSharedPtr<KWeakRef<T>>
    GetWeakRef();

protected:
    FAILABLE
    KWeakRefType();

    virtual
    ~KWeakRefType();

    KWeakRef<T>& GetWeakRefInternal();

private:        // private "API" for KWeakRef<>
    friend class KWeakRef<T>;

    __forceinline BOOLEAN
    TryTakeRef();

    __forceinline VOID
    DetachWeakRef(KWeakRef<T>* WeakRef);

private:
    KSharedPtr<KWeakRef<T>> _WeakRef;

#if EnableKWeakRefUnitTest
public:
    volatile LONG           _TryTakeRefRaceDetected;
#endif
};


//* KWeakRefType<> Implementation
//
//  NOTE: Only KShared<> derivations are supported
//
template <class T>
KWeakRefType<T>::KWeakRefType()
{
    static_assert(isSharedClassImp<T>::value, "Only KShared<> derivations are supported");

    KInvariant(((T*)this)->_RefCount == 0); // NOTE: KWeakRefType<T> MUST be listed in T's base class list (directly or indirectly) 
                                            //       BEFORE the KShared<T> declaration or this WILL HAPPEN

    #if EnableKWeakRefUnitTest
        _TryTakeRefRaceDetected = 0;
    #endif

    // Automatcally generate a parallel KWeakRef for this KWeakRefType instance - using the
    // same allocator used to allocate this instance
    _WeakRef = _new(KTL_TAG_WEAKREF, KAllocatorSupport::GetAllocator((T*)this)) KWeakRef<T>(this);
    if (!_WeakRef)
    {
        KObjectMixInBase::FwdSetConstructorStatus((T*)this, STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    KAssert(NT_SUCCESS(_WeakRef->Status()));        // KWeakRef ctor is NOFAIL
}

template <class T>
KWeakRefType<T>::~KWeakRefType()
{
    static_assert(isSharedClassImp<T>::value, "Only KShared<> derivations are supported");

    KInvariant(((T*)this)->_RefCount == 0);
    if (_WeakRef)                   // Protect against being partially constructed
    {
        _WeakRef->TargetIsDestructing(this);
        KAssert(!_WeakRef);         // DetachWeakRef() must be called by KWeakRef::TargetIsDestructing()
    }
}

template <class T> __inline
KSharedPtr<KWeakRef<T>>
KWeakRefType<T>::GetWeakRef()
{
    static_assert(isSharedClassImp<T>::value, "Only KShared<> derivations are supported");

    // Some form of "leaky" access is being attempted - should not happen - we assume the caller is
    // making ref to this instance ONLY via a SPtr and thus this->_RefCount must be > 0;
    KInvariant(((T*)this)->_RefCount > 0);
    return _WeakRef;
}

template <class T> __inline
KWeakRef<T>& KWeakRefType<T>::GetWeakRefInternal()
{
    static_assert(isSharedClassImp<T>::value, "Only KShared<> derivations are supported");

    KInvariant(_WeakRef.RawPtr());
    return *(_WeakRef.RawPtr());
}

template <class T> __inline
BOOLEAN
KWeakRefType<T>::TryTakeRef()
//
// Returns: FALSE  - instance's refcount == 0 (and latched there)
//          TRUE   - ref count has been AddRef'd
//
{
    static_assert(isSharedClassImp<T>::value, "Only KShared<> derivations are supported");

    LONG            refCountValue;
    volatile LONG*  refCountAlias = &((T*)this)->_RefCount;

    while ((refCountValue = *refCountAlias) > 0)
    {
        // Try and increment ref count; allowing for the count to go to 0 under us. We must never allow
        // the ref count to go above 0 once it goes to 0.
        LONG newRefCount = refCountValue + 1;

        #if EnableKWeakRefUnitTest
            #if KTL_USER_MODE
                //
                // Add yield here to allow other threads to run in this
                // window where a collision could occur and thus ensure
                // that TakeRefRaceTest unit test will detect a race
                // condition on single proc machines
                //
                KNt::Sleep(0);
            #endif
        #endif

        if (_InterlockedCompareExchange(refCountAlias, newRefCount, refCountValue) == refCountValue)
        {
            return TRUE;
        }

        #if EnableKWeakRefUnitTest
            _InterlockedIncrement(&_TryTakeRefRaceDetected);
        #endif
    }

    return FALSE;
}

template <class T> __inline
VOID
KWeakRefType<T>::DetachWeakRef(KWeakRef<T>* WeakRef)
//
// NOTE: Only "called" (inlined) by an associated WeakRef that has locked access (including
//       the ~KWeakRefType path).
//
{
    KAssert(_WeakRef.RawPtr() == WeakRef);  // must be associated to the referencing (calling) WeakRef
    #if !DBG
    UNREFERENCED_PARAMETER(WeakRef);
    #endif

    _WeakRef.Reset();
}


//* KWeakRef<T> Implementation
template <class T>
KWeakRef<T>::KWeakRef(__in KWeakRefType<T>* TargetObj)
    :   _TargetObj(TargetObj)
{
    // Can't use static_assert as instantiation needs to occur but call should not occur.
    if (!isSharedClassImp<T>::value)
    {
        if (isSharedInterface<T>::value)
        {
            // Only KShared<> derivations can be actually referenced via weak references
            KInvariant(FALSE);
        }
    }
    else
    {
        #if EnableKWeakRefUnitTest
            _TargetIsDestructing = 0;
            _TargetDestructingRaceDetected = FALSE;
        #endif
    }
}

template <class T>
KWeakRef<T>::~KWeakRef()
{
    // Can't use static_assert as instantiation needs to occur but call should not occur.
    if (!isSharedClassImp<T>::value)
    {
        if (isSharedInterface<T>::value)
        {
            // Only KShared<> derivations can be actually referenced via weak references
            KInvariant(FALSE);
        }
    }
    else
    {
        KAssert(_TargetObj == nullptr);
    }
}

template <class T>
BOOLEAN
KWeakRef<T>::IsAlive()
{
    // Can't use static_assert as instantiation needs to occur but call should not occur.
    if (!isSharedClassImp<T>::value)
    {
        if (isSharedInterface<T>::value)
        {
            // Do type mapping

            // NOTE: The this pointer is really pointing at a type specific KWeakInterfaceRefImp<> instance
            //       by the generated when a specfic K_WEAKREF_INTERFACE_IMP() GetWeakIfRef() is called.
            return ((KWeakInterfaceRef*)this)->IsAlive();
        }
    }
    else
    {
        #if EnableKWeakRefUnitTest
            K_LOCK_BLOCK(_ThisLock)
            {
                if (_TargetIsDestructing > 0)
                {
                    _TargetDestructingRaceDetected = TRUE;
                }
            }
        #endif

        return _TargetObj != nullptr;
    }
}

template <typename T>
template <typename U>
KSharedPtr<T>
KWeakRef<T>::TryGetTarget(typename Ktl::enable_if<isSharedClassImp<U>::value>::type*)
{
    KSharedPtr<T>   result;

    static_assert(isSharedClassImp<T>::value, "Only KShared<> derivations are supported");

    BOOLEAN         haveReference = FALSE;

    K_LOCK_BLOCK(_ThisLock)
    {
        #if EnableKWeakRefUnitTest
            if (_TargetIsDestructing > 0)
            {
                _TargetDestructingRaceDetected = TRUE;
            }
        #endif
        haveReference = (_TargetObj != nullptr) && _TargetObj->TryTakeRef();
    }

    if (haveReference)
    {
        // Call to TryTakeRef() above did an AddRef on _TargetObj
        result.Attach((T*)_TargetObj);          // Take the ref
        KAssert(_TargetObj != nullptr);
    }

    return result;
}

template <typename T>
template <typename U>
KSharedPtr<T>
KWeakRef<T>::TryGetTarget(typename Ktl::enable_if<isSharedInterface<U>::value && !isSharedClassImp<U>::value>::type*)
{
    KSharedPtr<T>   result;

    static_assert(isSharedInterface<T>::value, "Only SharedInterface types are supported");

    // Do type mapping

    // NOTE: The this pointer is really pointing at a type specific KWeakInterfaceRefImp<> instance
    //       by the generated when a specfic K_WEAKREF_INTERFACE_IMP() GetWeakIfRef() is called.

    void* realIfPtr = ((KWeakInterfaceRef*)this)->TryGetTarget();
    result.Attach((T*)realIfPtr);
    return result;
}

template <class T> __inline
VOID
KWeakRef<T>::TargetIsDestructing(KWeakRefType<T>* Target)
//
// Lock the API (and hold KWeakRefType dtor completion) until the issuing Target is detached from the
// KWeakRef, then reset _TargetObj and release Lock. Called ONLY from KWeakRefType<T> dtor.
//
{
    if (!isSharedClassImp<T>::value)
    {
        if (isSharedInterface<T>::value)
        {
            KInvariant(FALSE);          // Should never be called in terms of this type
        }
    }
    else
    {
        #if EnableKWeakRefUnitTest
            _InterlockedIncrement(&_TargetIsDestructing);
        #endif

        this->AddRef();           // Hold destruction (0); need to keep _ThisLock valid
        K_LOCK_BLOCK(_ThisLock)
        {
            #if EnableKWeakRefUnitTest
                _InterlockedDecrement(&_TargetIsDestructing);
            #endif

            // This instance must point to IssuingTarget if
            // IssuingTarget points to the current KWeakRef instance
            KInvariant(((void*)Target) == _TargetObj);
            _TargetObj = nullptr;
        }

        Target->DetachWeakRef(this);
        this->Release();          // Allow destruction (0)
    }
}


//** down_cast
//
// A KTL-specific cast implementation which is used to convert a base class SPtr
// to its childmost type.
//
template <class Derived, class Base>
Derived* down_cast(const KSharedPtr<Base>& Src)
{
    return static_cast<Derived*>(Src.RawPtr());
}

//** up_cast
//
// A KTL-specific cast implementation which is used to convert a derived class SPtr
// to its base type.
//
template<class Base, class Derived> __inline
KSharedPtr<Base>&
up_cast(const KSharedPtr<Derived>& DerivedRef)
{
    return reinterpret_cast<KSharedPtr<Base>&>((KSharedPtr<Derived>&)DerivedRef);
}


namespace Ktl {
    //
    // Magic templates that allow a lVaule to be converted to an rValue.
    //

    template <typename T> struct RemoveReference {
         typedef T type;
    };

    template <typename T> struct RemoveReference<T&> {
         typedef T type;
    };

    template <typename T> struct RemoveReference<T&&> {
         typedef T type;
    };

    template <typename T> typename RemoveReference<T>::type&& Move(T&& t) {
        return (typename RemoveReference<T>::type&&)(t);
    }
}
