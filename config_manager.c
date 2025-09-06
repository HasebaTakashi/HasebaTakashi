#include "config_manager.h"
#include "cJSON.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- ヘルパー関数プロトタイプ ---
static char* read_file_to_string(const char* filepath);
static bool parse_app_settings(const char* json_str, AppConfig* cfg);
static bool parse_device_settings(const char* json_str, DeviceSetting** settings, int* count);
static bool parse_channel_settings(const char* json_str, ChannelSetting** settings, int* count);
static char* load_config_file(const char* dir, const char* basename);

// --- 公開関数 ---

AllConfigs* config_load_all(const char* settings_dir) {
    AllConfigs* configs = (AllConfigs*)calloc(1, sizeof(AllConfigs));
    if (!configs) {
        LOG_ERROR("CONFIG", "Failed to allocate memory for AllConfigs");
        return NULL;
    }

    bool success = true;

    // アプリケーション設定の読み込み
    char* app_json = load_config_file(settings_dir, "app_setting.json");
    if (app_json) {
        if (!parse_app_settings(app_json, &configs->app_config)) {
            LOG_ERROR("CONFIG", "Failed to parse app settings.");
            success = false;
        }
        free(app_json);
    } else {
        LOG_ERROR("CONFIG", "Failed to load app settings file.");
        success = false;
    }

    // デバイス設定の読み込み
    if (success) {
        char* dev_json = load_config_file(settings_dir, "device_setting.json");
        if (dev_json) {
            if (!parse_device_settings(dev_json, &configs->device_settings, &configs->num_device_settings)) {
                LOG_ERROR("CONFIG", "Failed to parse device settings.");
                success = false;
            }
            free(dev_json);
        } else {
            LOG_ERROR("CONFIG", "Failed to load device settings file.");
            success = false;
        }
    }

    // チャンネル設定の読み込み
    if (success) {
        char* ch_json = load_config_file(settings_dir, "channel_setting.json");
        if (ch_json) {
            if (!parse_channel_settings(ch_json, &configs->channel_settings, &configs->num_channel_settings)) {
                LOG_ERROR("CONFIG", "Failed to parse channel settings.");
                success = false;
            }
            free(ch_json);
        } else {
            LOG_ERROR("CONFIG", "Failed to load channel settings file.");
            success = false;
        }
    }

    if (!success) {
        config_destroy(configs);
        return NULL;
    }

    LOG_INFO("CONFIG", "All settings loaded successfully.");
    return configs;
}

void config_destroy(AllConfigs* configs) {
    if (!configs) return;
    free(configs->device_settings);
    free(configs->channel_settings);
    free(configs);
}

// --- 静的ヘルパー関数 ---

static char* read_file_to_string(const char* filepath) {
    FILE* fp = fopen(filepath, "rb");
    if (!fp) return NULL;

    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* buffer = (char*)malloc(length + 1);
    if (!buffer) {
        fclose(fp);
        return NULL;
    }

    if (fread(buffer, 1, length, fp) != length) {
        fclose(fp);
        free(buffer);
        return NULL;
    }

    buffer[length] = '\0';
    fclose(fp);
    return buffer;
}

// メインのファイル読み込み、失敗したらバックアップを試す
static char* load_config_file(const char* dir, const char* basename) {
    char path[1024];
    char bak_path[1024];

    // パス文字列を安全に作成
    char* basename_dup = strdup(basename);
    char* dot = strrchr(basename_dup, '.');
    if (dot) *dot = '\0'; // 拡張子を削除
    snprintf(path, sizeof(path), "%s/%s", dir, basename);
    snprintf(bak_path, sizeof(bak_path), "%s/%s_bak.json", dir, basename_dup);
    free(basename_dup);

    // 1. メインファイルを読み込む
    char* content = read_file_to_string(path);
    if (content) {
        LOG_INFO("CONFIG", "Loaded '%s'", path);
        // 2. 成功したらバックアップを書き込む
        FILE* bak_fp = fopen(bak_path, "w");
        if (bak_fp) {
            fputs(content, bak_fp);
            fclose(bak_fp);
        } else {
            LOG_WARN("CONFIG", "Could not write backup to '%s'", bak_path);
        }
        return content;
    }

    // 3. 失敗したらバックアップを試す
    LOG_WARN("CONFIG", "Could not open '%s', trying backup '%s'", path, bak_path);
    content = read_file_to_string(bak_path);
    if (content) {
        LOG_INFO("CONFIG", "Loaded backup '%s'", bak_path);
        // 4. バックアップから元ファイルを復元する
        FILE* main_fp = fopen(path, "w");
        if(main_fp) {
            fputs(content, main_fp);
            fclose(main_fp);
            LOG_INFO("CONFIG", "Restored '%s' from backup.", path);
        } else {
            LOG_ERROR("CONFIG", "Failed to restore original file from backup.");
        }
    }
    return content;
}

