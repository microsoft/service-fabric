#
# This is the project.mk definitions that are specific for user mode compilatin
#

SAFESEH           = 1
STL_VER           = 100
USE_MSVCRT        = 1
USE_NATIVE_EH     = 1
USE_RTTI          = 1
USE_STL           = 1
NO_BROWSER_FILE   = 1
NO_WCHAR_T        = 1
PRECOMPILED_CXX   = 1
UNICODE           = 1

C_DEFINES =\
    $(C_DEFINES)\
    -DUNICODE=1\
    -D_UNICODE=1\
    -DKTL_USER_MODE=1\
    -DKTL_MIXED_STL_ATL\
    -DJET_VERSION=0x0602

# /Zm specifies to use a larger heap needed for compiling lots of templates
USER_C_FLAGS =\
    $(USER_C_FLAGS)\
    /Zc:wchar_t\
    /Zc:forScope\
    /Zm400


!if !$(FREEBUILD)

# get correct stack in debugging
LINKER_NOICF= 1

# explicitly disable LTCG in chk builds to improve build times 
# (reduces memory consumption at the cost of less optimization)
FORCENATIVEOBJECT=1
!UNDEF LINK_TIME_CODE_GENERATION

!endif  # !$(FREEBUILD)

!if $(FREEBUILD)

# explicitly enable LTCG if it is a fre build 
# (also improves build times, prevents restarts that happen when LTCG
# is only enabled implicitly (by compiler options))
LINK_TIME_CODE_GENERATION=1

!endif # $(FREEBUILD)
