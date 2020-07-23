include_directories(".")

# To use this pal lib, include ${lib_Pal} in your target_link_libraries
set (lib_Pal "Pal" CACHE STRING "Pal library")

# Windows sources
set( WIN_SOURCES
    accctrl.cpp
    aclapi.cpp
    basetsd.cpp
    bldver.cpp
    comutil.cpp
    conio.cpp
    dbghelp.cpp
    driverspecs.cpp
    esent.cpp
    evntprov.cpp
    evntrace.cpp
    fwpmtypes.cpp
    fwpmu.cpp
    fwpvi.cpp
    guiddef.cpp
    http.cpp
    icftypes.cpp
    ifdef.cpp
    in6addr.cpp
    inaddr.cpp
    intrin.cpp
    intsafe.cpp
    iphlpapi.cpp
    ktmtypes.cpp
    lmaccess.cpp
    lmapibuf.cpp
    lmcons.cpp
    lm.cpp
    lmerr.cpp
    mbusafecrt.cpp
    mstcpip.cpp
    mswsock.cpp
    nt.cpp
    ntddk.cpp
    ntdef.cpp
    ntosp.cpp
    ntregapi.cpp
    ntrtl.cpp
    ntrtl_x.cpp
    ntstatus.cpp
    ntstrsafe.cpp
    nturtl.cpp
    ntverp.cpp
    objbase.cpp
    objidl.cpp
    pdh.cpp
    pdhmsg.cpp
    perflib.cpp
    pla.cpp
    profinfo.cpp
    psapi.cpp
    rpc.cpp
    rpcdce.cpp
    rpcndr.cpp
    rpcnterr.cpp
    rpcsal.cpp
    sal.cpp
    schannel.cpp
    sddl.cpp
    sdkddkver.cpp
    security.cpp
    shlobj.cpp
    Shlwapi.cpp
    specstrings.cpp
    sspi.cpp
    stdmacros.cpp
    stringapiset.cpp
    strsafe.cpp
    subauth.cpp
    synchapi.cpp
    Unknwn.cpp
    userenv.cpp
    wdm.cpp
    winbase.cpp
    wincon.cpp
    wincrypt.cpp
    windef.cpp
    windns.cpp
    windows.cpp
    winerror.cpp
    winhttp.cpp
    winioctl.cpp
    winnls.cpp
    winnt.cpp
    winperf.cpp
    winreg.cpp
    winsock2.cpp
    winsvc.cpp
    winternl.cpp
    winuser.cpp
    wmistr.cpp
    ws2def.cpp
    ws2ipdef.cpp
    ws2tcpip.cpp
    wtypes.cpp
    wudfwdm.cpp
    xmllite.cpp
)

# Utilities for pal
set( UTL_SOURCES
    util/pal_hosting_util.cpp
    util/pal_string_util.cpp
    util/pal_time_util.cpp
    util/pal_ktl_util.cpp
)

# Pal sources
set ( PAL_SOURCES
    internal/pal_common.cpp
    internal/pal.cpp
    internal/palrt.cpp
)

add_library(objects_Win OBJECT ${WIN_SOURCES})
add_library(objects_Pal OBJECT ${PAL_SOURCES})
add_library(objects_Utl OBJECT ${UTL_SOURCES})
add_library(${lib_Pal} STATIC $<TARGET_OBJECTS:objects_Pal>
                              $<TARGET_OBJECTS:objects_Utl>
                              $<TARGET_OBJECTS:objects_Win>)
target_link_libraries(${lib_Pal}
    minizip
    z
)

set_target_properties(${lib_Pal} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})
