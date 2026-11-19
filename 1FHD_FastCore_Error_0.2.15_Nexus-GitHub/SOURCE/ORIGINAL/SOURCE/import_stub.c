// Link-time fixtures for GNU ld to emit a Kernel32 import library.
// This object and the stub DLL never enter the shipped plugin.
typedef unsigned int Word32;

int __attribute__((stdcall)) protect(void *address, Word32 size, Word32 flags, Word32 *old)
{
    (void)address;
    (void)size;
    (void)flags;
    (void)old;
    return 0;
}

int __attribute__((stdcall)) flush(void *process, const void *address, Word32 size)
{
    (void)process;
    (void)address;
    (void)size;
    return 0;
}

int __attribute__((stdcall)) pin(Word32 flags, const void *address, void **module)
{
    (void)flags;
    (void)address;
    (void)module;
    return 0;
}
