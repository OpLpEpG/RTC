if(CONFIG_STM32F1_RTC)

message("UMDOM: modul RTC Запуск сборки.")

zephyr_include_directories( . )
zephyr_library()
zephyr_library_compile_definitions(NO_POSIX_CHEATS)

include(../stm32f1xx_ll.cmake)

add_stm32_ll(pwr rcc rtc)

zephyr_library_sources(rtc_stm32.c)
    
endif(CONFIG_STM32F1_RTC)

