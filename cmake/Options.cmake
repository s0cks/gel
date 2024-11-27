option(WERROR "Turn warnings into errors." OFF)
if(WERROR)
  add_compile_options(-Werror)
endif()

option(ENABLE_RX "Compile w/ rx" ON)

option(ENABLE_GRAPHVIZ "Compile w/ Graphviz" OFF)
if(ENABLE_GRAPHVIZ)
  find_package(Graphviz REQUIRED)
  add_compile_definitions(SCM_ENABLE_GV)
  list(APPEND SCM_LIBRARIES graphviz::gvc graphviz::cgraph)
endif()

option(DISABLE_HEAP_ALLOCATOR "Disable the heap allocator." OFF)
if(DISABLE_HEAP_ALLOCATOR)
  add_compile_definitions(SCM_DISABLE_HEAP)
endif()

option(ENABLE_LAMBDA_CACHE "Enable the lambda cache" ON)
if(ENABLE_LAMBDA_CACHE)
  add_compile_definitions(SCM_ENABLE_LAMBDA_CACHE)
endif()

option(ENABLE_TRACING "Enable Tracy tracing." OFF)
if(ENABLE_TRACING)
  add_compile_definitions(SCM_TRACING)
endif()

option(ENABLE_CLANG_TIDY "Enable clang-tidy" ON)
if(ENABLE_CLANG_TIDY)
  include(ClangTidy)
  # enable_clang_tidy()
endif()

option(PRT_BUILD_DOCS "Build documentation" ON)
if(PRT_BUILD_DOCS)
  include(Doxygen)
endif()

option(ENABLE_COVERAGE "Enable code coverage reports using gcov/lcov" OFF)
if(ENABLE_COVERAGE)
  include(Coverage)
  # TODO: cleanup this logic, get it into Coverage.cmake
  if(${CMAKE_CXX_COMPILER_ID} MATCHES "([aA]pple)?[cC]lang|GNU")
    list(
      APPEND
      SCM_CXX_FLAGS
      -O0
      -g
      -fprofile-instr-generate
      -fcoverage-mapping
      --coverage)
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # TODO: implement
  else()
    message(FATAL_ERROR "unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
  endif()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "([aA]pple)?[cC]lang|GNU")
  option(ENABLE_ADDRESS_SCANITIZER "Enable Clang AddressSanitizer" OFF)
  if(ENABLE_ADDRESS_SCANITIZER AND CMAKE_BUILD_TYPE MATCHES "^[Dd]ebug")
    list(APPEND SCM_CXX_FLAGS -01 -fno-omit-frame-pointer -fsanitize=address)
  endif()

  option(ENABLE_UNDEFINED_SANITIZER "Enable Clang UndefinedBehaviorSanitizer"
         OFF)
  if(ENABLE_UNDEFINED_SANITIZER AND CMAKE_BUILD_TYPE MATCHES "^[Dd]ebug")
    list(APPEND SCM_CXX_FLAGS -fsanitize=undefined -fsanitize=integer)
  endif()
endif()