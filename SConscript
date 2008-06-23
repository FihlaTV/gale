import os, sys, platform, pickle, shutil
import glob as pyglob
import SConfig
from SCons.Script.SConscript import SConsEnvironment

#
# Setup the Package system.
#

def Package(env, pkg_module, required=True, **kw):
    """Create a new package to be configured."""
    if not hasattr(env, 'package_options'):
        env.package_options = Options()
    if not hasattr(env, 'packages'):
        env.packages = {}
        env.package_list = []
    if not pkg_module in env.packages:
        pkg = pkg_module(env, env.package_options, required, **kw)
        for attr, val in kw.iteritems():
            if not hasattr(pkg, attr):
                print 'Package does not have attribute!'
                sys.exit()
            setattr(pkg, attr, val)
        env.packages[pkg_module] = pkg
        env.package_list += [pkg]
    return env.packages[pkg_module]

def CheckPackages(ctx, pkg_list):
    for pkg in pkg_list:
        pkg.configure(ctx)

def configure_packages(env):
    # If we have 'help' given as a target, use that to generate help.
    if 'help' in COMMAND_LINE_TARGETS:
        env.Alias('help', '.')
        print env.package_options.GenerateHelpText(env)
        return

    # Get rid of the temporary directory to make sure we're building
    # from scratch.
    if os.path.exists('.sconsign.dblite'):
        os.remove('.sconsign.dblite')

    # Run the setup method for all nodes.
    is_setup = False
    while not is_setup:
        is_setup = True
        for n in env.package_list:

            # If any node is not ready, we need to loop through them
            # all again.
            if not n.is_setup:
                n.setup()
                is_setup = False

    # Update dependencies and requirements.
    pkgs_rem = list(env.package_list)
    while len(pkgs_rem):
        pkg = pkgs_rem.pop()
        if pkg.required:
            for d, r in pkg.deps:
                if r and not d.required:
                    d.required = True
                    pkgs_rem += [d]

    # Call the packages checker.
    sconf = Configure(pkg.env, custom_tests={'CheckPackages': CheckPackages})
    sconf.CheckPackages(env.package_list)
    sconf.Finish()

    # Print package results.
    print '\n*****************************************'
    print "*               Results                 *"
    print '*****************************************\n'
    for pkg in env.package_list:
        if isinstance(pkg, SConfig.Package):
            if pkg.selected:
                print str(pkg.selected),

    # Print out build message.
    print '\n*****************************************'
    print "* Now run 'scons' to build the project. *"
    print '*****************************************\n'

def save_config(env, filename='config.cfg'):
    # Put the results on this environment.
    for pkg in env.package_list: 
        if pkg.result:
            pkg.enable(env)

    # Update config variables.
    env.AppendUnique(CONFIGVARS=['CC', 'CFLAGS', 'CCFLAGS',
                                 'CPPPATH', 'CPPDEFINES',
                                 'LIBPATH', 'LIBS', 'STATICLIBS',
                                 'RPATH', 'INTLIBS',
                                 'FRAMEWORKS'])
    env.AppendUnique(CONFIGVARS=env.package_options.keys())

    # Dump to file.
    d = {}
    for a in env['CONFIGVARS']:
        if a in env._dict:
            d[a] = env[a]
    f = file(filename, 'w')
    import pickle
    pickle.dump(d, f)
    f.close()

def load_config(env, filename='config.cfg'):
    if not os.path.exists(filename):
        print "\nError: project hasn't been configured!"
        print '*******************************************************'
        print "* Run 'scons config' to configure the project.        *"
        print "* Run 'scons help' to see what options are available. *"
        print '*******************************************************'
        env.Exit()
    f = file(filename, 'r')
    import pickle
    d = pickle.load(f)
    f.close()
    for k, v in d.iteritems():
        env[k] = v
    for script in env.get('CONFIGSCRIPTS', []):
        env.SConscript(script, 'env')
    if 'build_dir' in env._dict:
        env.Default(env['build_dir'])

def write_pkgconfig(env, filename, name, desc='', version=0):
    """Write out a pkgconfig file."""

    # Make sure the directory structure exists.
    filename = File(filename).abspath
    dirs = os.path.dirname(filename)
    if not os.path.exists(dirs):
        os.makedirs(dirs)

    # Write the pkgconfig file.
    f = open(filename, 'w')
    build_path = env.get('build_dir', '')
    if build_path:
        f.write('prefix=%s\n' % build_path)
        f.write('exec_prefix=%s\n' % os.path.join(build_path, 'bin'))
        f.write('libdir=%s\n' % os.path.join(build_path, 'lib'))
        f.write('includedir=%s\n' % os.path.join(build_path, 'include'))
        f.write('\n')
    f.write('Name: %s\n' % name)
    f.write('Description: %s\n' % desc)
    f.write('Version: %s\n' % version)
    f.write('Requires:\n')

    # Unfortunately SCons leaves hashes in paths after calling the
    # subst command, so we'll need to expand these manually.
    old_state = {'LIBPATH': env['LIBPATH'], 'CPPPATH': env['CPPPATH']}
    env['LIBPATH'] = [Dir(p).abspath for p in env['LIBPATH']]
    env['CPPPATH'] = [Dir(p).abspath for p in env['CPPPATH']]
    f.write(env.subst('Libs: ${_LIBDIRFLAGS} ${_LIBFLAGS}') + '\n')
    f.write(env.subst('Cflags: ${_CPPINCFLAGS}') + '\n')
    env.Replace(**old_state)
    f.close()