// cJSONのヘルパーマクロ
#define GET_JSON_INT(obj, key, target) { cJSON* item = cJSON_GetObjectItem(obj, key); if(item) target = item->valueint; }
#define GET_JSON_DOUBLE(obj, key, target) { cJSON* item = cJSON_GetObjectItem(obj, key); if(item) target = item->valuedouble; }
#define GET_JSON_STRING(obj, key, target, len) { cJSON* item = cJSON_GetObjectItem(obj, key); if(item && item->valuestring) strncpy(target, item->valuestring, len-1); }

static bool parse_app_settings(const char* json_str, AppConfig* cfg) {
    cJSON* root = cJSON_Parse(json_str);
    if (!root) return false;

    GET_JSON_INT(root, "BufferNo", cfg->buffer_no);
    GET_JSON_STRING(root, "ServerIP", cfg->server_ip, sizeof(cfg->server_ip));
    GET_JSON_INT(root, "ServerPort", cfg->server_port);
    GET_JSON_INT(root, "PulseCountThreshold", cfg->pulse_count_threshold);
    GET_JSON_INT(root, "IdleRebootType", cfg->idle_reboot_type);
    GET_JSON_INT(root, "IdleRebootTimeout", cfg->idle_reboot_timeout);

    for (int i = 0; i < cfg->buffer_no; ++i) {
        char key[32];
        sprintf(key, "Buffer%dSize", i + 1);
        GET_JSON_INT(root, key, cfg->buffer_sizes[i]);
    }

    cJSON_Delete(root);
    return true;
}

static bool parse_device_settings(const char* json_str, DeviceSetting** settings, int* count) {
    cJSON* root = cJSON_Parse(json_str);
    if (!root) return false;

    *count = cJSON_GetArraySize(root);
    *settings = (DeviceSetting*)calloc(*count, sizeof(DeviceSetting));
    if (!*settings) {
        cJSON_Delete(root);
        return false;
    }

    cJSON* dev_item = NULL;
    int i = 0;
    // cJSON_ArrayForEachはオブジェクトのイテレーションにも使える
    cJSON_ArrayForEach(dev_item, root) {
        // dev_item は "Device1" のようなキーを持つアイテム。その値(child)が設定オブジェクト
        cJSON* dev_obj = dev_item; // The item itself is the value object in this iteration
        DeviceSetting* ds = &(*settings)[i];
        GET_JSON_INT(dev_obj, "DeviceID", ds->id);
        GET_JSON_STRING(dev_obj, "DeviceName", ds->name, sizeof(ds->name));
        GET_JSON_INT(dev_obj, "ChannelNo", ds->channel_no);
        GET_JSON_INT(dev_obj, "EnableDevice", ds->enable);
        i++;
    }

    cJSON_Delete(root);
    return true;
}

static bool parse_channel_settings(const char* json_str, ChannelSetting** settings, int* count) {
    cJSON* root = cJSON_Parse(json_str);
    if (!root) return false;

    *count = cJSON_GetArraySize(root);
    *settings = (ChannelSetting*)calloc(*count, sizeof(ChannelSetting));
    if (!*settings) {
        cJSON_Delete(root);
        return false;
    }

    cJSON* ch_item = NULL;
    int i = 0;
    // cJSON_ArrayForEachはオブジェクトのイテレーションにも使える
    cJSON_ArrayForEach(ch_item, root) {
        cJSON* ch_obj = ch_item; // The item itself is the value object
        ChannelSetting* cs = &(*settings)[i];
        GET_JSON_INT(ch_obj, "ChannelID", cs->channel_id);
        GET_JSON_STRING(ch_obj, "ChannelName", cs->channel_name, sizeof(cs->channel_name));
        GET_JSON_INT(ch_obj, "DeviceID", cs->device_id);
        GET_JSON_INT(ch_obj, "ChannelIndex", cs->ch_index);
        GET_JSON_INT(ch_obj, "SamplingFrequency", cs->sampling_freq);
        GET_JSON_DOUBLE(ch_obj, "Gain", cs->gain);
        GET_JSON_DOUBLE(ch_obj, "MaxRange", cs->max_range);
        GET_JSON_DOUBLE(ch_obj, "MinRange", cs->min_range);
        GET_JSON_INT(ch_obj, "Resolution", cs->resolution);
        GET_JSON_INT(ch_obj, "InputType", cs->input_type);
        i++;
    }

    cJSON_Delete(root);
    return true;
}
