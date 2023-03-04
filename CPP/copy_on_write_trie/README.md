# Copy-on-Write Trie

**Copy-on-write trie** is a data structure that can be used to implement dictionary, set, map, and other data structures. It uses the concepts of shared structure and delayed copying to **reduce memory usage** and **copying overhead**.

## Features

- **High-performance** implementation of copy-on-write trie
- Support for dictionary, set, map, and other data structures
- Efficient insertion, lookup, and deletion operations
- **Thread-safe** implementation with support for concurrent access and modification

## Test

I use **Google Test** framework for the testing of this project.
However, I **DO NOT** include the `gtest` library and the `test` source file in this repo, if you want to test it, you can configure it locally yourself.

## Disclaimer

This repository contains **ONLY** the source code for the copy-on-write trie implementation. It **DOES NOT** include any dependencies or libraries, and users are responsible for configuring the code to work in their own environment. This implementation is not intended for commercial use, and the author assumes **NO** responsibility for any consequences resulting from its use.

## License

This implementation is released under the MIT License. See LICENSE file for details.

Please feel free to modify and improve this implementation as needed, and let me know if you have any questions or suggestions. Thank you for using copy-on-write trie!