SConsEnvironment.Package = Package
SConsEnvironment.configure_packages = configure_packages
SConsEnvironment.save_config = save_config
SConsEnvironment.load_config = load_config
SConsEnvironment.write_pkgconfig = write_pkgconfig

#
# Useful utilities.
#

def copy_file(env, dst, src):
    dst = File(dst).abspath
    if os.path.exists(dst):
        return
    dst_dir = os.path.dirname(dst)
    if not os.path.exists(dst_dir):
        os.makedirs(dst_dir)
    shutil.copy(src, dst)

def get_build_path(env, prefix):
    if os.path.isabs(env['build_dir']):
        bld_dir = env['build_dir']
    else:
        bld_dir = '#' + env['build_dir']
    if prefix:
        return os.path.join(bld_dir, prefix)
    else:
        return bld_dir

def get_target_name(env, source, extension=''):
    """Return the destination name for a source file with suffix 'suffix'. This
    is useful for building files into the correct build path. Returns the full
    path to the built source without extension."""
    if extension:
        src = source[:-len(extension)]
    else:
        src = source
    return env.get_build_path(src)

def glob(env, pattern):
    if not os.path.isabs(pattern):
        old = os.getcwd()
        os.chdir(Dir('.').srcnode().abspath)
        res = pyglob.glob(pattern)
        os.chdir(old)
    else:
        res = pyglob.glob(pattern)
    return res

def path_exists(env, path):
    if not os.path.isabs(path):
        old = os.getcwd()
        os.chdir(Dir('.').srcnode().abspath)
        res = os.path.exists(path)
        os.chdir(old)
    else:
        res = os.path.exists(path)
    return res

def strip_dir(env, path, subdir):
    offs = path.find(os.path.sep + subdir + os.path.sep)
    if offs != -1:
        return path[:offs] + path[offs + len(subdir) + 1:]
    offs = path.find(os.path.sep + subdir)
    if offs != -1:
        return path[:-(len(subdir) + 1)]
    return path

def make_list(self, var):
    """Convert anything into a list. Handles things that are already lists,
    tuples and strings."""

    if isinstance(var, str):
        return [var]
    elif isinstance(var, (list, tuple)):
        if not var:
            return []
        return list(var)
    elif var is None:
        return []
    else:
        return [var]

def reverse_list(self, _list):
    """Return a reversed copy of a list."""

    rev = list(_list)
    rev.reverse()
    return rev

SConsEnvironment.strip_dir = strip_dir
SConsEnvironment.copy_file = copy_file
SConsEnvironment.get_build_path = get_build_path
SConsEnvironment.get_target_name = get_target_name
SConsEnvironment.glob = glob
SConsEnvironment.path_exists = path_exists
SConsEnvironment.make_list = make_list
SConsEnvironment.reverse_list = reverse_list

# Customize the created base environment.
Import('env')

def _interleave(int_libs, env):
    txt = ''
    first = True
    for paths, libs in int_libs:
        env['MYLIBPATHS'] = paths
        env['MYLIBS'] = libs
        if first:
            first = False
        else:
            txt += ' '
        txt += env.subst('$_MYLIBPATHS $_MYLIBS')
    return txt

env['_abspath'] = lambda x: File(x).abspath # Needed by Darwin.
env['_interleave'] = _interleave
env['_MYLIBPATHS'] = '$( ${_concat(LIBDIRPREFIX, MYLIBPATHS, LIBDIRSUFFIX, __env__, RDirs, TARGET, SOURCE)} $)'
env['_MYLIBS'] = '${_stripixes(LIBLINKPREFIX, MYLIBS, LIBLINKSUFFIX, LIBPREFIX, LIBSUFFIX, __env__)}'
env['_INTLIBS'] = '${_interleave(INTLIBS, __env__)}'
env['LINKCOM'] += ' $STATICLIBS $_INTLIBS' # Needed for static libs. Thanks SCons. :(
env['SHLINKCOM'] += ' $STATICLIBS $_INTLIBS' # Needed for static libs. Thanks SCons. :(
