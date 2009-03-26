from SCons.Script import *
#from SCons.Builder import Builder

class ToolPCUTestWarning(SCons.Warnings.Warning):
    pass

SCons.Warnings.enableWarningClass(ToolPCUTestWarning)

def multiget(dicts, key, default=None):
    for d in dicts:
        if d.has_key(key):
            return d[key]
    else:
        return default

def build_suite_runner(env, target, hdrs, objs, **kw):
    hdr_txt = ""
    suite_txt = ""
    init = multiget([kw, env], "PCU_INIT", "")

    setup = multiget([kw, env], "PCU_SETUP", "")
    if setup:
        setup = '\n   ' + setup

    teardown = multiget([kw, env], "PCU_TEARDOWN", "")
    if teardown:
        teardown = '\n   ' + teardown

    for h in hdrs:
        name = os.path.splitext(os.path.basename(h.path))[0]
        suite_txt += "pcu_runner_addSuite( %s, %s );\n"%(name, name + init)
        hdr_txt += "#include \"%s\""%str(h.abspath)

    src = """#include <stdlib.h>
#include <mpi.h>
#include <pcu/pcu.h>
%s

int main( int argc, char* argv[] ) {
   pcu_listener_t* lsnr;

   MPI_Init( &argc, &argv );
   pcu_runner_init( argc, argv );%s

   %s
   lsnr = pcu_textoutput_create();
   pcu_runner_run( lsnr );
   pcu_textoutput_destroy( lsnr );
%s
   pcu_runner_finalise();
   MPI_Finalize();
   return EXIT_SUCCESS;
}
"""%(hdr_txt, setup, suite_txt, teardown)

    dir_path = os.path.dirname(target.abspath)
    if not os.path.exists(dir_path):
        os.makedirs(dir_path)
    f = open(target.abspath, "w")
    f.write(src)
    f.close()

    return File(target.abspath)

def generate(env, **kw):
    env.SetDefault(PCUTEST_TARGET="check")
#     builder = Builder(action=["python", 

    def PCUSuite(env, target, source, **kw):
        """Create an object/header pair out of a
        *Suite.c/*Suite.h pair. The target should just
        be the name of the suite. So, if target were
        "Happy", the sources would be "HappySuite.c" and
        "HappySuite.h\""""
        obj = env.Object(target[0], source[0])
        return [obj + [File(os.path.splitext(source[0].abspath)[0] + ".h")]]

    def PCUTest(env, target, source, **kw):
        # Generate a list of headers, one for each suite source.
        hdrs = []
        objs = []
        for s in source:
            objs.append(s)
            hdrs.append(File(os.path.splitext(s.srcnode().abspath)[0] + '.h'))

        # Generate the program source.
        prog_src = build_suite_runner(env, File(str(target[0]) + ".c"), hdrs, objs, **kw)

        # Build everything.
        objs = env.StaticObject(os.path.splitext(prog_src.abspath)[0], prog_src) + objs
        libs = multiget([kw, env], 'LIBS', []) + ["pcu"]
        test = env.Program(target[0], objs, LIBS=libs)
        runner = env.Action(test[0].abspath)
        env.Alias(env["PCUTEST_TARGET"], test, runner)
        env.AlwaysBuild(env["PCUTEST_TARGET"])
        return test

    env.Append(BUILDERS={"PCUSuite": PCUSuite, "PCUTest": PCUTest})

def exists(env):
    # Should probably have this search for the pcu
    # libraries/source or something.
    return True
