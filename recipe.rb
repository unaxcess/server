class UA2Server < FPM::Cookery::Recipe

    source      'nothing', :with => :noop
    name        'ua2-server'
    description 'UA Server'
    maintainer  'Jon Topper <jon@scalefactory.com>'
    vendor      'fpm'
    version     '2.9-beta2'

    config_files '/var/lib/ua2/uadata.edf', 
                 '/etc/ua2/ua.edf'
    
    post_install "dist/post-install"

    depends 'daemonize'

    if ENV.has_key?('PKG_REVISION')
        revision ENV['PKG_REVISION']
    else
        raise 'No PKG_REVISION passed in the environment'
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

        sbin.install "#{workdir}/bin/ICE"
        bin.install [ "#{workdir}/bin/MessageSpec", "#{workdir}/bin/ua" ]
        root('usr/libexec').install "#{workdir}/bin/libua"
        etc('ua2').install "#{workdir}/bin/ua.edf"
        var('lib/ua2').install "#{workdir}/bin/uadata.edf"

	mkdir_p etc('init.d')
	mkdir_p etc('logrotate.d')
	cp "#{workdir}/dist/init", etc("init.d/ua2-server")
	cp "#{workdir}/dist/logrotate", etc("logrotate.d/ua2-server")

    end

end
