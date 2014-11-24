class UAServer < FPM::Cookery::Recipe

    source      'nothing', :with => :noop
    name        'ua-server'
    description 'UA Server'
    maintainer  'Jon Topper <jon@scalefactory.com>'
    vendor      'fpm'
    revision    0

    config_files '/var/lib/ua2/uadata.edf', 
                 '/etc/ua2/ua.edf'

    if ENV.has_key?('PKG_VERSION')
        version ENV['PKG_VERSION']
    else
        raise 'No PKG_VERSION passed in the environment'
    end

    def build
        Dir.chdir "#{workdir}" do
            safesystem("dos2unix configure.sh")
	    safesystem("./configure.sh")
            safesystem("make clean")
            safesystem("make all")
        end
    end

    def install

        mkdir_p etc('ua2')
        mkdir_p var('log/ua2')
        mkdir_p var('lib/ua2')
        mkdir_p root('usr/libexec')
	mkdir_p sbin()

	cp "#{workdir}/bin/ICE",         sbin("ICE")
	cp "#{workdir}/bin/MessageSpec", sbin("MessageSpec")
	cp "#{workdir}/bin/ua",          sbin("ua")

	cp "#{workdir}/bin/libua",       root("usr/libexec/libua")

	cp "#{workdir}/bin/ua.edf",	 etc("ua2/ua.edf")
	cp "#{workdir}/bin/uadata.edf",  var("lib/ua2/uadata.edf")

    end

end
