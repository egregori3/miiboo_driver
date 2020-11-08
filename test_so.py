from cffi_miiboo import ffi

lib = ffi.dlopen("/home/ubuntu/miiboo_driver/libmiiboo.so")
lib.Open(b"/dev/ttyUSB0")

lib.Write(b"f")

