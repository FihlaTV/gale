import os, platform
import config
import config.utils as utils

class Carbon(config.Package):

    def __init__(self, ctx):
        config.Package.__init__(self, ctx)
        self.base_dirs.append("/System/Library/Frameworks/Carbon.framework")

    def _setup_dependencies(self):
        config.Package._setup_dependencies(self)
        self.agl = self.add_dependency(config.packages.AGL, required=True, combine=True)

    def setup_libraries(self):
        self.add_library_set(["Carbon.h"], ["Carbon"])

    source_code = {"c": """#include <stdlib.h>
int main( int argc, char** argv ) {
  return EXIT_SUCCESS;
}
"""}
