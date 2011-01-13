import os

env = Environment()

env.Append( CPPPATH=[ "src","/nsourcecode/opensource/mongo-cxx-driver/mongo" ] )
env.Prepend( LIBPATH=["/nsourcecode/opensource/mongo-cxx-driver"] )

env.Prepend( LIBS=["libmongoclient.a","libztexting.a"])

env.Append( CPPFLAGS=" -ggdb3 -O0" )

env.Append( LINKFLAGS="-ggdb3 -Wl,--as-needed -Wl,-zdefs " )


boostLibs = [ "thread" , "program_options" , "system","unit_test_framework" ]
otherLibs = ['zmq','yaml-cpp','glog']
conf = Configure(env)
for lib in boostLibs:
    if not conf.CheckLib("boost_%s-mt" % lib):
        conf.CheckLib("boost_%s" % lib)
for lib in otherLibs:
   conf.CheckLib(lib)
   
allClientFiles = []
allClientFiles += ["src/zbroker_asio.cc","src/broker.cc","src/asio_handler.cc","src/asio_processor.cc","src/request_packet.cc"]


clientEnv = env.Clone();

env.Program( "src/zbroker" , allClientFiles )


# tests
clientEnv.Program( "test/broker_test" , [ "test/broker_test.cc","src/broker.cc" ] ) 
clientEnv.Program( "test/bson_test" , [ "test/bson_test.cc"] )
clientEnv.Program( "test/config_test" , [ "test/config_test.cc"] )
clientEnv.Program( "test/asio_processor_test" , ["src/broker.cc","src/asio_processor.cc", "test/asio_processor_test.cc","src/request_packet.cc"] ) 
