project(build-all)
cmake_minimum_required(VERSION 2.6.0)
cmake_policy(VERSION 2.6)

get_filename_component(topdir "${PROJECT_SOURCE_DIR}/.." ABSOLUTE)
set(CMAKE_MODULE_PATH "${topdir}/licq/cmake")

file(GLOB parts RELATIVE ${topdir} "${topdir}/*/CMakeLists.txt")

foreach (part ${parts})
  get_filename_component(dir ${part} PATH)

  if (NOT "${dir}" STREQUAL "build-all")
    string(TOUPPER "${dir}" dir_upper)
    string(REPLACE "-" "_" dir_upper "${dir_upper}")
    option(BUILD_${dir_upper} "Build ${dir}" ON)

    if (BUILD_${dir_upper})
      set(licq_target_prefix "${dir}_")
      add_subdirectory("${topdir}/${dir}" "${dir}")
    endif (BUILD_${dir_upper})

  endif (NOT "${dir}" STREQUAL "build-all")
endforeach (part)
