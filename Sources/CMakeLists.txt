# configure_file(${CMAKE_CURRENT_SOURCE_DIR}/gel/gel.h.in ${CMAKE_CURRENT_BINARY_DIR}/gel/gel.h)
# list(APPEND GEL_SOURCES
#   ${CMAKE_CURRENT_BINARY_DIR}/gel/gel.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/annotation.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/argument.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/argument.h
#   ${CMAKE_CURRENT_SOURCE_DIR}/gel/bitfield.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/bitvector.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/bitvector.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/buffer.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/buffer.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/bytecode.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/bytecode.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/call_stack.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/call_stack.h
#   ${CMAKE_CURRENT_SOURCE_DIR}/gel/common.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/environment.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/environment.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/event_emitter.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/event_emitter.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/event_loop.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/event_loop.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/exception.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/exception.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/exception_table.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/exception_table.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/flags.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/flags.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/fmt.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/fs.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/fs.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/gv.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/gv.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/hashcode.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/instruction.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/instruction.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/instruction_vm.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/iterator.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/iterator.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/local.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/local.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/local_scope.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/local_scope.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/memory_region.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/memory_region_osx.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/module_loader.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/module_loader.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/native_bindings.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/native_bindings.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/natives.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/natives.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/natives_debug.cc
#   ${CMAKE_CURRENT_SOURCE_DIR}/gel/os_thread.cc
#   ${CMAKE_CURRENT_SOURCE_DIR}/gel/os_thread.h
#   ${CMAKE_CURRENT_SOURCE_DIR}/gel/os_thread_linux.h
#   ${CMAKE_CURRENT_SOURCE_DIR}/gel/os_thread_osx.cc
#   ${CMAKE_CURRENT_SOURCE_DIR}/gel/os_thread_osx.h
#   ${CMAKE_CURRENT_SOURCE_DIR}/gel/os_thread_windows.h
#   ${CMAKE_CURRENT_SOURCE_DIR}/gel/platform.cc
#   ${CMAKE_CURRENT_SOURCE_DIR}/gel/platform.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/plugin.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/profiling.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/ref.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/ref.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/repl.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/repl.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/runtime.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/runtime.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/rx.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/rx.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/rx_object.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/rx_object.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/script.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/script.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/seq.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/set.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/set.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/shared_lib.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/shared_lib.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/struct.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/subject.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/subject.h
#   ${CMAKE_CURRENT_SOURCE_DIR}/gel/thread_local.h
#   ${CMAKE_CURRENT_SOURCE_DIR}/gel/thread_local_osx.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/timer.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/timer.h
#   ${CMAKE_CURRENT_SOURCE_DIR}/gel/to_string_helper.cc
#   ${CMAKE_CURRENT_SOURCE_DIR}/gel/to_string_helper.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/tracing.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/trie.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/util.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/uv.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/uv.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/variable.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/variable.h
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/vec.cc
#   # ${CMAKE_CURRENT_SOURCE_DIR}/gel/vec.h
#   gel/entry.h
#   gel/native_entry.h
#   gel/native_entry.cc
#   gel/runtime_entry.h
#   gel/runtime_entry.cc
#   ${CMAKE_CURRENT_SOURCE_DIR}/gel/version.cc)
#
# set(GEL_LIBS
#   fmt::fmt
#   units::units
#   elfio::elfio
#   FunctionalPlus::fplus
#   $<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv>
#   graphviz::gvc graphviz::cgraph
#   OpenSSL::SSL OpenSSL::Crypto
#   Backward::Backward
#   glm::glm
#   BZip2::BZip2
#   RPP::rpp 
#   rapidjson)
#
# include(Terminal)
# list(APPEND GEL_LIBS gel::term)
# if(ENABLE_TRACING)
#   include(Tracing)
#   list(APPEND GEL_LIBS gel::tracing)
# endif()
#
# add_library(gelrt SHARED
#   ${GEL_SOURCES})
# target_compile_options(gelrt
#   PUBLIC ${GEL_CXX_FLAGS})
# target_include_directories(gelrt
#   PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
#          ${CMAKE_CURRENT_BINARY_DIR})
# target_link_libraries(gelrt
#   PUBLIC ${GEL_LIBS})
# if(CMAKE_BUILD_TYPE MATCHES "^[rR]elease")
#   target_enable_iwyu(gelrt)
# endif()
# add_subdirectory(gel/)



