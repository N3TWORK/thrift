N3twork Triumph project branch of thrift compiler.

Changes:

Thrift syntax:
	- Field annotations come before the type name, not at end of declaration
		- old: `1: required int foo (annotation)`
		- new: `1: required (annotation) int foo`
	- Struct/enum/exception annotations come after the type name, not at the end of the type declaration:
		- old: `struct Foo {...} (annotation)`
		- new: `struct Foo (annotation) {...}`
	- `insert "FNAME"` statement -- acts as though filename were copy-and-pasted in place of the insert statemnt (like C's #include)

C#:
	- typedefs generate single-item c# structs
	- thrift structs can opt-in to be generated as c# structs (rather than classes) by using the "cs.struct" attribute
		- an *optional* field referencing a c# struct will still have reference semantics, via wrapping the struct in a single-item "Ref" class
	- no __isset generated for optional value types (why: less memory cost; in the rare case you need the functionality, can recreate manually by including explicit companion "isset" variable)
	- tab indents instead of spaces (to match our coding conventions)
	- various restrictions 
	
Python:
	- Add an element to thrift_spec identifying the class of enum fields
	
General:
	- `-drop ANNOTATION` command line flag to drop types/fields matching the given annotation
	- `-fast` flag to reuse parse results when processing the same file included multiple times
		- seems to work well for python and c#
		- java for some reason always generates fully-qualified type names

== Building ==

```
brew install bison # known to work w/ v3.4.1
cd compiler/cpp
mkdir cmake-build && cd cmake-build
cmake ..
make
```
	
== Restrictions ==

- optional, non-nullable values are always written when saving thrift
	- because we don't have isset
	- we could effectively get this behavior back by only writing non-default values
	- but we're not writing from c#, so we don't care
	
- "nullable", silverlight support dropped (why: implementation ease; we're not using the features)
	
== Bugs / Untested ==

- typedefs of typedefs is probably broken (i.e, something like this:

```
typedef string FooId
typedef FooId BarId
```

- service definitions is broken (needs to be updated for __isset)

- typdefs of structs is untested & probably broken

- GetHashCode() / Equals() generation for structs might be broken (untested with __isset) (we're not using this though)
