N3twork Triumph project branch of thrift compiler.

# Building

(Note: A build is checked into the triumph repo, so you only need to do this if you want to make further modifications to the compiler.)

```
brew install bison # known to work w/ v3.4.1
cd compiler/cpp
mkdir cmake-build && cd cmake-build
cmake ..
make
```

# Publishing `lib/java` to our mvn repo

Latest version published 4/7/2020 from the `n3/tri` branch: `0.12.1-N3-TRIUMPH.1`.
```
cd lib/java
./gradlew -PmavenUser=<mvnRepUn> -PmavenPassword=<mvnRepoPw> -Pmaven-repository-url=https://nexus.n3twork.com/repository/maven-releases/ -Prelease=true -Pthrift.version=<newVersion> uploadArchives
```
Once this is done, the artifact can be referenced in pom.xml like this:
```
<dependency>
    <!-- Thrift 0.12.1 with support for the N3TWORK TTiny protocol. -->
    <groupId>org.apache.thrift</groupId>
    <artifactId>libthrift</artifactId>
    <version>0.12.1-N3-TRIUMPH.1</version>
</dependency>
```

# Thrift Language Changes & Additions

Thrift syntax:

- Field annotations come before the type name, not at end of declaration
 * old: `1: required int foo (annotation)`
 * new: `1: required (annotation) int foo`
- Struct/enum/exception annotations come after the type name, not at the end of the type declaration:
 * old: `struct Foo {...} (annotation)`
 * new: `struct Foo (annotation) {...}`
- `insert "FNAME"` statement -- acts as though filename were copy-and-pasted in place of the insert statemnt (like C's #include)

C#:

- typedefs generate unique types (as single-item c# structs)
 * to disable this behavior, use the `alias` attribute:
	 * `typedef (alias) string FooId // will use string in generated source`
 * use `(nostr)` attribute to disable auto-generated ToString method
 * use `(nocast)` attribute to disable auto-generated explicit cast operators
- thrift structs can be generated as c# structs (rather than classes) by using the attribute  `csharp.struct`
 * LIMITATION: default values for structs is not supported (completely fixable, just have to work around a quirk of c#)
 * NB. an optional field referencing a c# struct will still have reference semantics, via wrapping the struct in a single-item "Ref" class)
- support for sum-types -- annotate a struct with `csharp.oneOf = "<interface>"` to generate a c# struct that has a single field, of that interface type. every field of thrift struct must be optional, and exactly one field should be set.
 * LIMITATION: which field is set is (lazily) determined by doing an "as" cast, so this *only works if all types are distinct*; TODO: remember what was set using the field identifier. (faster, more robust.)
- no __isset generated for optional value types (why: less memory cost; in the rare case you need the functionality, can recreate manually by including explicit companion "isset" variable)
- tab indents instead of spaces (to match our coding conventions)
- various restrictions for cases we are not using, so are untested...
	
Java:

- `java.oneOf` annotation to generate sum types
- don't generate a deepCopy method (it complicates sum-type usage)
 * (specifically then we need the interface type to implement deepCopy, which seems annoying, but would be no big deal if really needed)
	
Python:

- Add an element to thrift_spec identifying the class of enum fields
- Generate wrapper types for typedefs
	
General:

- `-drop ANNOTATION` command line flag to drop types/fields matching the given annotation
- `-fast` flag to reuse parse results when processing the same file included multiple times
 * seems to work well for python and c#
 * java for some reason alwaLangys generates fully-qualified type names

# Restrictions

- c#: optional, non-nullable values are always written when saving thrift
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

- in general there are tons of features of thrift we're not using that may have been broken by our mods
