SOURCES=zbroker.cc
LIBS=-lpcre -lpcre++ -lscws -lyaml-cpp
CXXFLAGS=-I/usr/local/include -L/usr/local/lib  -O0 -ggdb3 

includedir = $(prefix)/include/texting
include_HEADERS=*.h
if SHARED
shareddir = lib
shared_PROGRAMS = libztexting.so
libztexting_so_SOURCES = $(SOURCES)
libztexting_so_CXXFLAGS =$(CXXFLAGS) $(SHARED_CXXFLAGS) 
libztexting_so_LDFLAGS = $(SHARED_LDFLAGS) 
libztexting_so_LDADD= $(LIBS)
endif

if STATIC
lib_LIBRARIES = libztexting.a
libztexting_a_SOURCES = $(SOURCES)
libztexting_a_CXXFLAGS =$(CXXFLAGS) $(SHARED_CXXFLAGS)  $(LIBS)
endif

