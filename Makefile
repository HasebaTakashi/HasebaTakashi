# コンパイラとフラグ
CC = gcc
# 警告をすべて有効にし、デバッグ情報を含め、C11標準でコンパイル
# _POSIX_C_SOURCE=200809L は clock_gettime のために必要
CFLAGS = -Wall -Wextra -g -std=c11 -D_POSIX_C_SOURCE=200809L

# リンカフラグ
# -L.: カレントディレクトリをライブラリ検索パスに追加 (libvmonitor2.soのため)
# -lvmonitor2: libvmonitor2.so をリンク
# -pthread: POSIXスレッドライブラリをリンク
# -lrt: real-timeライブラリをリンク (clock_gettimeのため)
LDFLAGS = -L. -lvmonitor2 -pthread -lrt

# 生成する実行ファイル名
TARGET = vmonitor_c_app

# ソースファイル一覧
SRCS = main.c sampling_data.c sampling_queue.c vmonitor2_board.c sampling_device.c sampling_manager.c command_server.c

# オブジェクトファイル一覧 (ソースファイル名から自動生成)
OBJS = $(SRCS:.c=.o)

# デフォルトターゲット: 'make' と打つとこれが実行される
all: $(TARGET)

# リンクのルール: オブジェクトファイルをまとめて実行ファイルを生成
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo "================================================================="
	@echo "Successfully built '$(TARGET)'"
	@echo "Make sure 'libvmonitor2.so' is in the same directory or in your LD_LIBRARY_PATH."
	@echo "Run with: ./$(TARGET)"
	@echo "================================================================="


# コンパイルのルール: .cファイルから.oファイルを生成
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# クリーンルール: 'make clean' で実行ファイルとオブジェクトファイルを削除
clean:
	rm -f $(TARGET) $(OBJS)
	@echo "Cleaned up build artifacts."

# 偽ターゲットの宣言
.PHONY: all clean
