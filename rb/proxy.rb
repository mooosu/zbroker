require 'flogger'
require 'm2utils'
require 'yajl'
require 'socket'
require File.expand_path(File.dirname(__FILE__)+'/common.rb')
require File.expand_path(File.dirname(__FILE__)+'/packet.rb')
require 'pp'
module Zbroker
   class Proxy
      include Flogger::Loggable
      # options={
      #  :host=>"localhost",
      #  :port=>2765,
      # }
      def initialize(options,purpose)
         @socket = TCPSocket.new(options[:host],options[:port] || 2765)
         @packet_id = nil
         @purpose = purpose
         @default_open={
            :parameters=>{:skip=>0,:limit=>500,:queue_size=>500},
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
      def read
         new_ops = {:cmd=>Command::Read}
         str = Yajl::Encoder.encode(new_ops)
         packet = Packet.new(str)
         packet.packet_id = @packet_id
         packet,response = send_packet(packet)
         if response["response"] == StatusCode::OK
            response['docs']
         else
            raise ZbrokerError,ZbrokerError.error_message(response["response"])
         end
      end
      def write(docs)
         new_ops = {:cmd=>Command::Write}
         docs_hash={}
         docs.each_with_index{|doc,i|
            docs_hash[i.to_s] = doc
         }
         new_ops[:body]={:docs=>docs_hash}
         str = Yajl::Encoder.encode(new_ops)
         packet = Packet.new(str)
         packet.packet_id = @packet_id
         packet,response = send_packet(packet)
         if response["response"] == StatusCode::OK
            packet.packet_id
         else
            raise ZbrokerError,ZbrokerError.error_message(response["response"])
         end
      end
   end
   class ReadProxy < Proxy
      def initialize(options)
         super(options,Purpose::Read)
      end
      def write
         raise ZbrokerError,"Can not write to a readonly proxy"
      end
   end
   class WriteProxy < Proxy
      def initialize(options)
         super(options,Purpose::Read)
      end
      def read
         raise ZbrokerError,"Can not read from a write only proxy"
      end
   end
end
