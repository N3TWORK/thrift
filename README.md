N3twork Triumph project branch of thrift compiler.

Changes:

C#:
	- tab indents
	- typedefs are represented to single-item structs
	- no getter/setters; just raw var variables
	- no SILVERLIGHT support
	
== Restrictions ==

- No __isset for recording if optional, non-nullable types were provided

- Thus, optional, non-nullable values are always written when saving thrift
	- We could extend to only write if non-default-value
	
== Bugs / Untested ==

- typedefs of typedefs is probably broken (i.e, something like this:

```
typedef string FooId
typedef FooId BarId
```

- typdefs of structs might be broken

- service definitions is broken (needs to be updated for __isset)

- GetHashCode() / Equals() might be broken (untested with __isset)
	
== Building ==

```
cd compiler/cpp
mkdir cmake-build && cd cmake-build
cmake ..
make
```
