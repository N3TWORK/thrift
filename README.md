N3twork Triumph project branch of thrift compiler.

Changes:

C#:
	- tab indents
	- typedefs are represented to single-item structs
	- no getter/setters; just raw var variables
	- no SILVERLIGHT support
	
== Bugs ==

- typedefs of typedefs is probably broken (i.e, something like this:

```
typedef string FooId
typedef FooId BarId
```

- typdefs of structs might be broken
	

== Building ==

```
cd compiler/cpp
mkdir cmake-build && cd cmake-build
cmake ..
make
```
