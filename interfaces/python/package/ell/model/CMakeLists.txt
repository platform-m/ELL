if(${PYTHON_ENABLED})

    set(module_name "pythonpackage_ell_model")

    file(GLOB PYFILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.py)

    add_custom_target(${module_name} ALL DEPENDS SOURCES ${PYFILES})
    
    copy_newer_files(${module_name} PYFILES)

    set_property(TARGET ${module_name} PROPERTY FOLDER "interfaces/python/package/ell/model")

endif()  # PYTHON_ENABLED