/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    ktl.h

    Description:
      Kernel Tempate Library (KTL): Primary header file

      Generic definitions and all other includes

    History:
      raymcc          16-Aug-2010         Initial version.

--*/


#pragma once

#pragma warning(push)

//
// Disable harmless /W4 warnings
//
#pragma warning(disable:4481)   // C4481: nonstandard extension used: override specifier 'override'
#pragma warning(disable:4239)   // C4239: nonstandard extension used : 'token' : conversion from 'type' to 'type'

#if defined(PLATFORM_UNIX)
#include <unistd.h>
// At this point NULL is defined to be __null, but this gets set to nothing by our PAL headers (__null is used by SAL)
// Let's define it to 0, which is "normal" for C++
#define NULL 0
#include <cstddef>
#include <string>
#include <codecvt>
#include <locale>
#include <semaphore.h>
#include <ktlpal.h>
#include "winrt/ntassert.h"
#include <wchar.h>
#include <new>

template <typename T, size_t N> char(*RtlpNumberOf(UNALIGNED T(&)[N]))[N];

#define RTL_NUMBER_OF_V2(A)  (sizeof(*RtlpNumberOf(A)))

#define ARRAYSIZE(A)  RTL_NUMBER_OF_V2(A)

#define FIELD_OFFSET(type, field)  offsetof(type, field)

namespace KtlIfExistsUtil
{
    namespace detail
    {
        struct NilType
        {
        };

        template <typename T>
        struct IsNilType
        {
            static const bool value = false;
        };

        template <>
        struct IsNilType<NilType>
        {
            static const bool value = true;
            typedef void True;
        };
    }

    template <typename T, class Enable = void>
    struct TestExists
    {
        typedef T Type;
    };

    template <typename T>
    struct TestExists<T, typename std::enable_if<!std::is_class<T>::value >::type>
    {
        typedef detail::NilType Type;
    };
};

#define IMPLEMENTS(T, func)   KtlIfExistsUtil::TestExists<T>::Type::func

#define abstract
#define sealed

inline std::wstring Utf8To16(const char *str)
{
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;
    std::u16string u16str = conv.from_bytes(str);
    std::basic_string<wchar_t> result;
    for(int index = 0; index < u16str.length(); index++)
    {
        result.push_back((wchar_t)u16str[index]);
    }
    return result;
}

inline std::string Utf16To8(const wchar_t *wstr)
{
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;
    return conv.to_bytes((const char16_t *) wstr);
}

#include "paldef.h"

#else // PLATFORM_UNIX

#define IMPLEMENTS(T, func)   T::func

#endif // PLATFORM_UNIX

#if KTL_USER_MODE
#define SECURITY_WIN32
#else
#define SECURITY_KERNEL
#endif

#if !KTL_USER_MODE
#define interface class         // kernel compiler does not current support "interface"
#endif

//  All types that are blittable SHOULD be decorated with this marco in their type definition to
//  help cause optimized datastructure behavior - e.g. KArray
//
//  By definition a blittable object can be "moved" (not copied) without side effects first copying
//  the raw bytes of the object to another non-constructed location and the source bytes.
//
//  Further if a blittable type is all zeros by default (including after it is Moved), 
//  DefaultIsZero() can be used to further indicate this fact and enable optimazations.
//
//  KIs_BlittableType_DefaultIsZero():
//      1) Can be zeroed after it is a source of a move
//
//  Example:
//      struct MyStruct
//      {
//          ... POD fields
//
//          KIs_BlittableType();
//      };
//
//  This would allow:
//      KArray<MyStruct>    myStructArray(Allocator);
//
//
//
#define KIs_BlittableType() \
    public: \
        const static BOOLEAN KIsBlittableType = TRUE; \
    private:

#define KIs_BlittableType_DefaultIsZero() \
    public: \
        const static BOOLEAN KIsBlittableType_DefaultIsZero = TRUE; \
    private:

#include <ktlcommon.h>
#include <kstatus.h>

#include <ktags.h>
#include <kallocator.h>
#include <KTypes.h>

//
// The following class can be used to add an 8 character tag to an object. This is
// especially useful for verifying objects which have been down cast.
//

typedef char KTagType[9];

const KTagType KDefaultTypeTag = "KObjType";

class KObjectTypeTag : public KStrongType<KObjectTypeTag, ULONGLONG>
{
    K_STRONG_TYPE(KObjectTypeTag, ULONGLONG);

public:
    KObjectTypeTag(__in const KTagType& Name)
    {
        _Value = *((ULONGLONG UNALIGNED *)&Name);
    }
};


//
// A KObject object has the commonly used fields such as the "status" field.
//

