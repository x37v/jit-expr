JIT expr/expr~/fexpr~
===

Just-in-time (JIT) clones of the [expr/expr~/fexpr~](http://yadegari.org/expr/expr.html) [pure data](http://puredata.info/) objects.

Requirements
---

* [cmake](https://cmake.org/) 3.0 or greater
* [bison](https://www.gnu.org/software/bison/) tested with 3.0.4, didn't work with 2.3 on osX
* [flex](https://github.com/westes/flex) tested with 2.6.0 and 2.6.4
* [llvm](http://llvm.org/) 5.0
* [pure-data](https://puredata.info/) headers
* [pd.build](https://github.com/pierreguillot/pd.build)
	* it is already set up as a submodule, remember `git submodule init && git submodule update`

Build
---

### Linux

`./configure && make pd`

### Mac OsX

The build in flex header is old, I needed to target the one I installed with homebrew which was installed in */usr/local/opt/flex/include/* or with macports which wav installed in */opt/local/include/*

* make the cmake build directory:
	* `mkdir build/ && cd build`
* let cmake know where your (homebrew) flex headers and m_pd.h are, update this to reflect your paths:
  * `cmake -DFLEX_INCLUDE_DIR=/usr/local/opt/flex/include -DCMAKE_CXX_FLAGS="-I/Applications/Pd-0.48-1.app/Contents/Resources/src/" ..`
* 0.1.1 was built on a 10.10 machine with this command:
  * `cmake -DFLEX_INCLUDE_DIR=/opt/local/include -DCMAKE_CXX_FLAGS="-I/Applications/Pd*.app/Contents/Resources/src -I/opt/local/include -stdlib=libc++ -std=c++11" -DCMAKE_CXX_COMPILER=/opt/local/bin/clang++-mp-5.0 -DLLVM_DIR=/opt/local/libexec/llvm-5.0/lib/cmake/llvm/`

### Windows

Could use some help here..


Install
---

copy *build/jit_expr* to your appropriate *pd externals* directory

for example:

* Linux: `mkdir -p ~/.local/lib/pd/extra && cp -r build/jit_expr  ~/.local/lib/pd/extra/`
* MacOS: `cp -r build/jit_expr ~/Library/Pd/`
* Windows: not sure.. *%AppData%\Pd*

Notes
---

[godbolt](https://godbolt.org/) can be useful for finding what llvm instruction to use.
`-emit-llvm -S -O0`

Acknowledgements
---

* [Bison Flex C++ Template by Rémi Berson](https://github.com/remusao/Bison-Flex-CPP-template) was the starting point for the tokeniser and parser
* some of the LLVM code is based on the [llvm tutorial](https://llvm.org/docs/tutorial/)
* some of the expr pd code is based on the [original expr code](https://github.com/pure-data/pure-data) by [Shahrokh Yadegari](http://yadegari.org/) and [Miller Puckette](http://msp.ucsd.edu/software.html)

TODO
===

* remove overloaded virtual warning disable, if we can fix it
* convert more of the functions into LLVM code so it can be further optimized

Thanks
===

* Thanks to **Marco Matteo Markidis** for help building the osx external for 10.10+
