# Gengetopt: http://www.gnu.org/software/gengetopt/

add_definitions(-DPACKAGE=\"gengetopt\")
add_definitions(-DVERSION=\"2.22.4\")
add_definitions(-DHAVE_SSTREAM)
add_definitions(-DHAVE_NAMESPACES)

# Remove compiler warnings
string(REPLACE "/W3" "/w" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REPLACE "/W3" "/w" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
string(REPLACE "-Wall" "-w" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REPLACE "-Wall" "-w" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

# MSVC specific issues
if(MSVC)
  add_definitions(-DHAVE_STRING_H)
  add_definitions(-DYY_NO_UNISTD_H)
  add_definitions(-Disatty=!)
endif(MSVC)

add_executable(gengetopt
  ./cmdline.c
  ./ggos.cpp
  ./scanner.cc
  ./skels/required_option.cc
  ./skels/handle_version.cc
  ./skels/generic_option.cc
  ./skels/clear_arg.cc
  ./skels/update_given.cc
  ./skels/dependant_option.cc
  ./skels/custom_getopt_gen.cc
  ./skels/handle_help.cc
  ./skels/given_field.cc
  ./skels/init_args_info.cc
  ./skels/multiple_opt_list.cc
  ./skels/free_list.cc
  ./skels/exit_failure.cc
  ./skels/clear_given.cc
  ./skels/free_multiple.cc
  ./skels/enum_decl.cc
  ./skels/reset_group.cc
  ./skels/c_source.cc
  ./skels/group_option.cc
  ./skels/file_save.cc
  ./skels/check_modes.cc
  ./skels/copyright.cc
  ./skels/option_arg.cc
  ./skels/group_counter.cc
  ./skels/free_string.cc
  ./skels/header.cc
  ./skels/print_help_string.cc
  ./skels/multiple_fill_array.cc
  ./skels/file_save_multiple.cc
  ./fileutils.cpp
  ./getopt.c
  ./gm.cc
  ./globals.cpp
  ./yywrap.c
  ./getopt1.c
  ./argsdef.c
  ./gm_utils.cpp
  ./acceptedvalues.cpp
  ./gengetopt.cc
  ./yyerror.cc
  ./parser.cc)

include_directories(.)
include_directories(skels)
#include_directories(includes)
