DESTDIR=
PREFIX=/usr/local
CXX=clang++
LD=$(shell xcrun -f clang++)
CXXFLAGS=-c -pipe -stdlib=libc++ -O2 -std=c++17 -arch x86_64 -mmacosx-version-min=10.13
LDFLAGS=-stdlib=libc++ -std=c++17 -Wl,-dead_strip -Wl,-headerpad_max_install_names -arch x86_64 -mmacosx-version-min=10.13
LDLIBS=

all: dylibbundler

dylibbundler:
	$(CXX) $(CXXFLAGS) -I./src ./src/Settings.cpp -o ./Settings.o
	$(CXX) $(CXXFLAGS) -I./src ./src/DylibBundler.cpp -o ./DylibBundler.o
	$(CXX) $(CXXFLAGS) -I./src ./src/Dependency.cpp -o ./Dependency.o
	$(CXX) $(CXXFLAGS) -I./src ./src/main.cpp -o ./main.o
	$(CXX) $(CXXFLAGS) -I./src ./src/Utils.cpp -o ./Utils.o
	$(LD) ${LDFLAGS} ${LDLIBS} -o ./dylibbundler ./Settings.o ./DylibBundler.o ./Dependency.o ./main.o ./Utils.o

clean:
	rm -f *.o
	rm -f ./dylibbundler

install: dylibbundler
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp ./dylibbundler $(DESTDIR)$(PREFIX)/bin/dylibbundler
	chmod 775 $(DESTDIR)$(PREFIX)/bin/dylibbundler

.PHONY: all clean install
