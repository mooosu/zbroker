require 'bson'
require File.expand_path(File.dirname(__FILE__)+'/proxy.rb')
module Zbroker
   class Client
      def initialize(options,mode=[:read])
         @mode = mode
         if mode.include?(:read)
            @read_proxy = Zbroker::ReadProxy.new(options)
            @can_read = true
         end
         if mode.include?(:write)
            @write_proxy= Zbroker::WriteProxy.new(options)
            @can_write = true
         end
      end
      def open(options)
         @read_proxy.open(options) if @can_read
         @write_proxy.open(options) if @can_write
      end
      def write_by_oid( oid , doc )
         case oid
         when Hash
            raise ZbrokerError,"No $oid" unless oid.has_key?('$oid')
            oid = BSON::ObjectId.from_string(oid["$oid"])
         when String
            oid = BSON::ObjectId.from_string(oid)
         when BSON::ObjectId
         else
            raise ZbrokerError,"Invalid oid type: #{oid.type}"
         end
         query = {'_id'=>oid.to_json}
         write(query,doc)
      end
      def write(query,doc)
         raise ZbrokerError,"Not for write mode" unless @can_write
         queries = [{:query=>query,:doc=>doc}]
         @write_proxy.write(queries)
      end
      def read
         raise ZbrokerError,"Not for read mode" unless @can_read
         @read_proxy.read.values
      end
      def rewind
         raise ZbrokerError,"Not for read/rewind mode" unless @can_read
         @read_proxy.rewind
      end
   end
end

if __FILE__ == $0
   Flogger.on(Zbroker::ReadProxy,STDOUT)
   options={
      :host=>"192.168.1.86",
      :port=>27017,
      :database=>"zbroker",
      :collection=>"broker",
      :conditions=>{'brand'=>'Nokia'},
      :fields=>{'brand'=>1,'status'=>1},
   }
   config = {:host=>'localhost',:port=>2765}
   client = Zbroker::Client.new(config,[:read,:write])
   client.open(options)

   while(true)
      begin
         tmp = client.read
         updat_queries = []
         tmp.each{|doc|
            client.write_by_oid(doc['_id'],{"$set"=>{"rbclient"=>Time.now.to_s}})
         }
      rescue=>ex
         puts ex.message
      end
   end
end
