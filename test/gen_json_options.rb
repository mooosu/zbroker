require 'pp'
require 'yajl'
options = {
   :host=>'192.168.1.86',:port=>27017,
   :database=>'zbot_development',:collection=>'download',
   :skip=>100,:limit=>1000,:conditions=>{'brand'=>'Nokia'},
   :fields=>['brand','title'],
   :upsert=>false,:multi=>true,:queue_size=>100
}
pp Yajl::Encoder.encode(options)

update={
   :query=>{ :id=>{"$oid"=>"%s"} },
   :doc=>{:brand=>"updated",:status=>1234},
   :upsert=>false,:multi=>false
}
pp Yajl::Encoder.encode(update)

OPEN  = 100
READ  = 101
WRITE = 102

request={:cmd=>OPEN,:body=>options}

pp Yajl::Encoder.encode(request)

update_docs={
   :cmd=>WRITE,
   :body=>{:purpose=>2},
   :docs=>[update]
}

pp Yajl::Encoder.encode(update_docs)
