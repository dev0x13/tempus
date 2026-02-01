# Localization System

## Overview

The localization system has been redesigned to be simpler and more intuitive:
- **English text is the key**: Use actual English text as the lookup key
- **Auto-fallback**: If no translation exists, the English text is returned
- **Single translation file**: Only `localization_ru.h` contains Russian translations

## Architecture

### Files
- `LocalizationManager.hpp` - Main interface
- `LocalizationManager.cpp` - Implementation
- `localization_ru.h` - Russian translations (key-value map)
- `Language.hpp` - Language enumeration

### How It Works

1. **English Mode**: Returns the key itself (no lookup needed)
2. **Russian Mode**: Looks up the key in the translations map
3. **Fallback**: If translation not found, returns the English key

## Usage

### Basic String Translation

```cpp
auto& L = localization::L10n();

// Simple text
ImGui::Button(L.get("Save"), ImVec2(100, 0));
ImGui::Text(L.get("Activity:"));

// Format strings
char buffer[256];
snprintf(buffer, sizeof(buffer), L.get("Page %d of %d"), page, total);
ImGui::Text("%s", buffer);
```

### Integer Values

For locale-specific integer values (like FirstDayOfWeek):

```cpp
int firstDay = L.getInt("FirstDayOfWeek", 0); // 0 for English (Sunday), 1 for Russian (Monday)
```

### Language Switching

```cpp
// Set language
L10n().setLanguage(Language::Russian);
L10n().setLanguage(Language::English);

// Get current language
Language current = L10n().getCurrentLanguage();
```

## Adding New Translations

To add a new translatable string:

1. Use it directly in code with English text:
   ```cpp
   ImGui::Button(L.get("My New Button"), ImVec2(100, 0));
   ```

2. Add the Russian translation to `localization_ru.h`:
   ```cpp
   {"My New Button", "Моя новая кнопка"},
   ```

That's it! No need to modify multiple files or add function pointers.

## Translation File Format

The `localization_ru.h` file contains a simple map:

```cpp
inline std::unordered_map<std::string, std::string> getTranslations() {
    return {
        // Common
        {"Save", "Сохранить"},
        {"Cancel", "Отмена"},

        // Main Window
        {"Time Tracker", "Трекер времени"},
        {"Start", "Старт"},

        // Format strings
        {"Page %d of %d", "Страница %d из %d"},

        // Date/Time formats
        {"%m/%d/%Y", "%d.%m.%Y"},
        {"h", "ч"},
        {"min", "мин"},

        // Comma-separated lists
        {"Sunday,Monday,Tuesday,Wednesday,Thursday,Friday,Saturday",
         "Воскресенье,Понедельник,Вторник,Среда,Четверг,Пятница,Суббота"},
    };
}
```

## Benefits

1. **Simplicity**: No complex struct hierarchies or function pointers
2. **Self-documenting**: English text in code shows what will be displayed
3. **Easy to add**: Just use English text in code, add translation later
4. **No duplication**: Don't need to maintain English translation file
5. **Type-safe**: Compile-time string literals prevent typos in most cases
6. **Flexible**: Can handle any string format (plain text, format strings, CSV lists)

## Migration Notes

All existing code has been migrated from the old system:
- `L.Common.Save()` → `L.get("Save")`
- `L.TimeEntries.Activity()` → `L.get("Activity")`
- `L.DateTime.HourAbbrev()` → `L.get("h")`

The migration script is available at `migrate_localization.sh` for reference.
