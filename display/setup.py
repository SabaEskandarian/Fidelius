from distutils.core import setup, Extension

module1 = Extension('decode',
                    sources = ['decode.c'])

setup (name = 'decode',
       version = '1.0',
       description = 'This is a decode package',
       ext_modules = [module1])