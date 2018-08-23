

/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    kstringview.cpp

    Description:
      Kernel Tempate Library (KTL): KStringView - WCHAR types and Ansi types

      String manipulation & conversion utilities

    History:
      raymcc          27-Feb-2012         Initial version.

--*/

#include <ktl.h>
#include <ktrace.h>

// Generate Ansi source
#define K$AnsiStringTarget
#include "./kstringview.proto.cpp"
#undef K$AnsiStringTarget

// Generate WCHAR source
#include "./kstringview.proto.cpp"

