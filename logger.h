#ifndef LOGGER_H
#define LOGGER_H

#include <stdbool.h>
#include <stdarg.h>

// ログレベル定義
typedef enum {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL
} LogLevel;

/**
 * @brief ロガーを初期化します。ログ機能を使用する前に一度だけ呼び出す必要があります。
 *
 * @param log_dir ログファイルを保存するディレクトリ。
 * @param log_filename ログファイルのベース名 (例: "driver.log")。
 * @param max_size_bytes ローテーションを行うファイルサイズの上限（バイト単位）。
 * @param max_files 保持するログファイルの最大数。
 * @return bool 初期化に成功した場合はtrue、失敗した場合はfalse。
 */
bool logger_init(const char* log_dir, const char* log_filename, long max_size_bytes, int max_files);

/**
 * @brief ロガーをシャットダウンし、開いているファイルを閉じます。
 */
void logger_close(void);

/**
 * @brief ログにメッセージを書き込みます。
 *
 * @param level ログレベル。
 * @param source メッセージのソース（モジュール名など）。
 * @param format printf形式のフォーマット文字列。
 * @param ... フォーマット文字列に対応する引数。
 */
void logger_write(LogLevel level, const char* source, const char* format, ...);

// ログ出力を簡単にするためのヘルパーマクロ
#define LOG_DEBUG(source, ...) logger_write(LOG_LEVEL_DEBUG, source, __VA_ARGS__)
#define LOG_INFO(source, ...)  logger_write(LOG_LEVEL_INFO,  source, __VA_ARGS__)
#define LOG_WARN(source, ...)  logger_write(LOG_LEVEL_WARN,  source, __VA_ARGS__)
#define LOG_ERROR(source, ...) logger_write(LOG_LEVEL_ERROR, source, __VA_ARGS__)
#define LOG_FATAL(source, ...) logger_write(LOG_LEVEL_FATAL, source, __VA_ARGS__)

#endif // LOGGER_H
