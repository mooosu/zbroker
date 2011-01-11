require File.expand_path(File.dirname(__FILE__)+'/proxy.rb')
Flogger.on(Zbroker::ReadProxy,STDOUT)
client = Zbroker::ReadProxy.new(:host=>'localhost',:port=>2765)
options={
   :host=>"192.168.1.86",
   :port=>27017,
   :database=>"zbroker",
   :collection=>"broker",
   :conditions=>{'brand'=>'Nokia'},
   :fields=>{'brand'=>1,'status'=>1},
}

puts "request_id:" + client.open(options)
while(true)
   begin
      docs = client.read
      puts tmp = docs.values
      #puts client.write(docs)
   rescue=>ex
      puts ex.message
   end
end

