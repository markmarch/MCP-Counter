#! /usr/bin/evn python
# encoding: utf-8

import Options, UnitTest
from TaskGen import feature, after
import Task, Utils

srcdir = '.'
blddir = 'build'

def set_options(opt):
    opt.tool_options('compiler_cxx')

    opt.add_option('--debug', help='Build debug variant',
                   action='store_true', dest="build_debug", default=True
                  )

    opt.add_option('--release', help='Build release variant',
                  action='store_false', dest="build_debug"
                 )

def configure(conf):
    conf.check_tool('compiler_cxx')

    #
    # Configure libraries
    #
    conf.env.LIB_PTHREAD = [ 'pthread' ]
    conf.env.LIB_PROFILE = [ 'profiler' ]
    conf.env.LIB_TCMALLOC = [ '' ]

    #
    # Configure a debug environment
    #
    env = conf.env.copy()
    env.set_variant('debug')
    conf.set_env_name('debug', env)
    conf.setenv('debug')
    conf.env.CXXFLAGS = ['-g', '-Wall', '--pedantic',
                         '-fno-omit-frame-pointer', '-std=c++0x']

    #
    # Configure a release environment
    #
    env = conf.env.copy()
    env.set_variant('release')
    conf.set_env_name('release', env)
    conf.setenv('release')
    conf.env.CXXFLAGS = ['-O3', '-Wall', '--pedantic',
                         '-fno-omit-frame-pointer']

def build(bld):
    #****************************************
    # libs
    #

    # header only libs; just documenting
    # http_request.hpp
    # lock.hpp
    # unit_test.hpp
    bld.new_task_gen( features     = 'cxx cstaticlib',
                      source       = """ spinlock.cpp
                                     tree_node.cpp
                                     combining_tree.cpp
                                     thread.cpp
                                     stat_counter.cpp
                                      """
                                      ,
                       include      = '.. .',
                       uselib       = 'PTHREAD',
                       target       = 'combine',
                       name         = 'combine'
                    )
    #****************************************
    # TEST
    #

    bld.new_task_gen( features      = 'cxx cprogram',
                      source        = 'combining_tree_test.cpp', 
                      includes      = '.. .',
                      uselib        = '',
                      uselib_local  = 'combine',
                      target        = 'combining_tree_test',
                      unit_test     = 1
                    )

    bld.new_task_gen( features      = 'cxx cprogram',
                      source        = 'stat_counter_test.cpp', 
                      includes      = '.. .',
                      uselib        = '',
                      uselib_local  = 'stat_counter',
                      target        = 'stat_counter_counter_test',
                      unit_test     = 1
                    )
    #
    # Build debug variant, if --debug was set
    #
    if Options.options.build_debug:
        clone_to = 'debug'
    else: 
        clone_to = 'release'
    for obj in [] + bld.all_task_gen:
        obj.clone(clone_to)
        obj.posted = True # dont build in default environment
