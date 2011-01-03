import os

env = Environment()

env.Append( CPPPATH=[ "src","/nsourcecode/opensource/mongo-cxx-driver/mongo" ] )
env.Prepend( LIBPATH=["/nsourcecode/opensource/mongo-cxx-driver"] )

env.Prepend( LIBS=["libmongoclient.a"])

env.Append( CPPFLAGS=" -ggdb3 -O0" )

env.Append( LINKFLAGS=" -Wl,--as-needed -Wl,-zdefs " )


boostLibs = [ "thread" , "program_options" , "system","unit_test_framework" ]
otherLibs = ['zmq']
conf = Configure(env)
for lib in boostLibs:
    if not conf.CheckLib("boost_%s-mt" % lib):
        conf.CheckLib("boost_%s" % lib)
for lib in otherLibs:
   conf.CheckLib(lib)
   
allClientFiles = []
allClientFiles += Glob( "src/*.cc" )

env.Program( "src/zbroker" , allClientFiles )



# tests
clientEnv = env.Clone();
clientTests = []
clientTests += [ clientEnv.Program( "test/broker_test" , [ "test/broker_test.cc","src/broker.cc" ] ) ]
