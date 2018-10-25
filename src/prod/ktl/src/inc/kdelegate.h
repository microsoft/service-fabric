/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    kdelegate

    Description:
      Kernel Tempate Library (KTL): KDelegate

      Defines general purpose typeless member-function invoker similar to C# delegates. These are compatible
      with both per-instance member functions and static functions.

    History:
      raymcc          05-Oct-2010         Initial version
	  raymcc		  03-Nov-2010		  Updates for multiple inheritance support

    To use this, in practice you will declare a typedef representing a callback of a particular
    signature using Boost/STL-like syntax:

    typedef KDelegate<int(int,int)> MyCallbackType;

    The type 'MyCallbackType' can take pointer-to-member or pointers to static functions (class members
    or global C functions).

    The delegate can be bound to its target at construct-time or by a later explicit call to
    Bind.   To invoke, operator() is used on the delegate, so it looks like a function call.
    For example, to bind a pointer-to-member-function of a specific instance:

       MyCallbackType callback(&object, &object::MemberFunction);  // Bind at construct-time
       ...
       callback.Bind(&someOtherObject, &OtherClass::Member);        // Bind via explicit call
       ...
       int Result = callback(10, 20);    // Invokes the per-instance member

    Type erasure is used; the delegate is with any class member function of the same signature.
    It is legal and safe to rebind any delegate using Bind at any time.

--*/

#pragma once

// Determine the pointer sizes for the current build target architecture
// Pointers-to-member of classes with multiple inheritance are larger
//
class __single_inheritance __KDelegAnonymousSingle;
class __multiple_inheritance __KDelegAnonymousMulti;
const int __KDelegSingleInhMemPtrSize = sizeof(void (__KDelegAnonymousSingle::*)());
const int __KDelegMultiInhMemPtrSize = sizeof(void (__KDelegAnonymousMulti::*)());

// Base template
template <class Signature>
struct KDelegate;


// Specialization for R func(P0, P1)
//
template <class R, typename... Ts>
struct KDelegate<R(Ts...)>
{
private:
    typedef R(KDelegate::*MethodType)(Ts...);
    typedef R(*StaticPtr)(Ts...);
    KDelegate* _Obj;
    union
    {
        MethodType _MethodPtr;
        StaticPtr  _StaticMethod;
    }  _U;

public:
    operator BOOLEAN() const { return _U._MethodPtr ? TRUE : FALSE; }
    void     Reset() { _U._MethodPtr = nullptr; _Obj = nullptr; }
    BOOLEAN  IsStatic() { return _Obj ? FALSE : TRUE; }

#if !defined(PLATFORM_UNIX)

    template<size_t n, class Src>
    struct Adjuster
    {
        void Adjust(Src*&, R(Src::*In)(Ts...), MethodType& Out) { int x[-1]; }
    };
    
    template<class Src>
    struct Adjuster<__KDelegSingleInhMemPtrSize, Src>
    {
        void Adjust(Src*&, R(Src::*In)(Ts...), MethodType& Out)
        {
            Out = MethodType(In);
        }
    };
    template<class Src>
    struct Adjuster<__KDelegMultiInhMemPtrSize, Src>
    {
        using MemFuncPtr = R(Src::*)(Ts...);
        void Adjust(Src*& SrcThis, MemFuncPtr In, MethodType& Out)
        {
            union
            {
                MemFuncPtr InPtr;
                struct
                {
                    MethodType _Normalized;
                    int _ThisOffset;
                }   S;
            } Caster;
            
            Caster.InPtr = In;
            
            UCHAR* p = (UCHAR*) SrcThis;
            p += Caster.S._ThisOffset;
            SrcThis = (Src *)p;
            
            Out = Caster.S._Normalized;
        }
    };
#else

#if __KDelegSingleInhMemPtrSize != __KDelegMultiInhMemPtrSize
#error "Expected __single_inheritance and __multiple_inheritance pointers to be of equal size on this platform"
#endif
    template<size_t n, class Src>
    struct Adjuster
    {
        void Adjust(Src*&, R(Src::*In)(Ts...), MethodType& Out) { int x[0]; }
    };
    
    template<class Src>
    struct Adjuster<__KDelegSingleInhMemPtrSize, Src>
    {
        void Adjust(Src*&, R(Src::*In)(Ts...), MethodType& Out) 
        {
            Out = MethodType(In);
        }
    };

#endif

    void Bind(R(*Method)(Ts...))
    {
        _Obj = nullptr;
        _U._StaticMethod = reinterpret_cast<StaticPtr>(Method);
    }

#ifndef _M_IX86
    void Bind(PVOID Obj, R (*Method)(PVOID, Ts...))
    {
        _Obj = reinterpret_cast<KDelegate*>(Obj);
        _U._StaticMethod = reinterpret_cast<StaticPtr>(Method);
    }
#endif

        
    template <class Src>
    void Bind(Src* Obj, R(Src::*Method)(Ts...))
    {
        MethodType AdjustedMethod;
        Adjuster<sizeof(Method), Src> Adj;
        Adj.Adjust(Obj, Method, AdjustedMethod);
        _Obj = reinterpret_cast<KDelegate*>(Obj);
        _U._MethodPtr = reinterpret_cast<MethodType>(AdjustedMethod);
    }

    template <class Src, class Base>
    void Bind(Src* Obj, R(Base::*Method)(Ts...))
    {
        MethodType AdjustedMethod;
        Adjuster<sizeof(Method), Src> Adj;
        Adj.Adjust(Obj, Method, AdjustedMethod);
        _Obj = reinterpret_cast<KDelegate*>(static_cast<Base*>(Obj));
        _U._MethodPtr = reinterpret_cast<MethodType>(AdjustedMethod);
    }

    NOFAIL KDelegate()
    {
        _U._MethodPtr = nullptr;
        _Obj = nullptr;
    }
    NOFAIL KDelegate(R(*Method)(Ts...))
    {
        _Obj = nullptr;
        Bind(Method);
    }
    NOFAIL KDelegate(PVOID Obj, R(*Method)(PVOID Obj, Ts...))
    {
        _U._MethodPtr = nullptr;
        Bind(Obj, Method);
    }
    template <class Src>
    NOFAIL KDelegate(Src* Obj, R(Src::*Method)(Ts...))
    {
        Bind(Obj, Method);
    }
    template <class Src, class Base>
    NOFAIL KDelegate(Src* Obj, R(Base::*Method)(Ts...))
    {
        Bind(Obj, Method);
    }

    // Execute
	R operator()(Ts... args)
	{
		// Check for static vs. per-instance execution
		return _Obj ? (_Obj->*(_U._MethodPtr))(args...) : _U._StaticMethod(args...);
	}

	R ConstCall(Ts... args) const
	{
		// Check for static vs. per-instance execution
		return _Obj ? (_Obj->*(_U._MethodPtr))(args...) : _U._StaticMethod(args...);
	}
};

