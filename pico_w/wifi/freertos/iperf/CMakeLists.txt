add_executable(picow_freertos_iperf_server_mqtt
		 tcp_debug.c
		 tcp_debug.h
		head_tail.c
		head_tail.h	
        picow_freertos_iperf.c
       
		mqtt.c
		mqtt_example.h

		
		
        )
target_compile_definitions(picow_freertos_iperf_server_mqtt PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        )
target_include_directories(picow_freertos_iperf_server_mqtt PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/../.. # for our common lwipopts
        )
target_link_libraries(picow_freertos_iperf_server_mqtt
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        pico_lwip_iperf
        FreeRTOS-Kernel-Heap4 # FreeRTOS kernel and dynamic heap
		hardware_rtc
        )
pico_add_extra_outputs(picow_freertos_iperf_server_mqtt)

 