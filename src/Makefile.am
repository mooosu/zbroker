MONGO_ROOT=/sourcecode/mongo-cxx-driver-v1.6
MONGO_CLIENT=$(MONGO_ROOT)/libmongoclient.a


SOURCES=zbroker.cc
INCFLAGS=-L/usr/local/lib -I$(MONGO_ROOT)/mongo
CXXFLAGS= -O0 -ggdb3  $(INCFLAGS)
LIBS= -lyaml-cpp -lboost_system -lboost_thread -lboost_program_options -lzmq
 

bin_PROGRAMS= zbroker

zbroker_SOURCES = $(SOURCES)
zbroker_CXXFLAGS= $(CXXFLAGS)
zbroker_LDADD= $(LIBS) $(MONGO_CLIENT)

