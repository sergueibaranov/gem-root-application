echo "Use: provide the path to source directory as argument"
echo "Compiling ROOT Event library ..."
cd $1
g++  -O2 -Wall -fPIC -pthread -m64 -I /usr/include/root -c Event.cxx
rootcint -f EventDict.cxx -c Event.h EventLinkDef.h
g++  -O2 -Wall -fPIC -pthread -m64 -I /usr/include/root -c EventDict.cxx
g++ -shared -O2 -m64 Event.o EventDict.o -o  libEvent.so
cd -
echo "libEvent.so done"
