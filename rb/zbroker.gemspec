Gem::Specification.new do |s|
  s.name    = 'zbroker'
  s.version = '0.0.2'
  s.date    = '2011-03-03'
  s.rubyforge_project = "zbroker"
  
  s.summary = "A Ruby interface to the Zbroker"
  s.description = "A Ruby interface to the Zbroker."
  
  s.authors  = ['xiaowei']
  s.email    = 'xiaowei@hellom2.com'
  s.homepage = 'http://www.hellom2.com'
  
  s.has_rdoc = false

  s.files = Dir.glob("lib/*.rb")
  s.files += Dir.glob("lib/zbroker/*.rb")

  s.rdoc_options << '--title' << 'Rbhl' <<
    '--main' << 'README.rdoc' <<
    '--line-numbers'

  #s.extra_rdoc_files = ["README.rdoc", "LICENSE"]
end

