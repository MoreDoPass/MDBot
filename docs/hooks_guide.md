# Полное руководство по использованию хуков в C++20 для WoW 3.3.5a

## Содержание

- [Введение](#введение)
- [Основные концепции](#основные-концепции)
  - [Что такое хуки](#что-такое-хуки)
  - [Типы хуков](#типы-хуков)
  - [Управление памятью](#управление-памятью)
- [Архитектура системы хуков](#архитектура-системы-хуков)
  - [Базовые компоненты](#базовые-компоненты)
  - [Современный подход](#современный-подход)
  - [Паттерны проектирования](#паттерны-проектирования)
- [Реализация хуков](#реализация-хуков)
  - [Inline хуки](#inline-хуки)
  - [IAT хуки](#iat-хуки)
  - [VMT хуки](#vmt-хуки)
- [Продвинутые техники](#продвинутые-техники)
  - [Трамплины](#трамплины)
  - [Относительные смещения](#относительные-смещения)
  - [Работа с исключениями](#работа-с-исключениями)
- [Безопасность и отладка](#безопасность-и-отладка)
  - [Защита от обнаружения](#защита-от-обнаружения)
  - [Логирование](#логирование)
  - [Обработка ошибок](#обработка-ошибок)
- [Практические примеры](#практические-примеры)
  - [Хук функции отрисовки](#хук-функции-отрисовки)
  - [Хук обработки пакетов](#хук-обработки-пакетов)
  - [Хук игровой логики](#хук-игровой-логики)
- [Оптимизация](#оптимизация)
  - [Производительность](#производительность)
  - [Управление ресурсами](#управление-ресурсами)
- [Заключение](#заключение)

## Введение

Хуки (hooks) представляют собой мощный механизм для модификации поведения программы во время выполнения. В контексте WoW 3.3.5a они позволяют перехватывать и изменять различные аспекты игры, от обработки сетевых пакетов до рендеринга интерфейса.

### Цели документации

1. Предоставить полное понимание механизма работы хуков
2. Научить безопасному использованию хуков в современном C++
3. Показать практические примеры для WoW 3.3.5a
4. Объяснить особенности работы с памятью на x86 архитектуре

## Основные концепции

### Что такое хуки

Хук - это техника перехвата вызовов функций, позволяющая:

- Модифицировать параметры вызова
- Изменять возвращаемые значения
- Выполнять дополнительный код до или после вызова
- Полностью заменять функциональность

```cpp
// Пример простейшего хука
using MessageBoxA_t = int (WINAPI*)(HWND, LPCSTR, LPCSTR, UINT);
MessageBoxA_t original_MessageBoxA = nullptr;

int WINAPI hooked_MessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType) {
    // Выполняем код до вызова оригинальной функции
    Logger::Debug("MessageBoxA вызван с текстом: {}", lpText);
    
    // Вызываем оригинальную функцию
    int result = original_MessageBoxA(hWnd, lpText, lpCaption, uType);
    
    // Выполняем код после вызова
    Logger::Debug("MessageBoxA вернул: {}", result);
    
    return result;
}
```

### Управление памятью

#### Защита памяти

В Windows все страницы памяти имеют определенные атрибуты защиты:

- PAGE_NOACCESS
- PAGE_READONLY
- PAGE_READWRITE
- PAGE_EXECUTE
- PAGE_EXECUTE_READ
- PAGE_EXECUTE_READWRITE

```cpp
// Класс для автоматического управления защитой памяти (RAII)
class MemoryProtector {
private:
    void* address;
    size_t size;
    DWORD oldProtect;
    bool active;

public:
    MemoryProtector(void* addr, size_t sz) 
        : address(addr), size(sz), active(false) {
        if (VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            active = true;
        } else {
            throw std::runtime_error(
                std::format("Не удалось изменить защиту памяти: {}", GetLastError())
            );
        }
    }

    ~MemoryProtector() {
        if (active) {
            DWORD dummy;
            VirtualProtect(address, size, oldProtect, &dummy);
        }
    }
};
```

### Типы хуков

#### 1. Inline Hook

Inline hook заменяет первые байты функции на инструкцию jump:

```cpp
// Структура для x86 jump
#pragma pack(push, 1)
struct JumpInstruction {
    uint8_t opcode;    // 0xE9 для near jump
    int32_t offset;    // Относительное смещение
};
#pragma pack(pop)

class InlineHook {
private:
    void* target_function;
    void* hook_function;
    std::array<uint8_t, 5> original_bytes;
    bool enabled;

    static constexpr size_t JUMP_SIZE = sizeof(JumpInstruction);

public:
    InlineHook(void* target, void* hook) 
        : target_function(target)
        , hook_function(hook)
        , enabled(false) {
        // Сохраняем оригинальные байты
        std::memcpy(original_bytes.data(), target_function, JUMP_SIZE);
    }

    bool Enable() {
        if (enabled) return true;

        MemoryProtector protector(target_function, JUMP_SIZE);

        // Вычисляем относительное смещение
        int32_t relativeOffset = 
            reinterpret_cast<int32_t>(hook_function) - 
            (reinterpret_cast<int32_t>(target_function) + JUMP_SIZE);

        // Создаем инструкцию jump
        JumpInstruction jump{0xE9, relativeOffset};

        // Записываем jump в целевую функцию
        std::memcpy(target_function, &jump, JUMP_SIZE);
        enabled = true;

        return true;
    }

    bool Disable() {
        if (!enabled) return true;

        MemoryProtector protector(target_function, JUMP_SIZE);
        
        // Восстанавливаем оригинальные байты
        std::memcpy(target_function, original_bytes.data(), JUMP_SIZE);
        enabled = false;

        return true;
    }
};
```

#### 2. IAT Hook

IAT (Import Address Table) hook модифицирует таблицу импорта:

```cpp
class IATHook {
private:
    HMODULE module;
    const char* dll_name;
    const char* function_name;
    void* hook_function;
    void* original_function;

public:
    IATHook(const char* mod, const char* dll, const char* func, void* hook)
        : module(GetModuleHandleA(mod))
        , dll_name(dll)
        , function_name(func)
        , hook_function(hook)
        , original_function(nullptr) {
    }

    bool Enable() {
        ULONG size;
        PIMAGE_IMPORT_DESCRIPTOR importDesc = (PIMAGE_IMPORT_DESCRIPTOR)
            ImageDirectoryEntryToData(
                module, 
                TRUE, 
                IMAGE_DIRECTORY_ENTRY_IMPORT, 
                &size
            );

        while (importDesc->Name) {
            char* modName = (char*)((BYTE*)module + importDesc->Name);
            if (_stricmp(modName, dll_name) == 0) {
                // Нашли нужный модуль
                PIMAGE_THUNK_DATA thunk = (PIMAGE_THUNK_DATA)
                    ((BYTE*)module + importDesc->FirstThunk);

                while (thunk->u1.Function) {
                    void** functionAddress = (void**)&thunk->u1.Function;
                    
                    if (*functionAddress == GetProcAddress(
                            GetModuleHandleA(dll_name), 
                            function_name)) {
                        // Нашли нужную функцию
                        MemoryProtector protector(
                            functionAddress, 
                            sizeof(void*)
                        );
                        
                        original_function = *functionAddress;
                        *functionAddress = hook_function;
                        
                        return true;
                    }
                    thunk++;
                }
            }
            importDesc++;
        }
        return false;
    }
};
```

#### 3. VMT Hook

VMT (Virtual Method Table) hook для перехвата виртуальных методов:

```cpp
template<typename T>
class VMTHook {
private:
    void** vmt_table;
    void** original_table;
    std::unordered_map<size_t, void*> hooks;
    size_t method_count;

    size_t CountMethods() {
        size_t count = 0;
        while (vmt_table[count] != nullptr &&
               IsValidCodePtr(vmt_table[count])) {
            count++;
        }
        return count;
    }

    static bool IsValidCodePtr(void* ptr) {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(ptr, &mbi, sizeof(mbi))) {
            return (mbi.State == MEM_COMMIT &&
                    mbi.Protect & (PAGE_EXECUTE | 
                                 PAGE_EXECUTE_READ |
                                 PAGE_EXECUTE_READWRITE));
        }
        return false;
    }

public:
    VMTHook(T* instance) {
        vmt_table = *(void***)instance;
        method_count = CountMethods();

        // Создаем копию оригинальной таблицы
        original_table = new void*[method_count];
        std::memcpy(original_table, 
                   vmt_table, 
                   method_count * sizeof(void*));
    }

    template<typename Fn>
    Fn GetOriginalMethod(size_t index) {
        if (index >= method_count) return nullptr;
        return reinterpret_cast<Fn>(original_table[index]);
    }

    bool HookMethod(size_t index, void* hook_function) {
        if (index >= method_count) return false;

        MemoryProtector protector(
            &vmt_table[index], 
            sizeof(void*)
        );

        hooks[index] = vmt_table[index];
        vmt_table[index] = hook_function;

        return true;
    }

    ~VMTHook() {
        delete[] original_table;
    }
};
```

## Продвинутые техники

### Трамплины

Трамплин - это промежуточный код, который позволяет выполнить оригинальную функцию после модификации:

```cpp
class Trampoline {
private:
    std::vector<uint8_t> code_buffer;
    void* entry_point;

    static constexpr size_t PAGE_SIZE = 4096;
    static constexpr size_t MIN_SIZE = 32;

public:
    Trampoline(void* original_function, size_t size) {
        // Выделяем память для трамплина
        code_buffer.resize(size + MIN_SIZE);
        entry_point = VirtualAlloc(
            nullptr, 
            PAGE_SIZE, 
            MEM_COMMIT | MEM_RESERVE,
            PAGE_EXECUTE_READWRITE
        );

        if (!entry_point) {
            throw std::runtime_error("Не удалось выделить память для трамплина");
        }

        // Копируем оригинальный код
        std::memcpy(entry_point, original_function, size);

        // Добавляем jump обратно в оригинальную функцию
        uint8_t* jump_back = static_cast<uint8_t*>(entry_point) + size;
        int32_t relative_offset = 
            reinterpret_cast<int32_t>(original_function) + size - 
            (reinterpret_cast<int32_t>(jump_back) + 5);

        jump_back[0] = 0xE9; // JMP
        *reinterpret_cast<int32_t*>(jump_back + 1) = relative_offset;
    }

    void* GetEntryPoint() const {
        return entry_point;
    }

    ~Trampoline() {
        if (entry_point) {
            VirtualFree(entry_point, 0, MEM_RELEASE);
        }
    }
};
```

### Работа с исключениями

Современный подход к обработке ошибок с использованием C++20:

```cpp
class HookException : public std::exception {
private:
    std::string message;

public:
    template<typename... Args>
    HookException(std::format_string<Args...> fmt, Args&&... args)
        : message(std::format(fmt, std::forward<Args>(args)...)) {}

    const char* what() const noexcept override {
        return message.c_str();
    }
};

// Пример использования
try {
    if (!VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        throw HookException(
            "Ошибка изменения защиты памяти: {:#x}, код: {}", 
            reinterpret_cast<uintptr_t>(address), 
            GetLastError()
        );
    }
} catch (const HookException& e) {
    Logger::Error(e.what());
    // Обработка ошибки...
}
```

## Практические примеры

### Хук функции отрисовки

```cpp
// Тип указателя на функцию отрисовки
using EndScene_t = HRESULT(WINAPI*)(IDirect3DDevice9*);
EndScene_t original_EndScene = nullptr;

// Наш хук
HRESULT WINAPI hooked_EndScene(IDirect3DDevice9* device) {
    static bool initialized = false;
    
    if (!initialized) {
        // Инициализация ImGui или другого оверлея
        ImGui_ImplDX9_Init(device);
        initialized = true;
    }

    // Начинаем новый кадр
    ImGui_ImplDX9_NewFrame();
    ImGui::NewFrame();

    // Отрисовка нашего интерфейса
    ImGui::Begin("Bot Control");
    if (ImGui::Button("Toggle Bot")) {
        // Включение/выключение бота
    }
    ImGui::End();

    // Рендеринг
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

    // Вызов оригинальной функции
    return original_EndScene(device);
}
```

### Хук обработки пакетов

```cpp
// Структура для работы с пакетами WoW
struct PacketHandler {
    uint32_t opcode;
    const char* name;
    void (CDataStore::*handler)();
};

class PacketHook {
private:
    std::unordered_map<uint32_t, 
                      std::function<void(CDataStore&)>> hooks;

public:
    template<typename F>
    void RegisterHook(uint32_t opcode, F&& handler) {
        hooks[opcode] = std::forward<F>(handler);
    }

    bool HandlePacket(uint32_t opcode, CDataStore& packet) {
        auto it = hooks.find(opcode);
        if (it != hooks.end()) {
            it->second(packet);
            return true;
        }
        return false;
    }
};

// Пример использования
PacketHook g_packetHook;

// Регистрация хука для SMSG_SPELL_START
g_packetHook.RegisterHook(SMSG_SPELL_START, [](CDataStore& packet) {
    uint64_t caster_guid = packet.ReadGUID();
    uint64_t target_guid = packet.ReadGUID();
    uint32_t spell_id = packet.ReadUInt32();

    Logger::Debug("Каст заклинания {} от {} к {}", 
                 spell_id, caster_guid, target_guid);
});
```

## Оптимизация

### Производительность

```cpp
template<typename T>
class FastHook {
private:
    static constexpr size_t CACHE_LINE = 64;
    alignas(CACHE_LINE) std::atomic<T*> original_function;
    alignas(CACHE_LINE) std::atomic<bool> enabled;

public:
    FastHook(T* target) : original_function(target), enabled(false) {}

    template<typename... Args>
    auto operator()(Args&&... args) {
        if (enabled.load(std::memory_order_acquire)) {
            // Вызов перехватчика
            return hook_function(std::forward<Args>(args)...);
        } else {
            // Прямой вызов оригинальной функции
            return original_function.load(std::memory_order_relaxed)
                (std::forward<Args>(args)...);
        }
    }
};
```

### Управление ресурсами

```cpp
template<typename T>
class HookManager {
private:
    struct HookInfo {
        std::unique_ptr<T> hook;
        std::string description;
        bool is_enabled;
    };

    std::unordered_map<std::string, HookInfo> hooks;
    std::shared_mutex mutex;

public:
    template<typename... Args>
    bool AddHook(const std::string& name, 
                std::string description,
                Args&&... args) {
        std::unique_lock lock(mutex);
        
        auto& info = hooks[name];
        info.hook = std::make_unique<T>(std::forward<Args>(args)...);
        info.description = std::move(description);
        info.is_enabled = false;

        return true;
    }

    bool EnableHook(const std::string& name) {
        std::unique_lock lock(mutex);
        
        auto it = hooks.find(name);
        if (it != hooks.end() && !it->second.is_enabled) {
            if (it->second.hook->Enable()) {
                it->second.is_enabled = true;
                return true;
            }
        }
        return false;
    }

    void EnableAll() {
        std::unique_lock lock(mutex);
        
        for (auto& [name, info] : hooks) {
            if (!info.is_enabled) {
                if (info.hook->Enable()) {
                    info.is_enabled = true;
                }
            }
        }
    }

    std::vector<std::string> GetActiveHooks() const {
        std::shared_lock lock(mutex);
        
        std::vector<std::string> active;
        for (const auto& [name, info] : hooks) {
            if (info.is_enabled) {
                active.push_back(name);
            }
        }
        return active;
    }
};
```

## Заключение

При работе с хуками важно помнить:

1. Безопасность:
   - Всегда проверяйте целостность памяти
   - Используйте RAII для управления ресурсами
   - Обрабатывайте все возможные исключения

2. Производительность:
   - Используйте атомарные операции где необходимо
   - Избегайте лишних копирований данных
   - Учитывайте кэширование процессора

3. Поддержка:
   - Ведите подробное логирование
   - Документируйте все хуки
   - Используйте современные возможности C++20

4. Тестирование:
   - Проверяйте работу хуков в разных условиях
   - Имейте механизм отключения хуков
   - Следите за утечками памяти
