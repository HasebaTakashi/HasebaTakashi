#include "command_server.h"
#include "commands.h"
#include "app_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024
#define LARGE_RESPONSE_SIZE (32 * 1024 * 16)

#define LOG_SERVER_INFO(msg, ...) printf("[SERVER_INFO] " msg "\n", ##__VA_ARGS__)
#define LOG_SERVER_ERROR(msg, ...) fprintf(stderr, "[SERVER_ERROR] " msg "\n", ##__VA_ARGS__)

typedef struct {
    int client_fd;
    struct sockaddr_in client_addr;
    CommandServer* server;
} ClientContext;

static char* command_proc(CommandServer* server, char* command_line);
static void* client_thread_proc(void* context);
static void* server_loop(void* server_ptr);
static char* serialize_data_proc(SamplingData* data, int ch_id_filter);

CommandServer* command_server_create(int port, SamplingManager* manager) {
    CommandServer* server = (CommandServer*)calloc(1, sizeof(CommandServer));
    if (!server) {
        LOG_SERVER_ERROR("Failed to allocate memory for CommandServer");
        return NULL;
    }
    server->port = port;
    server->manager = manager;

    server->server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->server_fd < 0) {
        LOG_SERVER_ERROR("Socket creation failed");
        free(server);
        return NULL;
    }

    int opt = 1;
    setsockopt(server->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server->server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        LOG_SERVER_ERROR("Socket bind failed");
        close(server->server_fd);
        free(server);
        return NULL;
    }

    if (listen(server->server_fd, MAX_CLIENTS) < 0) {
        LOG_SERVER_ERROR("Socket listen failed");
        close(server->server_fd);
        free(server);
        return NULL;
    }

    LOG_SERVER_INFO("Server initialized on port %d", port);
    return server;
}

bool command_server_start(CommandServer* server) {
    if (!server) return false;
    server->running = true;
    if (pthread_create(&server->server_thread_id, NULL, server_loop, server) != 0) {
        LOG_SERVER_ERROR("Failed to create server accept thread");
        server->running = false;
        return false;
    }
    return true;
}

void command_server_stop(CommandServer* server) {
    if (server && server->running) {
        server->running = false;
        shutdown(server->server_fd, SHUT_RDWR);
        close(server->server_fd);
        pthread_join(server->server_thread_id, NULL);
        LOG_SERVER_INFO("Server stopped.");
    }
}

void command_server_destroy(CommandServer* server) {
    if (!server) return;
    if (server->running) {
        command_server_stop(server);
    }
    free(server);
}

static void* server_loop(void* server_ptr) {
    CommandServer* server = (CommandServer*)server_ptr;
    LOG_SERVER_INFO("Server accept loop started.");
    while (server->running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server->server_fd, (struct sockaddr*)&client_addr, &client_len);

        if (client_fd < 0) {
            if (server->running) LOG_SERVER_ERROR("Accept failed");
            break;
        }

        ClientContext* context = (ClientContext*)malloc(sizeof(ClientContext));
        context->client_fd = client_fd;
        context->client_addr = client_addr;
        context->server = server;

        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, client_thread_proc, context) != 0) {
            LOG_SERVER_ERROR("Failed to create client thread");
            close(client_fd);
            free(context);
        }
        pthread_detach(client_thread);
    }
    LOG_SERVER_INFO("Server accept loop finished.");
    return NULL;
}

static void* client_thread_proc(void* context) {
    ClientContext* ctx = (ClientContext*)context;
    char buffer[BUFFER_SIZE];
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ctx->client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    LOG_SERVER_INFO("Client connected: %s", client_ip);

    FILE* client_stream = fdopen(ctx->client_fd, "r+");
    if (!client_stream) {
        LOG_SERVER_ERROR("fdopen failed for client %s", client_ip);
        close(ctx->client_fd);
        free(ctx);
        return NULL;
    }
    setvbuf(client_stream, NULL, _IOLBF, 0);

    while (fgets(buffer, sizeof(buffer), client_stream)) {
        buffer[strcspn(buffer, "\r\n")] = 0;
        LOG_SERVER_INFO("Recv from %s: %s", client_ip, buffer);

        char* response = command_proc(ctx->server, buffer);
        if (response) {
            fprintf(client_stream, "%s\n", response);
            free(response);
        }
    }

    LOG_SERVER_INFO("Client disconnected: %s", client_ip);
    fclose(client_stream);
    free(ctx);
    return NULL;
}

