#include "../kdelegate/KDelegateBasicTests.cpp"
#include "../kdelegate/KDelegateScaleTests.cpp"
#include "../kdelegate/KDelegateInheritanceTests.cpp"


#if CONSOLE_TEST
int
#if !defined(PLATFORM_UNIX)
main(int argc, WCHAR* args[])
{
#else
main(int argc, char* cargs[])
{
	CONVERT_TO_ARGS(argc, cargs)
#endif
#if defined(PLATFORM_UNIX)
    status = KtlTraceRegister();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to KtlTraceRegister\n");
        return(status);
    }
#endif

			
	KInvariant(NT_SUCCESS(ConstructorTests(argc - 1, args)));
	KInvariant(NT_SUCCESS(BindTests(argc - 1, args)));
	KInvariant(NT_SUCCESS(RebindTests(argc - 1, args)));
	KInvariant(NT_SUCCESS(FunctionScaleTests(argc - 1, args)));
	KInvariant(NT_SUCCESS(FunctionObjectScaleTests(argc - 1, args)));
	KInvariant(NT_SUCCESS(InheritanceTests(argc - 1, args)));

#if defined(PLATFORM_UNIX)
    KtlTraceUnregister();
#endif  
	
	
	return 0;
}
#endif

