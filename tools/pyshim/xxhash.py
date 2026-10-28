"""Minimal stand-in for the python-xxhash module: XXH3_64 (seed 0) through the system libxxhash.

Original code (WORKLOG.md "Original code"): python-xxhash could not be installed on the build machine
(package download fails), and the ported futari-nx tools only use xxh3_64_intdigest / xxh3_64_hexdigest.
Put tools/pyshim on PYTHONPATH (the CMake build does) when the real module is missing.
"""
import ctypes
import ctypes.util

_lib = ctypes.CDLL(ctypes.util.find_library('xxhash') or 'libxxhash.so.0')
_lib.XXH3_64bits.restype = ctypes.c_uint64
_lib.XXH3_64bits.argtypes = (ctypes.c_char_p, ctypes.c_size_t)


def xxh3_64_intdigest(data, seed=0):
    if seed:
        raise NotImplementedError('seeded XXH3 is not needed by these tools')
    data = bytes(data)
    return _lib.XXH3_64bits(data, len(data))


def xxh3_64_hexdigest(data, seed=0):
    return '%016x' % xxh3_64_intdigest(data, seed)
