function (compile_flatbuffer out_var)
  get_property(_FLATC_INSTALL_DIR GLOBAL PROPERTY "AIRTREE_DEP_INSTALL_DIR_flatbuffers")
  set(_FLATC "${_FLATC_INSTALL_DIR}/bin/flatc")

  set(result "")
  foreach(fbs IN LISTS ARGN)
    get_filename_component(name ${fbs} NAME_WE)
    set(out "${CMAKE_CURRENT_BINARY_DIR}/generated/${name}_generated.h")

    add_custom_command(
      OUTPUT ${out}
      COMMAND ${_FLATC} -c -o ${CMAKE_CURRENT_BINARY_DIR}/generated ${fbs}
      DEPENDS ${fbs} ${_FLATC}
      COMMENT "Compiling ${fbs} to ${out}"
      VERBATIM
    )

    message(STATUS "Flatbuffer schema ${fbs} will generate ${out}")
    list(APPEND result ${out})
  endforeach()

  set(${out_var} ${result} PARENT_SCOPE)
endfunction()