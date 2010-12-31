require './zbroker_client'
describe ZbrokerClient do
   it "should get error" do
      client = ZbrokerClient.new(:host=>"localhost",:port=>5666)
      client.request(123).should match(/Command not found/)
   end
   it "should get no more items" do
      client = ZbrokerClient.new(:host=>"localhost",:port=>5666)
      0.upto(300) do|i|
         puts "read #{i} ==> #{client.read}"
      end
   end
end