static char* serialize_data_proc(SamplingData* data, int ch_id_filter) {
    if (!data) return strdup("NoData");

    char* response = (char*)malloc(LARGE_RESPONSE_SIZE);
    if (!response) return strdup("Error: Out of memory");

    response[0] = '\0';
    char* current_pos = response;
    int remaining_size = LARGE_RESPONSE_SIZE;
    bool channel_found = (ch_id_filter == 0);

    for (int i = 0; i < data->num_channels; ++i) {
        struct DataPerChannel* dpc = &data->data_per_channel[i];
        if (ch_id_filter != 0 && dpc->channel_id != ch_id_filter) {
            continue;
        }
        channel_found = true;
        if (dpc->ch_index == VMONITOR2_CH_NO) {
            for (int j = 0; j < dpc->sampling_no; ++j) {
                int n = snprintf(current_pos, remaining_size, "%d,", dpc->buffer.pulse[j]);
                current_pos += n;
                remaining_size -= n;
            }
        } else {
            for (int j = 0; j < dpc->sampling_no; ++j) {
                int n = snprintf(current_pos, remaining_size, "%d,", dpc->buffer.ad[j]);
                current_pos += n;
                remaining_size -= n;
            }
        }
    }

    if (!channel_found) {
        snprintf(response, LARGE_RESPONSE_SIZE, "Error Not exist ch:%d", ch_id_filter);
        return response;
    }

    struct tm* tm_info = localtime(&data->data_time.tv_sec);
    snprintf(current_pos, remaining_size, "%d,%d,%d,%d,%d,%d",
             tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);

    return response;
}

static char* alloc_response_int(int value) {
    char* response = malloc(16);
    snprintf(response, 16, "%d", value);
    return response;
}

static char* alloc_response_double(double value) {
    char* response = malloc(32);
    snprintf(response, 32, "%f", value);
    return response;
}

