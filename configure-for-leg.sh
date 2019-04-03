#!/bin/sh

#//Requires these

# brew install pkg-config
# brew install automake
# brew install boost
# brew install libevent
# brew install libtool
# brew install cmake
# brew install bison
# brew install openssl

# echo 'export LDFLAGS="-L/usr/local/opt/bison/lib"' >> ~/.profile 
# echo 'export PATH="/usr/local/opt/bison/bin:$PATH"' >> ~/.profile

#first
./bootstrap.sh

#then (paths to boost and libevent may differ)
# don't necessarily need to exclude all these language libs, but provided for limited build scope
PY_PREFIX=`pwd`/py-install ./configure --prefix=`pwd`/thrift-install --with-boost=/usr/local/Cellar/boost/* --with-libevent=/usr/local/Cellar/libevent/*  --without-cpp --without-qt4 --without-qt5 --without-c_glib --without-erlang --without-perl --without-php --without-php_extension --without-ruby --without-haskell --without-go --without-d --without-openssl --without-nodejs --without-lua

#then
#cd compiler/cpp; ./cmakebuild.sh
