This is the 'NVIDIA Shared' foundation library.

This code should not ever appear in any public headers or interfaces.

This library is primarily a platform abstraction layer.

It contains code to handle mutexes, atomic operations, etc.

It also handles some SIMD data types.

It provides math utility functions.

It implements a number of common container classes.

It manages trapping all memory allocations.

All projects should leverage against this foundation library to
perform these common functions.
