JIT expr/expr~/fexpr~
===

Just-in-time (JIT) clones of the [expr/expr~/fexpr~](http://yadegari.org/expr/expr.html) [pure data](http://puredata.info/) objects.

Requirements
---

* [cmake](https://cmake.org/) 3.0 or greater
* [bison](https://www.gnu.org/software/bison/) tested with 3.0.4, didn't work with 2.3 on osX
* [flex](https://github.com/westes/flex) tested with 2.6.0
* [llvm](http://llvm.org/) 5.0
* [pure-data](https://puredata.info/) headers
* [pd.build](https://github.com/pierreguillot/pd.build)
	* it is already set up as a submodule, remember `git submodule init && git submodule update`

Build
---

### Linux

`./configure && make pd`

### osX

The build in flex header is old, I needed to target the one I installed with homebrew which was installed in */usr/local/opt/flex/include/*

* make the cmake build directory:
	* `mkdir build/ && cd build`
* let cmake know where your flex headers and m_pd.h are:
	* `cmake -DCMAKE_CXX_FLAGS="-I/usr/local/opt/flex/include/ -I/Applications/Pd-0.48-0.app/Contents/Resources/include/" ..`

### Windows

??


Install
---

* Linux:
	* `sudo make install`

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