template <class T>
class KObject
{
public:
    NTSTATUS Status() const
    {
        return _Status;
    }

    KObject<T>&
    operator=(__in KObject<T> const& Right)
    {
        if (this != &Right)
        {
            _Status = Right._Status;
        }

        return *this;
    }

    KObjectTypeTag GetTypeTag() const
    {
        return _TypeTag;
    }

protected:
    friend class KObjectMixInBase;

    NOFAIL KObject() : _TypeTag(KDefaultTypeTag), _Status(STATUS_SUCCESS) {}

    VOID SetStatus(__in NTSTATUS Status)
    {
        _Status = Status;
    }

    VOID SetConstructorStatus(__in NTSTATUS Status)
    {
        if (!NT_SUCCESS(Status) && NT_SUCCESS(_Status))
        {
            _Status = Status;
        }
    }

    VOID SetTypeTag(__in KObjectTypeTag TypeTag)
    {
        _TypeTag = TypeTag;
    }

private:

    //
    // Type information of this object.
    // It is a cheap way to check type safety for down casting.
    // It is also useful in debugging to determine the real type of a base class object.
    //
    // If type T is not derived from KTagBase<>, _TypeTag is set to the default type tag.
    // Otherwise, _TypeTag is automatically set to the leaf-most type tag during construction
    // by the KTagBase<> template ctor.
    //

    KObjectTypeTag _TypeTag;

    NTSTATUS    _Status;
};

//* A special concrete friend class of all KObject<> generated types; allowing access to those
// types' KObject related behaviors.
//
class KObjectMixInBase
{
protected:
    KObjectMixInBase() {}       // Only allow use by derivations

    template <class KObjectDerived> __inline static
    VOID FwdSetConstructorStatus(KObjectDerived* thisPtr, NTSTATUS Status)
    {
        thisPtr->KObject::SetConstructorStatus(Status);
    }

    template <class KObjectDerived> __inline static
    VOID FwdSetTypeTag(KObjectDerived* thisPtr, ULONGLONG TypeTag)
    {
        thisPtr->KObject::SetTypeTag(TypeTag);
    }
};

namespace Ktl
{
    //
    // Implementation of STL enable_if
    //
    template<bool B, class T = void>
    struct enable_if {};

    template<class T>
    struct enable_if<true, T> { typedef T type; };
}

#include <kpointer.h>
#include <krtt.h>

//
// Use this macro to create an ULONGLONG tag from two 4-char parts.
//
// Example to create a tag of "MyType":
// const ULONGLONG MyTag = K_MAKE_TYPE_TAG('  ep', 'yTyM');
//
// MyTag can be used as the template parameter for KTagBase<>.
//

#define K_MAKE_TYPE_TAG(Tag1, Tag2)    ((ULONGLONG)Tag2 | ((ULONGLONG)Tag1) << 32)

//
// Template class to automatically set the correct type information during object construction.
//
// Example (assuming the tag values are already created, e.g. via the K_MAKE_TYPE_TAG() macro):
//
//  class Base : public KObject<Base>, public KTagBase<Base, BaseTypeTag>
//  { ... }
//
//  class Derived1 : public Base
//  { ... }
//
//  class Derived2 : public Derived1, public KTagBase<Derived2, Derived2Tag>
//  { ... }
//
//  ...
//
//  Base* p = new Derived2();
//  KFatal(p->GetTypeTag() == Derived2Tag); // This assert guarantees to be true
//
//  Notes:
//  (1) Use of KTagBase<> is optional. Any class in a class inheritance hierarchy
//      can choose to inherit from its own KTagBase<> class or not.
//  (2) Always put KTagBase<> as the last base class to inherit from in the class definition.
//      This is to ensure that the correct KTagBase<> ctor is called last to set the type value.
//

template <class DerivedType, ULONGLONG TypeTag>
class KTagBase : private KObjectMixInBase
{
protected:
    KTagBase()
    {
        //
        // Set the _TypeTag field in DerivedType.
        // DerivedType must inherit from KObject, directly or indirectly.
        //
        DerivedType* derivedType = static_cast<DerivedType*>(this);
        KObjectMixInBase::FwdSetTypeTag(derivedType, TypeTag);
    }

    ~KTagBase() {}
};

//
// Routines to access KTL allocators.
//
class KtlSystemCore : public KObject<KtlSystemCore>
{
public:
    virtual ~KtlSystemCore() = 0;

    virtual KAllocator&
    NonPagedAllocator() = 0;

    virtual KAllocator&
    PagedAllocator() = 0;

    virtual KAllocator&
    PageAlignedRawAllocator() = 0;

