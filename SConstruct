# vim:ft=python:
import os

zbroker = Environment()

cpp_path = ["src"]
mongo_root ="/nsourcecode/opensource/mongo-cxx-driver"
if not os.path.exists(mongo_root):
   mongo_root = "/sourcecode/mongo-cxx-driver"

zbroker.Append( CPPPATH=["src","/usr/local/include",mongo_root+"/mongo"])
zbroker.Prepend( LIBPATH=[mongo_root] )

zbroker.Prepend( LIBS=["libmongoclient.a","libztexting.a"])

zbroker.Append( CPPFLAGS=" -ggdb3 -O0" )

zbroker.Append( LINKFLAGS="-ggdb3 -Wl,--as-needed -Wl,-zdefs " )


boostLibs = [ "thread" , "program_options" , "system","unit_test_framework" ]
otherLibs = ['zmq','yaml-cpp','glog']
conf = Configure(zbroker)
for lib in boostLibs:
    if not conf.CheckLib("boost_%s-mt" % lib):
        conf.CheckLib("boost_%s" % lib)
for lib in otherLibs:
   conf.CheckLib(lib)

allClientFiles = ["src/zbroker_asio.cc","src/broker.cc","src/asio_handler.cc","src/asio_processor.cc","src/request_packet.cc","src/color.cc"]


Default(zbroker.Program( "src/zbroker" , allClientFiles ))


# tests

zbroker.Program( "test/broker_test" , [ "test/broker_test.cc","src/broker.cc" ,"src/color.cc"] ) 
zbroker.Program( "test/bson_test" , [ "test/bson_test.cc"] )
zbroker.Program( "test/config_test" , [ "test/config_test.cc"] )
zbroker.Program( "test/asio_processor_test" , ["src/broker.cc","src/asio_processor.cc", "test/asio_processor_test.cc","src/request_packet.cc","src/color.cc"] ) 
