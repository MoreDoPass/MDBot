cmake_minimum_required(VERSION 3.16)

# Находим Google Test
find_package(GTest CONFIG REQUIRED)

# Создаем исполняемый файл для тестов
add_executable(mdbot_tests
    hooks/test_hook.cpp
)

# Подключаем заголовочные файлы
target_include_directories(mdbot_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# Линкуем с Google Test и основными библиотеками проекта
target_link_libraries(mdbot_tests PRIVATE
    mdbot_core
    GTest::gtest
    GTest::gtest_main
    Qt6::Core
    Qt6::Widgets
)

# Добавляем тест в систему тестирования CMake
add_test(NAME HookTests COMMAND mdbot_tests)

# Автоматическое копирование DLL
if(WIN32)
    # Копируем все DLL из основной Debug папки в папку с тестами
    add_custom_command(TARGET mdbot_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:mdbot_tests>/platforms"
        
        # Копируем все DLL из основной Debug папки
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_BINARY_DIR}/Debug"
            "$<TARGET_FILE_DIR:mdbot_tests>"
            
        COMMENT "Copying DLLs from main Debug folder to tests..."
    )
endif()