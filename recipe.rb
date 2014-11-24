class UA2Server < FPM::Cookery::Recipe

    source      'nothing', :with => :noop
    name        'ua2-server'
    description 'UA Server'
    maintainer  'Jon Topper <jon@scalefactory.com>'
    vendor      'fpm'
    revision    0

    config_files '/var/lib/ua2/uadata.edf', 
                 '/etc/ua2/ua.edf'
    
    post_install "dist/post-install"

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

        sbin.install "#{workdir}/bin/ICE"
        bin.install [ "#{workdir}/bin/MessageSpec", "#{workdir}/bin/ua" ]
        root('usr/libexec').install "#{workdir}/bin/libua"
        etc('ua2').install "#{workdir}/bin/ua.edf"
        var('lib/ua2').install "#{workdir}/bin/uadata.edf"
        etc('init.d').install "#{workdir}/dist/init"
        etc('logrotate.d').install "#{workdir}/dist/logrotate"

    end

end
