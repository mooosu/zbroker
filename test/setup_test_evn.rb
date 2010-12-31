require 'pp'
require 'mongo'
require 'm2utils'
$cfg = {
   :host=>'localhost',
   :port=>7890,
   :database=>'zbot_development',
   :collection=>'download'
}
def run_bg_job( cmd )
   job = fork do
      exec({},cmd)
   end
   Process.detach(job)
end
def start_mongod
   if File.exists?("/tmp/7890.pid")
      pid = open("/tmp/7890.pid").read.to_i
      if pid > 0
         Process.kill("INT",pid)
      end
   end
   FileUtils.rm_rf("/tmp/7890db/")
   FileUtils.mkdir_p("/tmp/7890db/")
   run_bg_job("mongod --dbpath /tmp/7890db --port 7890 -vvvvv --logpath /tmp/7890.log --pidfilepath /tmp/7890.pid")
   sleep(1)
end
def init_data
   db =Mongo.establish_connection($cfg)
   coll = db.collection($cfg[:collection])
   puts "insert 256 docs"
   0.upto(256) do|i|
      doc = {:page_type=>2,:data=>"test_data_#{i}",:status=>123,:timestamp=>Time.now.to_s}
      print "#{i} "
      pp doc
      coll << doc
   end
end
start_mongod
init_data
