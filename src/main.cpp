/**
 * @file main.cpp
 * @brief Точка входа в приложение MDBot
 * @details Инициализирует Qt приложение и создает главное окно
 */
#include <QApplication>
#include "gui/MainWindow.hpp"


/**
 * 
 * @brief Функция main - точка входа в приложение
 * @param argc Количество аргументов командной строки
 * @param argv Массив аргументов командной строки
 * @return int Код возврата программы
 */
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);       // Создание приложения Qt
    
    MainWindow window;                  // Создание главного окна
    window.show();                      // Отображение окна на экране
    
    return app.exec();                  // Запуск цикла обработки событий
}
