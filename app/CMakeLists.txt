
set(INC
	.
	../device
	../kernel
	../kernel/svm
	../bvh
	../util
	../render
	../subd
)

set(LIBRARIES
	cycles_device
	cycles_kernel
	cycles_render
	cycles_bvh
	cycles_subd
	cycles_util
	${BOOST_LIBRARIES}
	${OPENGL_LIBRARIES}
	${CYCLES_GLEW_LIBRARY}
	${OPENIMAGEIO_LIBRARIES}
)

#link_libraries(glut GLEW boost_date_time boost_filesystem boost_thread boost_regex boost_system OpenImageIO)

if (HACK_RETAIN_VIEWER) 
  link_libraries(glut GLEW  )
endif()

link_directories(${OPENIMAGEIO_LIBPATH} ${BOOST_LIBPATH})

if(WITH_CYCLES_TEST)
#	list(APPEND LIBRARIES ${GLUT_LIBRARIES})
endif()

if(WITH_CYCLES_OSL)
	list(APPEND LIBRARIES cycles_kernel_osl ${OSL_LIBRARIES})
endif()

if(WITH_CYCLES_PARTIO)
	list(APPEND LIBRARIES ${PARTIO_LIBRARIES})
endif()

include_directories(${INC})

if(WITH_CYCLES_TEST)
	set(SRC
		cycles_test.cpp
		cycles_xml.cpp
		cycles_xml.h
		pugixml.cpp
	)
	add_executable(cycles_test ${SRC})
	target_link_libraries(cycles_test ${LIBRARIES})

	if(UNIX AND NOT APPLE)
		set_target_properties(cycles_test PROPERTIES INSTALL_RPATH $ORIGIN/lib)
	endif()
	unset(SRC)
endif()

if(WITH_CYCLES_NETWORK)
	set(SRC
		cycles_server.cpp
	)
	add_executable(cycles_server ${SRC})
	target_link_libraries(cycles_server ${LIBRARIES})

	if(UNIX AND NOT APPLE)
		set_target_properties(cycles_server PROPERTIES INSTALL_RPATH $ORIGIN/lib)
	endif()
	unset(SRC)
endif()

foreach(f gumbo.xml elephant.xml)
  configure_file(${f} ${CMAKE_BINARY_DIR}/${f} COPYONLY)
endforeach ()

message(STATUS "Only supported command: ./app/cycles_test")
message(STATUS "Requires elephant.xml and gumbo.xml in current directory")


if (HACK_EMSCRIPTEN)

  # for emscripten
  add_executable(trivial_test trivial_test.cpp)
  set_property(
    TARGET trivial_test
    PROPERTY SUFFIX
    .bc)

  set_property(
    TARGET cycles_test
    PROPERTY SUFFIX
    .bc)

endif()

add_executable(rejigger_image rejigger_image.cpp)