    #if KTL_USER_MODE
        virtual void SetEnableAllocationCounters(bool ToEnabled) = 0;
    #endif
};

__inline
KtlSystemCore::~KtlSystemCore()
{
}

#ifndef KTL_CORE_LIB
//
// Routines to access KTL global data structures.
//
class KThreadPool;
class KtlSystemBase;
class KtlSystem : public KtlSystemCore
{
	friend KtlSystemBase;

public:
    static NTSTATUS
    Initialize(__out_opt KtlSystem* *const OptResultSystem = nullptr);

    static NTSTATUS
    Initialize(__in BOOLEAN EnableVNetworking,
               __out_opt KtlSystem* *const OptResultSystem = nullptr);

    static VOID
    Shutdown(__in ULONG TimeoutInMs = 5000);

    static KtlSystem&
    GetDefaultKtlSystem();          // Only valid in test environments after Initialize() is called.
	
    static ULONG const      InfiniteTime = MAXULONG;

    //  Activate an instance of KtlSystem
    //
    //  Description: The caller of this method will not get control back until an error occurs (e.g. timeout)
    //               or the underlying instance can do useful work from the caller's perspective. Care should
    //               be taken to call this method on an appropriate thread - it should not be a KTL thread pool
    //               thread.
    //
    //  Parameters:
    //      MaxActiveDurationInMsecs - The number of msecs the activation process is allowed before failing.
    //                                  A value of KtlSystem::InfiniteTime can be passed to disable the timeout
    //                                  facility.
    //
    //  On Return:
    //      Status()    - STATUS_SUCCESS - The activated instance is usable. It can be assumed that this condition
    //                                     indicates that the underlying environment hosted by a KtlSystem instance
    //                                     is useful until deactivated via Deactivate().
    //
    virtual VOID
    Activate(__in ULONG MaxActiveDurationInMsecs) = 0;

    //  Deactivate an instance of KtlSystem
    //
    //  Parameters:
    //      MaxDeactiveDurationInMsecs - The number of msecs the deactivation process is allowed before failing.
    //                                   A value of KtlSystem::InfiniteTime can be passed to disable the timeout
    //                                   facility.
    //
    //  On Return:
    //      Status()    - STATUS_SUCCESS - The instance was deactivated correctly. Any other status will indicate a
    //                                     specific failure during the deactivation process.
    //
    virtual VOID
    Deactivate(__in ULONG MaxDeactiveDurationInMsecs) = 0;

    // Return the unique ID of a KtlSystem instance
    virtual GUID
    GetInstanceId() = 0;

    virtual KThreadPool&
    DefaultThreadPool() = 0;

    virtual KThreadPool&
    DefaultSystemThreadPool() = 0;

    virtual NTSTATUS
    RegisterGlobalObject(
        __in_z const WCHAR* ObjectName,
        __inout KSharedBase& Object) = 0;

    virtual VOID
    UnregisterGlobalObject(__in_z const WCHAR* ObjectName) = 0;

    virtual
    KSharedBase::SPtr
    GetGlobalObjectSharedBase(__in_z const WCHAR* ObjectName) = 0;

    template <class T>
    KSharedPtr<T>
    GetGlobalObject(__in_z const WCHAR* ObjectName)
    {
        KSharedPtr<KSharedBase> object = GetGlobalObjectSharedBase(ObjectName);
        return (T*) (object.RawPtr());
    }

    virtual void
    SetStrictAllocationChecks(__in BOOLEAN AllocationChecksOn) = 0;

    virtual BOOLEAN
    GetStrictAllocationChecks() = 0;

    #if KTL_USER_MODE
        // Set default KAsyncContextBase internal thread pool usage
        // Defaults: UM - TRUE; KM - FALSE
        virtual void
        SetDefaultSystemThreadPoolUsage(BOOLEAN OnOrOff) = 0;

        virtual BOOLEAN
        GetDefaultSystemThreadPoolUsage() = 0;

		//* Returns TRUE is the calling thread is a dedicated KTL thread associated with any active KtlSystem
		static BOOLEAN	IsCurrentThreadOwned();
    #endif

    // Global allocation support for Test applications
    static KAllocator&
    GlobalNonPagedAllocator();

    static KAllocator&
    GlobalPagedAllocator();

protected:
    KtlSystem();
    ~KtlSystem();

};
#endif // KTL_CORE_LIB


// _KMemRef
//
// This is general purpose utility struct which holds a reference to an address, its allocated size,
// and the number of bytes actually in use.  This object never owns the memory it references.
//
// The _KMemRef version is used internally within KVariant to bypass the rules on unions having constructors.
// The derived version only adds a constructor and the version end-users should use.
//
struct _KMemRef
{
    ULONG _Size;
    ULONG _Param;           // Generic multi-use parameter for 'Bytes Used', or flag values
    PVOID _Address;
};