static char* command_proc(CommandServer* server, char* command_line) {
    char* saveptr;
    char* command = strtok_r(command_line, ",", &saveptr);
    if (!command) return alloc_response_int(CMD_ERR_UNKNOWN);

    SamplingManager* sm = server->manager;

    if (strcmp(command, CMD_CHECK_VERSION) == 0) {
        return strdup("C-Driver Ver. 1.0.0");
    }
    if (strcmp(command, CMD_GET_CH_ID) == 0) {
        char* response = malloc(LARGE_RESPONSE_SIZE);
        response[0] = '\0';
        char* current_pos = response;
        for (int i = 0; i < sm->num_channels; ++i) {
            current_pos += sprintf(current_pos, "%d%s", sm->channels[i]->id, (i == sm->num_channels - 1) ? "" : ",");
        }
        return response;
    }
    if (strcmp(command, CMD_GET_CH_INFO) == 0) {
        char* arg1 = strtok_r(NULL, ",", &saveptr);
        if(!arg1) return alloc_response_int(CMD_ERR_ARGUMENTS);
        int ch_id = atoi(arg1);
        for(int i=0; i < sm->num_channels; ++i) {
            if(sm->channels[i]->id == ch_id) {
                SamplingChannel* ch = sm->channels[i];
                char* response = malloc(256);
                sprintf(response, "%d,%d,%f,%f,%f,%d", ch->id, ch->sampling_no, ch->gain, ch->max_range, ch->min_range, ch->resolution);
                return response;
            }
        }
        return strdup("");
    }
    if (strcmp(command, CMD_CHECK_DATA) == 0) {
        char* arg1 = strtok_r(NULL, ",", &saveptr);
        if (!arg1) return alloc_response_int(CMD_ERR_ARGUMENTS);
        int buffer_no = atoi(arg1);
        if (buffer_no > 0 && buffer_no <= sm->num_queues) {
            return alloc_response_int(queue_data_count(sm->data_queues[buffer_no - 1]) > 0 ? 1 : 0);
        }
        return alloc_response_int(CMD_ERR_BUFFER_NO);
    }
    if (strcmp(command, CMD_GET_DATA) == 0) {
        char* arg1 = strtok_r(NULL, ",", &saveptr);
        if (!arg1) return alloc_response_int(CMD_ERR_ARGUMENTS);
        int buffer_no = atoi(arg1);
        if (buffer_no > 0 && buffer_no <= sm->num_queues) {
            SamplingData* data = queue_dequeue(sm->data_queues[buffer_no - 1]);
            char* response = serialize_data_proc(data, 0); // 0 means no filter
            if (data) sampling_data_destroy(data);
            return response;
        }
        return alloc_response_int(CMD_ERR_BUFFER_NO);
    }
    if (strcmp(command, CMD_GET_CH_DATA) == 0) {
        char* arg1 = strtok_r(NULL, ",", &saveptr);
        char* arg2 = strtok_r(NULL, ",", &saveptr);
        if (!arg1 || !arg2) return alloc_response_int(CMD_ERR_ARGUMENTS);
        int buffer_no = atoi(arg1);
        int ch_id = atoi(arg2);
        if (buffer_no > 0 && buffer_no <= sm->num_queues) {
            SamplingData* data = queue_dequeue(sm->data_queues[buffer_no - 1]);
            char* response = serialize_data_proc(data, ch_id);
            if (data) sampling_data_destroy(data);
            return response;
        }
        return alloc_response_int(CMD_ERR_BUFFER_NO);
    }
    if (strcmp(command, CMD_GET_DATA_NO) == 0) {
        char* arg1 = strtok_r(NULL, ",", &saveptr);
        if (!arg1) return alloc_response_int(CMD_ERR_ARGUMENTS);
        int buffer_no = atoi(arg1);
        if (buffer_no > 0 && buffer_no <= sm->num_queues) {
            return alloc_response_int(queue_data_count(sm->data_queues[buffer_no - 1]));
        }
        return alloc_response_int(CMD_ERR_BUFFER_NO);
    }
    if (strcmp(command, CMD_CLEAR_BUFFER) == 0) {
        char* arg1 = strtok_r(NULL, ",", &saveptr);
        if (!arg1) return alloc_response_int(CMD_ERR_ARGUMENTS);
        int buffer_no = atoi(arg1);
        if (buffer_no > 0 && buffer_no <= sm->num_queues) {
            queue_clear(sm->data_queues[buffer_no - 1]);
            return alloc_response_int(CMD_RESPONSE_OK);
        }
        return alloc_response_int(CMD_ERR_BUFFER_NO);
    }
    if (strcmp(command, CMD_GET_TERMINAL_VOLTAGE) == 0) {
        char* arg1 = strtok_r(NULL, ",", &saveptr);
        if (!arg1) return alloc_response_int(CMD_ERR_ARGUMENTS);
        int ch_no = atoi(arg1);
        return alloc_response_double(sampling_manager_get_terminal_voltage(sm, ch_no));
    }
    if (strcmp(command, CMD_GET_GAIN) == 0) {
        char* arg1 = strtok_r(NULL, ",", &saveptr);
        if (!arg1) return alloc_response_int(CMD_ERR_ARGUMENTS);
        int ch_no = atoi(arg1);
        return alloc_response_double(sampling_manager_get_gain(sm, ch_no));
    }
    if (strcmp(command, CMD_SET_GAIN) == 0) {
        char* arg1 = strtok_r(NULL, ",", &saveptr);
        char* arg2 = strtok_r(NULL, ",", &saveptr);
        if (!arg1 || !arg2) return alloc_response_int(CMD_ERR_ARGUMENTS);
        int ch_no = atoi(arg1);
        double gain = atof(arg2);
        if (sampling_manager_set_gain(sm, ch_no, gain)) {
            return alloc_response_int(CMD_RESPONSE_OK);
        }
        return alloc_response_int(CMD_ERR_ARGUMENTS);
    }
    if (strcmp(command, CMD_GET_DI) == 0) {
        char* arg1 = strtok_r(NULL, ",", &saveptr);
        char* arg2 = strtok_r(NULL, ",", &saveptr);
        if (!arg1 || !arg2) return alloc_response_int(CMD_ERR_ARGUMENTS);
        DeviceType dev_id = (DeviceType)atoi(arg1);
        int ch_no = atoi(arg2);
        return alloc_response_int(sampling_manager_get_digital_in(sm, dev_id, ch_no));
    }
    if (strcmp(command, CMD_GET_DO) == 0) {
        char* arg1 = strtok_r(NULL, ",", &saveptr);
        char* arg2 = strtok_r(NULL, ",", &saveptr);
        if (!arg1 || !arg2) return alloc_response_int(CMD_ERR_ARGUMENTS);
        DeviceType dev_id = (DeviceType)atoi(arg1);
        int ch_no = atoi(arg2);
        return alloc_response_int(sampling_manager_get_digital_out(sm, dev_id, ch_no));
    }
    if (strcmp(command, CMD_SET_DO) == 0) {
        char* arg1 = strtok_r(NULL, ",", &saveptr);
        char* arg2 = strtok_r(NULL, ",", &saveptr);
        char* arg3 = strtok_r(NULL, ",", &saveptr);
        if (!arg1 || !arg2 || !arg3) return alloc_response_int(CMD_ERR_ARGUMENTS);
        DeviceType dev_id = (DeviceType)atoi(arg1);
        int ch_no = atoi(arg2);
        int status = atoi(arg3);
        if(sampling_manager_set_digital_out(sm, dev_id, ch_no, status)) {
            return alloc_response_int(CMD_RESPONSE_OK);
        }
        return alloc_response_int(CMD_ERR_ARGUMENTS);
    }

    return alloc_response_int(CMD_ERR_UNKNOWN);
}
