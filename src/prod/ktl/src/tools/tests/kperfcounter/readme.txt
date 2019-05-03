

To deploy the user mode EXE for testing:

[1] Copy the manifest (kperfcounter.um.man) to the same directory as the binary KPerfCounterTests.exe
[2] Create an en-us subdirectory and copy the KPerfCounterTests.exe.mui into that subdirectory. MUI files
    are binplaced into %_NTTREE%\loc\src\bin\KtlTest
[3] In ad ADMIN console window, run lodctr /m:kperfcounter.um.man
[4] Start the EXE and observe/debug the perf counters.
[5] When finished deinstall the manifest using unlodctr /m:kperfcounter.um.man


To deploy the test driver for debugging/testing

[1] Copy the SYS file KPerfCounterKernel.sys to wherever it will load from
[2] Copy the KPerfCounterKernel.sys.mui file to an en-us subdirect off of the directory in [1]. MUI files
    are binplaced into %_NTTREE%\loc\src\bin\KtlTest
[3] In an ADMIN console window, run lodctr /m:kperfcounter.km.man
[4] Run the kernel tests under control of the kernel debugger
[5] When tests are completed, deinstall the manifest using unlodctr /m:kperfcounter.km.man
