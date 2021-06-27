set(TESTS
  delta-crdts
)

string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER)
if(CMAKE_BUILD_TYPE_LOWER MATCHES "debug")
  set(TESTS
    ${TESTS}
    error
  )
endif()


foreach (T ${TESTS})
  add_executable(${T} ${T}.cc)
  target_link_libraries(${T} ${DELTA_CRDTS_LIBNAME})

  add_test(NAME ${T} COMMAND ./${T})
endforeach()
