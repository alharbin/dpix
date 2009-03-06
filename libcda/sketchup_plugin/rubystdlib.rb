if not file_loaded?("rubystdlib.rb")
    processor, platform, *rest = RUBY_PLATFORM.split("-")

    case platform
        when /darwin.*/
        $: << "/usr/lib/ruby/1.8" << "/usr/lib/ruby/site_ruby/1.8"
        
        when /mswin.*/
        $: << "C:/ruby/lib/ruby/1.8" << "C:/ruby/lib/ruby/site_ruby/1.8"
        end
    end
    
file_loaded("rubystdlib.rb")
