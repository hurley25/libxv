set(HEADERS xv.h xv_define.h xv_socket.h xv_log.h xv_queue.h xv_th_pool.h xv_atomic.h xv_service.h xv_buffer.h)
set(BASE_SRCS xv.c xv_async.c xv_timer.c xv_signal.c xv_socket.c xv_log.c xv_queue.c xv_th_pool.c xv_service.c xv_buffer.c)

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(ALL_SRCS ${BASE_SRCS} xv_epoll.c)
else()
    set(ALL_SRCS ${BASE_SRCS} xv_select.c)
endif()

add_library(xv ${ALL_SRCS})

target_link_libraries(xv pthread)

install(TARGETS xv DESTINATION lib)

install(FILES ${HEADERS} DESTINATION include/xv)

