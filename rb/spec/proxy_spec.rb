require File.expand_path(File.dirname(__FILE__) +"/../rb/rbclient.rb")
describe Zbroker::ReadProxy do
   it "should rewind" do
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
      client = Zbroker::Client.new(config,[:read])
      client.open(options)
      docs1 = client.read
      docs2 = client.read
      client.rewind
      client.read.should == docs1
   end
end
