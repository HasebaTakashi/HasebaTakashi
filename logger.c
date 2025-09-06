#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/stat.h>
#include <errno.h>

// --- ロガーの状態を管理する静的変数 ---
static struct {
    char log_dir[1024];
    char log_filename[256];
    char log_filepath[1300];
    long max_size_bytes;
    int max_files;
    FILE* fp;
    pthread_mutex_t mutex;
    bool initialized;
} logger_state;

// --- プロトタイプ宣言 ---
static void rotate_logs(void);
static bool create_log_directory(const char* dir);

// --- 公開関数 ---

bool logger_init(const char* log_dir, const char* log_filename, long max_size_bytes, int max_files) {
    if (logger_state.initialized) {
        return true;
    }

    if (!create_log_directory(log_dir)) {
        fprintf(stderr, "Failed to create log directory: %s\n", log_dir);
        return false;
    }

    strncpy(logger_state.log_dir, log_dir, sizeof(logger_state.log_dir) - 1);
    strncpy(logger_state.log_filename, log_filename, sizeof(logger_state.log_filename) - 1);
    snprintf(logger_state.log_filepath, sizeof(logger_state.log_filepath),
             "%s/%s", logger_state.log_dir, logger_state.log_filename);

    logger_state.max_size_bytes = max_size_bytes;
    logger_state.max_files = max_files;
    logger_state.fp = fopen(logger_state.log_filepath, "a");

    if (!logger_state.fp) {
        perror("Failed to open log file");
        return false;
    }

    if (pthread_mutex_init(&logger_state.mutex, NULL) != 0) {
        perror("Failed to initialize logger mutex");
        fclose(logger_state.fp);
        return false;
    }

    logger_state.initialized = true;
    return true;
}

void logger_close(void) {
    if (!logger_state.initialized) return;
    pthread_mutex_lock(&logger_state.mutex);
    if (logger_state.fp) {
        fclose(logger_state.fp);
        logger_state.fp = NULL;
    }
    pthread_mutex_unlock(&logger_state.mutex);
    pthread_mutex_destroy(&logger_state.mutex);
    logger_state.initialized = false;
}

void logger_write(LogLevel level, const char* source, const char* format, ...) {
    if (!logger_state.initialized) return;

    static const char* level_strings[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

    // タイムスタンプ生成
    char time_buf[32];
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

    // メッセージ整形
    char log_buf[2048];
    va_list args;
    va_start(args, format);
    vsnprintf(log_buf, sizeof(log_buf), format, args);
    va_end(args);

    // コンソールとファイルへの出力
    pthread_mutex_lock(&logger_state.mutex);

    // ローテーションチェック
    if (ftell(logger_state.fp) >= logger_state.max_size_bytes) {
        rotate_logs();
    }

    // ログ出力
    fprintf(stdout, "[%s] [%-5s] [%s] %s\n", time_buf, level_strings[level], source, log_buf);
    if (logger_state.fp) {
        fprintf(logger_state.fp, "[%s] [%-5s] [%s] %s\n", time_buf, level_strings[level], source, log_buf);
        fflush(logger_state.fp);
    }

    pthread_mutex_unlock(&logger_state.mutex);
}

// --- 静的ヘルパー関数 ---

static void rotate_logs(void) {
    if (!logger_state.fp) return;
    fclose(logger_state.fp);
    logger_state.fp = NULL;

    char old_path[1300];
    char new_path[1300];

    // 1. Delete the oldest file, if it exists (e.g., driver.log.10 for max_files=10)
    snprintf(old_path, sizeof(old_path), "%s.%d", logger_state.log_filepath, logger_state.max_files);
    remove(old_path);

    // 2. Shift files up by one (e.g., driver.log.9 -> driver.log.10, ..., driver.log.1 -> driver.log.2)
    for (int i = logger_state.max_files - 1; i > 0; --i) {
        snprintf(old_path, sizeof(old_path), "%s.%d", logger_state.log_filepath, i);
        snprintf(new_path, sizeof(new_path), "%s.%d", logger_state.log_filepath, i + 1);

        struct stat st;
        if (stat(old_path, &st) == 0) {
            rename(old_path, new_path);
        }
    }

    // 3. Rename the current log file to be the first rotated file (e.g., driver.log -> driver.log.1)
    snprintf(new_path, sizeof(new_path), "%s.1", logger_state.log_filepath);
    rename(logger_state.log_filepath, new_path);

    // 4. Open a new log file
    logger_state.fp = fopen(logger_state.log_filepath, "a");
    if (!logger_state.fp) {
        fprintf(stderr, "FATAL: Failed to open new log file after rotation: %s\n", strerror(errno));
    }
}

// ログディレクトリを作成する (mkdir -p のような動作)
static bool create_log_directory(const char* dir) {
    char tmp[256];
    char* p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", dir);
    len = strlen(tmp);
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }

    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (mkdir(tmp, S_IRWXU) != 0 && errno != EEXIST) {
                return false;
            }
            *p = '/';
        }
    }
    if (mkdir(tmp, S_IRWXU) != 0 && errno != EEXIST) {
        return false;
    }
    return true;
}
