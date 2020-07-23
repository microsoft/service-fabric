# Steps in customizing this for your own branch:
#
# 1. Change PROJECT_NAME to the directory under which your code lives in services. Example include winfab or nfc or ktl.
#
# 2. Change WEX_PATH, WEX_LIB_PATH and WEX_MANAGED_REFS to point to the locations of TAEF headers and libs
#
# 3. Change KTL_SRC_ROOT and KTL_OBJECT_ROOT to point to the location where KTL is mapped into your client view

#################################################################################
# Beginning of configurable section

#
# Change project name to match the directory location of your project. 
#
PROJECT_NAME=winfab

#
# Configure location of TAEF
#
WEX_PATH=$(BASEDIR)\services\$(PROJECT_NAME)\imports\taef\amd64\Include;

WEX_LIB_PATH = $(BASEDIR)\services\$(PROJECT_NAME)\imports\taef\amd64\Library\x64

WEX_MANAGED_REFS= \
    $(BASEDIR)\services\$(PROJECT_NAME)\imports\taef\amd64\x64\TestExecution\wex.logger.interop.dll; \
    $(BASEDIR)\services\$(PROJECT_NAME)\imports\taef\amd64\x64\TestExecution\te.managed.dll;

#
# Configure location of xmldiffpath.dll
#
XMLDIFF_PATH= $(BASEDIR)\services\$(PROJECT_NAME)\external\xmldiff

#
# Configure location of KTL sources
#
KTL_SRC_ROOT=$(BASEDIR)\services\$(PROJECT_NAME)\prod\shared\ktl
KTL_OBJECT_ROOT=$(OBJECT_ROOT)\services\$(PROJECT_NAME)\prod\shared\ktl

# End of configurable section
#################################################################################


KTL_TRACE_INCS = \
  $(KTL_OBJECT_ROOT)\mc\user\$(O); \
  $(KTL_OBJECT_ROOT)\mc\kernel\$(O);

KTL_INCS = $(KTL_SRC_ROOT)\inc;\
           $(INTERNAL_SDK_INC_PATH);\
           $(INTERNAL_SDK_INC_PATH)\minwin;\
           $(DDK_INC_PATH);\
           $(KTL_TRACE_INCS);\
           $(KTL_OBJECT_ROOT)\StatusCodes\$(O)

KTL_USER_LIBS = \
  $(KTL_OBJECT_ROOT)\src\user\$(O)\kallocator.obj \
  $(KTL_OBJECT_ROOT)\src\user\$(O)\ktluser.lib

KTL_KERNEL_LIBS = \
  $(KTL_OBJECT_ROOT)\src\kernel\$(O)\kallocator.obj \
  $(KTL_OBJECT_ROOT)\src\kernel\$(O)\ktl.lib

_NT_TARGET_VERSION=$(_NT_TARGET_VERSION_WIN8)

KTL_USER_PROJECT_MK=$(KTL_SRC_ROOT)\UserProject.mk


MSC_WARNING_LEVEL = /W4 /WX
#
# Disable harmless /W4 warnings
#
# C4481: nonstandard extension used: override specifier 'override'
# C4239: nonstandard extension used : 'token' : conversion from 'type' to 'type'
# C4127: conditional expression is constant
#
C_DEFINES=$(C_DEFINES) /wd4239 /wd4481 /wd4127

!IF "$(TARGETTYPE)" != "DRIVER"
#User Mode Target: Auto include libs
TARGETLIBS = $(TARGETLIBS) \
    $(SDK_LIB_PATH)\shlwapi.lib
!ENDIF

NO_WCHAR_T=1

