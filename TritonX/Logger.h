#pragma once

#include <iostream>
#include <type_traits>
#include <ctime>
#include <iomanip>
#include <string_view>

namespace Log {

   enum class Level {
      Trace = 0,
      Debug,
      Info,
      Warn,
      Error
   };

   class LogManager {
    public:
      static LogManager& getInstance() {
         static LogManager instance;
         return instance;
      }
      void setMinLevel(Level newLevel) {
         level = newLevel;
      }

      [[nodiscard]] Level getLevel() const {
         return level;
      }

    private:
      LogManager() = default;
      Level level;
   };

   class Logger {
    public:
      Logger(Level level) : level(level) {
      }

      [[nodiscard]] const std::wstring_view header() const {

         if (level == Level::Trace) {
            return L"Trace ";
         }
         if (level == Level::Debug) {
            return L"Debug ";
         }
         if (level == Level::Info) {
            return L"Info  ";
         }
         if (level == Level::Warn) {
            return L"Warn  ";
         }
         if (level == Level::Error) {
            return L"Error ";
         }
         return L"Unknown";
      }

      template <typename T>
      Logger& operator<<(const T& value) {
         if (LogManager::getInstance().getLevel() > level) {
            return *this;
         }
         if (isNextBegin) {
            std::wstringstream wss{};
            wss << header().data() << value << std::endl;
            OutputDebugString(wss.str().c_str());
         } else {
            OutputDebugString(value);
         }
         isNextBegin = false;
         return *this;
      }

      Logger& operator<<(decltype(std::endl<char, std::char_traits<char>>)& manip) {
         if (LogManager::getInstance().getLevel() > level) {
            return *this;
         }
         isNextBegin = true;
         std::cout << manip;
         return *this;
      }

    private:
      Level level;
      bool isNextBegin{true};
   };

   inline Logger trace = Logger{Level::Trace};
   inline Logger debug = Logger{Level::Debug};
   inline Logger info = Logger{Level::Info};
   inline Logger warn = Logger{Level::Warn};
   inline Logger error = Logger{Level::Error};

      void LastError() {
      DWORD errCode = GetLastError();
      LPWSTR errMsg = nullptr;
      FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                     nullptr,
                     errCode,
                     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                     (LPWSTR)&errMsg,
                     0,
                     nullptr);

      Log::error << errMsg << std::endl;
      LocalFree(errMsg);
   }
}