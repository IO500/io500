file(GLOB TESTFILES "${CMAKE_CURRENT_SOURCE_DIR}" "*.c")
foreach(TESTFILE ${TESTFILES})
  if(IS_DIRECTORY ${TESTFILE} )
    #message(STATUS ${TESTFILE})
  else()
    get_filename_component(TESTNAME_C ${TESTFILE} NAME)
    STRING(REGEX REPLACE ".c$" "" TESTNAME ${TESTNAME_C})

	# Build, link and add as test
    add_executable(${TESTNAME} ${TESTFILE})
   	target_link_libraries(${TESTNAME} esdm ${MPI_LIBRARIES} -lrt)
    target_include_directories(${TESTNAME} PRIVATE ${MPI_INCLUDE_PATH} ${CMAKE_BINARY_DIR} ${ESDM_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS})

    add_test(${TESTNAME} ./${TESTNAME})
  endif()
endforeach()
