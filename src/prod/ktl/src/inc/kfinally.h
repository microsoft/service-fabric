/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    kfinally

    Description:
      Kernel Tempate Library (KTL): KFinally object

      An object which invokes an arbitrary lambda at destruct-time,
      used for local cleanup.  Similar to a __finally block with SEH.

    History:
      raymcc          16-Aug-2010         Initial version.
      raymcc           8-Feb-2011         Vastly simpler rewrite

--*/

#pragma once


// __KFinally
//
// This is used by the KFinally macro to define a lambda which auto-executes
// when the current code block is exited, in a manner similar to try/finally in SEH.
//
// The __KFinally struct is not used directly.  Rather, the user declares
// using the KFinally macro:
//
// KFinally([&](){ <code goes here> });
//
// All types of lambda captures are supported.
//

template <class T>
struct __KFinally
{
  T _Lam;

  __KFinally(T && Src) : _Lam(Ktl::Move(Src)){}

  ~__KFinally()
  {
    _Lam(); // when the auto goes out of scope, invoke the lambda via pointer
  }
};

template<class T>
auto __MakeKFinally(T && Src)->__KFinally<T>  // Receive the lambda body by ref and construct a __KFinally around it
{
  return __KFinally<T>(Ktl::Move(Src)); // Convert to an anonymous auto
}

#define KFinally \
   auto K_MAKE_UNIQUE_NAME(__KFinally_Instance) = __MakeKFinally

