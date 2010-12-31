#!/usr/bin/env ruby
#encoding:utf-8
require 'zmq'
require 'yajl'

class ZbrokerClient
   Read = 1
   Write = 2

   MinError       = 500
   UnknownCommand = 500
   Unimplemented  = 501
   RequestTooLong = 502
   NoMoreItem     = 503
   ErrorMax = NoMoreItem+1
   OK             = 200

   class NoMoreItemError < StandardError ; end
   class ZbrokerClientError < StandardError ; end
   @@context = nil
   def self.context( num =1 )
      return @@context if @@context 
      @@context = ZMQ::Context.new(num)
   end
   def initialize( options )
      @host = options[:host]
      @port = options[:port] || 5666
      @socket = ZbrokerClient.context.socket(ZMQ::REQ)
      @socket.connect("tcp://#{@host}:#{@port}")
   end
   def pack_request( cmd,data=nil)
      pkg = {:cmd=>cmd,}
      pkg[:data] = data if data
      Yajl::Encoder.encode(pkg)
   end
   def request( cmd ,data = nil)
      @socket.send(pack_request(cmd,data))
      response
   end
   def response
      str = @socket.recv
      rep_code = str[str.size-4..-1].unpack("I")
      str.chomp!(rep_code)
      if OK == rep_code
         return str
      else
         raise NoMoreItemError,"no more items" if rep_code == NoMoreItem
         raise ZbrokerClientError,rep_code.to_s
      end
   end
   def read
      request(Read)
   end
   def write( data )
      request(Write,data)
   end
end