struct KMemRef : public _KMemRef
{
    KMemRef()
    {
        _Size = _Param = 0;
        _Address = nullptr;
    }

    KMemRef(
        __in _KMemRef& Src
        )
    {
        _Size = Src._Size;
        _Param = Src._Param;
        _Address = Src._Address;
    }

    KMemRef(
        __in ULONG Sz,
        __in PVOID Addr
        )
    {
        _Param = 0;
        _Size = Sz;
        _Address = Addr;
    }

    KMemRef(
        __in ULONG Sz,
        __in PVOID Addr,
        __in ULONG ParamVal
        )
    {
        _Param = ParamVal;
        _Size = Sz;
        _Address = Addr;
    }



    ~KMemRef()
    {
        // Never delete the memory
    }
};

// Ktl::Remove*::type - type conversions to remove type modifiers
//
namespace Ktl
{
    // Remove outer "const"
    template<typename T>
    struct RemoveConst         // non-const normalization
    {
        typedef T type;
    };

    template<typename T>
    struct RemoveConst<const T>
    {
        typedef T type;
    };
}

#include <knt.h>

#ifdef KTL_CORE_LIB
#include <kdelegate.h>
#include <kspinlock.h>
#include <ksemaphore.h>
#include <kthread.h>
#include <kbuffer.h>
#include <kfinally.h>
#include <kqueue.h>
#include <kstack.h>
#include <karray.h>
#include <klookaside.h>
#include <kchecksum.h>
#include <kavltree.h>
#include <knodelist.h>
#include <kiobuffer.h>
#include <knodetable.h>
#include <kbitmap.h>

#include <KGuid.h>
#include <kevent.h>
#include <kspinlock.h>
#include <karray.h>
#include <knodelist.h>
#include <KtlCoreAllocator.h>
#include <KtlSystemCoreImp.h>
#include <kmemchannel.h>
#else
#include <kdelegate.h>
#include <kspinlock.h>
#include <ksemaphore.h>
#include <kthread.h>
#include <kbuffer.h>
#include <kfinally.h>
#include <kqueue.h>
#include <kstack.h>
#include <karray.h>
#include <klookaside.h>
#include <kchecksum.h>
#include <kavltree.h>
#include <knodelist.h>
#include <kiobuffer.h>
#include <knodetable.h>
#include <kbitmap.h>
#include <kthreadpool.h>
#include <KLinkList.h>
#include <KAsyncContext.h>
#include <kdeferredcall.h>
#include <kresourcelock.h>
#include <KAsyncLock.h>
#include <KAsyncEvent.h>

#include <kstringview.h>
#include <kuri.h>
#include <kdatetime.h>

#include <kwstring.h>
#include <KStreams.h>

#include <khashtable.h>
#include <kregistry.h>
#include <kevent.h>
#include <ksynchronizer.h>
#include <KGuid.h>
#include <kperfcounter.h>
#include <kblockfile.h>
#include <KTimer.h>
#include <kvolumenamespace.h>
#include <krangelock.h>
#include <kreadcache.h>
#include <kcachedblockfile.h>
#include <KBlockDevice.h>
#include <KShiftArray.h>
#include <KAsyncQueue.h>
#include <kvariant.h>

#include <kmemchannel.h>

#include <kdgrammeta.h>

#include <itransport.h>

#include <knetworkendpoint.h>

#include <kserial.h>
#include <knetfaultinjector.h>
#include <ksocket.h>
#include <kmessageport.h>

#if !defined(PLATFORM_UNIX)
#include <knetwork.h>
#endif

#include <KTextFile.h>
#include <KQuotaGate.h>
#include <KtlCoreAllocator.h>
#include <KtlSystem.h>
#include <KtlSystemCoreImp.h>

#if !defined(PLATFORM_UNIX)
#include <knetchannel.h>
#endif

#include <KXmlParser.h>
#include <ktlperf.h>
#include <kdompath.h>
#include <KDom.h>

#if !defined(PLATFORM_UNIX)
#include <kwebsocket.h>
#include <khttp.h>
#include <kmgmt.h>
#include <kmgmtserver.h>
#endif

#include <ktask.h>
#include <KAsyncService.h>

#if !defined(PLATFORM_UNIX)
#include <ksspi.h>
#include <kssl.h>
#include <kKerberos.h>
#endif
#include <kstream.h>
#include <kfilestream.h>

#endif

#if defined(PLATFORM_UNIX) && !defined(KTL_BUILD)
#include "palundef.h"
#endif
#pragma warning(pop)

