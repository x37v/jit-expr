JIT expr/expr~/fexpr~
===

Just-in-time (JIT) clones of the [expr/expr~/fexpr~](http://yadegari.org/expr/expr.html) [pure data](http://puredata.info/) objects.

Requirements
---

* [bison](https://www.gnu.org/software/bison/)
* [flex](https://github.com/westes/flex)
* [llvm](http://llvm.org/)
* [pure-data](https://puredata.info/) headers
* [pd-lib-builder](https://github.com/pure-data/pd-lib-builder)
	* it is already set up as a submodule, remember `git submodule init && git submodule update`

Build
---

`./configure && make pd`

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

