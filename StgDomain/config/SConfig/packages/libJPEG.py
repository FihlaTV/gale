import os
import SConfig

class libJPEG(SConfig.Package):
    def __init__(self, scons_env, scons_opts, required=False):
        SConfig.Package.__init__(self, scons_env, scons_opts, required)
        self.headers = [['jpeglib.h']]
        self.libraries = [['jpeg']]
        self.have_define = 'HAVE_JPEG'
