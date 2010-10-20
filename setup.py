# Setup Script for pyphp.
# TO USE CALL:
#   python setup.py build
#   python setup.py install
#
from distutils.core import setup, Extension

pyphpModule = Extension('pyphp',
	sources = [
		'pyphp-core.c',
		'pyphp.c'
	],
	include_dirs=[
		'/usr/local/include/php',
		'/usr/local/include/php/main',
		'/usr/local/include/php/Zend',
		'/usr/local/include/php/TSRM'
	],
	libraries=['php5'],
	runtime_library_dirs=[
		'/usr/local/lib',
		'/lib'
	],
	extra_compile_args = ['-std=gnu99']
)

setup (
	name = 'pyphp',
	version = '0.5',
	description = 'Execute PHP scripts from within Python',
	author = 'Caleb P Burns',
	author_email = 'cpburns2009@gmail.com',
	ext_modules = [pyphpModule]
)
