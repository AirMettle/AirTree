# Helper to create a target dependency graph

function(add_airtree_module TARGET_NAME LIBRARY_TYPE)
    add_library(${TARGET_NAME} ${LIBRARY_TYPE} ${ARGN})
    
    if(TARGET am_airtree_dependencies)
        add_dependencies(${TARGET_NAME} am_airtree_dependencies)
    endif()
endfunction()
