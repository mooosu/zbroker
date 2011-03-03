require File.expand_path(File.dirname(__FILE__) +"/../lib/zbroker.rb")
describe Zbroker::ReadProxy do
   it "should rewind" do
      Flogger.on(Zbroker::ReadProxy,STDOUT)
      options={
         :host=>"192.168.1.110",
         :port=>27017,
         :database=>"zbot_20110216",
         :collection=>"info",
         :conditions=>{'identifier'=>'360buy'},
      }
      config = {:host=>'localhost',:port=>2765}
      client = Zbroker::Client.new(config,[:read])
      client.open(options)
      docs1 = client.read
      docs2 = client.read
      client.rewind
      client.read[0,10].should == docs1[0,10]
   end
end
