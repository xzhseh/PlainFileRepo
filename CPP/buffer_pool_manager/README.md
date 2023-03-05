# Buffer Pool Manager

The Buffer Pool Manager (BPM) is responsible for moving physical pages back and forth from disk to main memory. It allows a DBMS to support databases that are larger than the amount of memory available to the system. The BPM's operations are transparent to other parts of the system, and it provides good abstraction and encapsulation. The implementation has undergone basic stress testing.

## Features

- Implementation of a buffer pool manager for managing physical pages
- Support for moving pages back and forth from disk to main memory
- Transparent operations that are independent of other parts of the system
- LRU-K algorithm used as a cache replacement policy
- Support for multi-threaded access with Latch-based protection for internal data structures
- PageGuard encapsulation to provide safe access to Page objects, with automatic lock release using RAII mechanism to avoid potential deadlocks, as well as manual Unpin and Unlatch interfaces for added flexibility
- Basic stress testing with 5000 QPS achieved under conditions of 64-page buffer pool size and 16 concurrent threads accessing a single BPM instance

## Test

I use **Google Test** framework for the testing of this project.
However, I **DO NOT** include the `gtest` library and the `test` source file in this repo, if you want to test it, you can configure it locally yourself.

## Disclaimer

This repository contains **ONLY** the source code for the BufferPoolManager implementation. It **DOES NOT** include any dependencies or libraries, and users are responsible for configuring the code to work in their own environment. This implementation is not intended for commercial use, and the author assumes **NO** responsibility for any consequences resulting from its use.

## License

This implementation is released under the MIT License. See LICENSE file for details.

Please feel free to modify and use this implementation as needed. If you have any questions or suggestions, please let me know. Thank you for using this Buffer Pool Manager!
