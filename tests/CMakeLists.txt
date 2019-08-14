add_executable(xv_loop_test xv_loop_test.c)
target_link_libraries(xv_loop_test xv)
add_test(NAME xv_loop_test COMMAND xv_loop_test)

add_executable(xv_socket_test xv_socket_test.c)
target_link_libraries(xv_socket_test xv)
add_test(NAME xv_socket_test COMMAND xv_socket_test)

add_executable(xv_epoll_test xv_epoll_test.c)
target_link_libraries(xv_epoll_test xv)
add_test(NAME xv_epoll_test COMMAND xv_epoll_test)

add_executable(xv_log_test xv_log_test.c)
target_link_libraries(xv_log_test xv)
add_test(NAME xv_log_test COMMAND xv_log_test)

add_executable(xv_loop_socket_test xv_loop_socket_test.c)
target_link_libraries(xv_loop_socket_test xv)
add_test(NAME xv_loop_socket_test COMMAND xv_loop_socket_test)

add_executable(xv_loop_async_test xv_loop_async_test.c)
target_link_libraries(xv_loop_async_test xv)
add_test(NAME xv_loop_async_test COMMAND xv_loop_async_test)

add_executable(xv_queue_test xv_queue_test.c)
target_link_libraries(xv_queue_test xv)
add_test(NAME xv_queue_test COMMAND xv_queue_test)

add_executable(xv_worker_thread_test xv_worker_thread_test.c)
target_link_libraries(xv_worker_thread_test xv)
add_test(NAME xv_worker_thread_test COMMAND xv_worker_thread_test)

add_executable(xv_th_pool_test xv_th_pool_test.c)
target_link_libraries(xv_th_pool_test xv)
add_test(NAME xv_th_pool_test COMMAND xv_th_pool_test)

add_executable(xv_atomic_test xv_atomic_test.c)
target_link_libraries(xv_atomic_test xv)
add_test(NAME xv_atomic_test COMMAND xv_atomic_test)

add_executable(xv_buffer_test xv_buffer_test.c)
target_link_libraries(xv_buffer_test xv)
add_test(NAME xv_buffer_test COMMAND xv_buffer_test)

add_executable(xv_service_test xv_service_test.c)
target_link_libraries(xv_service_test xv)
add_test(NAME xv_service_test COMMAND xv_service_test)

add_executable(xv_service_room_test xv_service_room_test.c)
target_link_libraries(xv_service_room_test xv)
