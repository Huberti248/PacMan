# Generated by Boost 1.76.0

# address-model=32

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  _BOOST_SKIPPED("libboost_graph-vc142-mt-gd-x32-1_76.lib" "32 bit, need 64")
  return()
endif()

# layout=versioned

# toolset=vc142

if(Boost_COMPILER)
  if(NOT Boost_COMPILER STREQUAL "vc142" AND NOT Boost_COMPILER STREQUAL "-vc142")
    _BOOST_SKIPPED("libboost_graph-vc142-mt-gd-x32-1_76.lib" "vc142, Boost_COMPILER=${Boost_COMPILER}")
    return()
  endif()
else()
  if(BOOST_DETECTED_TOOLSET AND NOT BOOST_DETECTED_TOOLSET STREQUAL "vc142")
    _BOOST_SKIPPED("libboost_graph-vc142-mt-gd-x32-1_76.lib" "vc142, detected ${BOOST_DETECTED_TOOLSET}, set Boost_COMPILER to override")
    return()
  endif()
endif()

# link=static

if(DEFINED Boost_USE_STATIC_LIBS)
  if(NOT Boost_USE_STATIC_LIBS)
    _BOOST_SKIPPED("libboost_graph-vc142-mt-gd-x32-1_76.lib" "static, Boost_USE_STATIC_LIBS=${Boost_USE_STATIC_LIBS}")
    return()
  endif()
else()
  if(NOT WIN32 AND NOT _BOOST_SINGLE_VARIANT)
    _BOOST_SKIPPED("libboost_graph-vc142-mt-gd-x32-1_76.lib" "static, default is shared, set Boost_USE_STATIC_LIBS=ON to override")
    return()
  endif()
endif()

# runtime-link=shared

if(Boost_USE_STATIC_RUNTIME)
  _BOOST_SKIPPED("libboost_graph-vc142-mt-gd-x32-1_76.lib" "shared runtime, Boost_USE_STATIC_RUNTIME=${Boost_USE_STATIC_RUNTIME}")
  return()
endif()

# runtime-debugging=on

if(NOT "${Boost_USE_DEBUG_RUNTIME}" STREQUAL "" AND NOT Boost_USE_DEBUG_RUNTIME)
  _BOOST_SKIPPED("libboost_graph-vc142-mt-gd-x32-1_76.lib" "debug runtime, Boost_USE_DEBUG_RUNTIME=${Boost_USE_DEBUG_RUNTIME}")
  return()
endif()

# threading=multi

if(DEFINED Boost_USE_MULTITHREADED AND NOT Boost_USE_MULTITHREADED)
  _BOOST_SKIPPED("libboost_graph-vc142-mt-gd-x32-1_76.lib" "multithreaded, Boost_USE_MULTITHREADED=${Boost_USE_MULTITHREADED}")
  return()
endif()

# variant=debug

if(NOT "${Boost_USE_DEBUG_LIBS}" STREQUAL "" AND NOT Boost_USE_DEBUG_LIBS)
  _BOOST_SKIPPED("libboost_graph-vc142-mt-gd-x32-1_76.lib" "debug, Boost_USE_DEBUG_LIBS=${Boost_USE_DEBUG_LIBS}")
  return()
endif()

if(Boost_VERBOSE OR Boost_DEBUG)
  message(STATUS "  [x] libboost_graph-vc142-mt-gd-x32-1_76.lib")
endif()

# Create imported target Boost::graph

if(NOT TARGET Boost::graph)
  add_library(Boost::graph STATIC IMPORTED)

  set_target_properties(Boost::graph PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${_BOOST_INCLUDEDIR}"
    INTERFACE_COMPILE_DEFINITIONS "BOOST_ALL_NO_LIB"
  )
endif()

# Target file name: libboost_graph-vc142-mt-gd-x32-1_76.lib

get_target_property(__boost_imploc Boost::graph IMPORTED_LOCATION_DEBUG)
if(__boost_imploc)
  message(SEND_ERROR "Target Boost::graph already has an imported location '${__boost_imploc}', which is being overwritten with '${_BOOST_LIBDIR}/libboost_graph-vc142-mt-gd-x32-1_76.lib'")
endif()
unset(__boost_imploc)

set_property(TARGET Boost::graph APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)

set_target_properties(Boost::graph PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG CXX
  IMPORTED_LOCATION_DEBUG "${_BOOST_LIBDIR}/libboost_graph-vc142-mt-gd-x32-1_76.lib"
  )

list(APPEND _BOOST_GRAPH_DEPS regex headers)
