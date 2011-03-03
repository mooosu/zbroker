require 'flogger'
require 'm2utils'
require 'yajl'
require 'socket'
require File.expand_path(File.dirname(__FILE__)+'/common.rb')
require File.expand_path(File.dirname(__FILE__)+'/packet.rb')
module Zbroker
   class Proxy
      include Flogger::Loggable
      # options={
      #  :host=>"localhost",
      #  :port=>2765,
      # }
      def initialize(options,purpose)
         @host = options[:host]
         @port = options[:port] || 2765
         @socket = nil
         @packet_id = nil
         @purpose = purpose
         @default_open={
            :parameters=>{:skip=>0,:limit=>500},
            :upsert=>false,:multi=>false
         }.freeze
      end
      # options={
      #  :host=>"192.168.1.86",
      #  :port=>27017,
      #  :database=>"zbroker",
      #  :collection=>"broker",
      #  :conditions=>{'brand'=>'Nokia'},
      #  :fields=>['brand','status'],
      # }
      def open( options )
         raise ZbrokerError,"Alread Opened" if @socket
         @socket = TCPSocket.new(@host,@port)
         body= @default_open.merge(options)
         body[:purpose] = @purpose
         new_ops = {:cmd=>Command::Open,:body=>body}
         str = Yajl::Encoder.encode(new_ops)
         packet = Packet.new(str)
         packet,response = send_packet(packet)

         if response["response"] == StatusCode::OK
            @packet_id =  packet.packet_id
         else
            raise ZbrokerError,ZbrokerError.error_message(response["response"])
         end
      end
      def recv_packet
         data = @socket.recv(Limit::HeaderSize)
         if data.size == 0
            raise ZbrokerError,"Broker was dead"
         end
         size = data.to_i
         str = ''
         tmp = nil
         while size > 0
            tmp = @socket.recv(size)
            break if tmp.size == 0
            str +=  tmp
            size -= tmp.size
         end
         packet = Packet.new(str)
         packet.decode_header
         packet
      end
      def send_packet(packet)
         @socket.send(packet.data,0)
         ret = recv_packet
         response =Yajl::Parser.parse( ret.body )
         [ret,response]
      end
      def make_packet( ops )
         str = Yajl::Encoder.encode(ops)
         packet = Packet.new(str)
         packet.packet_id = @packet_id
         packet
      end
      def read
         new_ops = {:cmd=>Command::Read}
         packet = make_packet(new_ops)
         elapsed = Time.now
         packet,response = send_packet(packet)
         ret = nil
         if response["response"] == StatusCode::OK
            ret = response['docs']
         else
            raise ZbrokerError,ZbrokerError.error_message(response["response"])
         end
         elapsed = Time.now - elapsed
         debug(' read elapsed: ' + elapsed.to_s )
         ret
      end
      def write(docs)
         new_ops = {:cmd=>Command::Write}
         docs_hash={}
         docs.each_with_index{|doc,i|
            docs_hash[i.to_s] = doc
         }
         new_ops[:body]={:docs=>docs_hash}
         packet = make_packet(new_ops)
         packet,response = send_packet(packet)
         if response["response"] == StatusCode::OK
            true
         else
            raise ZbrokerError,ZbrokerError.error_message(response["response"])
         end
      end
      def rewind
         new_ops = {:cmd=>Command::Rewind}
         packet,response = send_packet(make_packet(new_ops))
         if response["response"] == StatusCode::OK
            true
         else
            raise ZbrokerError,ZbrokerError.error_message(response["response"])
         end
      end
   end
   class ReadProxy < Proxy
      def initialize(options)
         super(options,Purpose::Read)
      end
      def write(*args)
         raise ZbrokerError,"Can not write to a readonly proxy"
      end
   end
   class WriteProxy < Proxy
      def initialize(options)
         super(options,Purpose::Write)
      end
      def read(*args)
         raise ZbrokerError,"Can not read from a write only proxy"
      end
      def rewind(*args)
         raise ZbrokerError,"Can not rewind a write only proxy"
      end
   end
end
