# https://realpython.com/python-bindings-overview/

# pip3 install cffi

import cffi
...
""" Build the CFFI Python bindings """
print("Building CFFI Module")
ffi = cffi.FFI()

print("Generating cdef from header")
h_file_name = "miiboo_driver.h"
with open(h_file_name) as h_file:
    ffi.cdef(h_file.read())

if 0:
    print("Compiling for API mode")
    ffi.set_source(
        "cffi_miiboo",
        # Since you're calling a fully-built library directly, no custom source
        # is necessary. You need to include the .h files, though, because behind
        # the scenes cffi generates a .c file that contains a Python-friendly
        # wrapper around each of the functions.
        '#include "miiboo_driver.h"',
        # The important thing is to include the pre-built lib in the list of
        # libraries you're linking against:
        libraries=["miiboo"],
        library_dirs=["./"],
        extra_link_args=["-lpthread"],
    )

if 1:
    print("Compiling for ABI mode - Generates cffi_miiboo.py")
    ffi.set_source("cffi_miiboo", None)

print("Compiling")
ffi.compile()

