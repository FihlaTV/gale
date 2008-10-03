#!/usr/bin/python

from mercurial import hg, ui, util
import urllib2
import ConfigParser
import os, errno

deps = [ \
	['https://csd.vpac.org/hg/hgforest', '.hg/forest' ], \
	['https://csd.vpac.org/hg/SConfigure', 'config' ], \
	['https://www.mcc.monash.edu.au/hg/gLucifer', 'gLucifer' ], \
	['https://csd.vpac.org/hg/PICellerator', 'PICellerator' ], \
	['https://csd.vpac.org/hg/StgDomain', 'StgDomain' ], \
	['https://csd.vpac.org/hg/StGermain', 'StGermain' ], \
	['https://csd.vpac.org/hg/StgFEM', 'StgFEM' ], \
	['https://www.mcc.monash.edu.au/hg/Underworld', 'Underworld' ] ]

# Make sure the '.hg' directory exists
try:
	os.mkdir('.hg')
except OSError, e:
	if( e.errno != errno.EEXIST ):
		raise OSError( e )  # if the error is the directory exists already, keep going


# Download the dependancies...
u = ui.ui()
for dep in deps:
	try:
		print dep[1], '...'
		hg.clone( u, dep[0], dep[1] );
	except util.Abort, e:
		c = ConfigParser.ConfigParser()
		try:
			c.readfp( open( dep[1] + '/.hg/hgrc', 'r' ) )
			if( c.get( 'paths', 'default' ) != dep[0] ):
				print 'Creation failed - ', e, ' but points to another repository'
			else:
				print dep[1], 'already present'
		except:
			print 'Creation failed - ', e, ' and does not seem to be a valid repository'
	except urllib2.URLError, e:
		print 'Download failed - ', e

# Tell this root repository that its has forest
c=ConfigParser.ConfigParser()
c.read('.hg/hgrc')
try:
	c.add_section('extensions')
except ConfigParser.DuplicateSectionError:
	pass    # If the error is that it exists already, keep going
c.set( 'extensions', 'hgext.forest', os.getcwd() + '/.hg/forest/forest.py' )
c.write( open( '.hg/hgrc', 'w' ) )
