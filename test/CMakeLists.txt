include(CMTest)

cm_find_package(Boost REQUIRED COMPONENTS unit_test_framework)

if(Boost_FOUND AND BUILD_SHARED_LIBS)
    set(Boost_USE_STATIC_LIBS FALSE)
else()
    set(Boost_USE_STATIC_LIBS TRUE)
endif()

cm_test_link_libraries(${CMAKE_WORKSPACE_NAME}_${CURRENT_PROJECT_NAME}

                       ${Boost_LIBRARIES})

macro(define_storage_test name)
    cm_test(NAME ${name}_test SOURCES ${name}.cpp)

    target_include_directories(${name}_test PRIVATE
                               "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/public>"
                               "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/private>"
                               "$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>"

                               ${Boost_INCLUDE_DIRS})

    set_target_properties(${name}_test PROPERTIES CXX_STANDARD 14)

    target_compile_definitions(${name}_test PRIVATE
                               $<$<STREQUAL:$<UPPER_CASE:$<TARGET_PROPERTY:BUILD_SHARED_LIBS>>,
                               TRUE>:BOOST_TEST_DYN_LINK>)
endmacro()

set(TESTS_NAMES
    "storage"
    )

foreach(TEST_NAME ${TESTS_NAMES})
    define_storage_test(${TEST_NAME})
endforeach()