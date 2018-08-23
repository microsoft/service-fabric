/*++

    (c) 2012 by Microsoft Corp. All Rights Reserved.

    krtt.h

    Description:
      Kernel Tempate Library (KTL) Runtime typing

      This version is limited to a single inheritance chain for the type inquiry.
      While the user class itself can multiply inherit from other classes & templates, there can only be one inheritance
      chain from KRTT.

    History:
      raymcc          11-May-2012         Initial version.

--*/

#pragma once

/*  Sample usage

    class Base : public KRTT
    {
        K_RUNTIME_TYPED(Base);

    public:
    };

    class Derived : public Base
    {
        K_RUNTIME_TYPED(Derived);

    public:
    };

    (a) Note that only the basemost class derives from KRTT
    (b) All classes must have the K_RUNTIME_TYPED() macro in their public section, repeating the name of the current class

    To test an object for typing, use either a raw pointer or KSharedPtr<>

        Base* foo = ...
        if (is_type<Derived>(foo)) ...

        Base::SPtr foo2;
        if (is_type<Derived>(foo2)) ...


*/



// class KRTT
//
// Base class for any object needing runtime typing.
//
class KRTT
{
public:
    typedef const char* (KRTT::*TypeAddr)();
    const char* _TypeInquiry_(){ return __FUNCTION__; }

    virtual BOOLEAN
    IsType(
      __in TypeAddr Candidate
      )
      {
          // This is the last function called in the chain, so if there is no match, it always return FALSE

         if (Candidate == (KRTT::TypeAddr) &KRTT::_TypeInquiry_) return TRUE;
         else return FALSE;
      }
};


// Include this macro in the public section of any class needing runtime typing.
//
#define K_RUNTIME_TYPED(Class) \
    public:\
    const char* _TypeInquiry_(){ return __FUNCTION__; }\
    BOOLEAN \
    IsType( \
      __in KRTT::TypeAddr Candidate\
      ) override \
      { \
         if (Candidate == (KRTT::TypeAddr) &Class::_TypeInquiry_) return TRUE;\
         else return __super::IsType(Candidate); \
      }\


// Cast operator to check the type using a raw ptr
//
template <class T>
BOOLEAN is_type(KRTT* Candidate)
{
    KRTT::TypeAddr TRequired = (KRTT::TypeAddr) &T::_TypeInquiry_;
    return Candidate->IsType(TRequired);
}



// Cast operator to check the type of an SPtr
//
template <class T, class T2>
BOOLEAN is_type(KSharedPtr<T2>& SCandidate)
{
    KRTT* Candidate = SCandidate.RawPtr();
    KRTT::TypeAddr TRequired = (KRTT::TypeAddr) &T::_TypeInquiry_;
    return Candidate->IsType(TRequired);
}


