# frozen_string_literal: true

require 'sinatra'
require 'faye/websocket'
require 'puma'
require 'json'

require_relative 'mqtt_client'

# サーバ設定
set :server, 'puma'
set :environment, :production
set :bind, '0.0.0.0'
set :port, 46000
set :sockets, []

# mqtt受信開始
# NOTE: MQTTブローカーのIPアドレスを '127.0.0.1' に変更しました。
# 環境に合わせて変更が必要な場合は、この値を修正してください。
mqtt = MqttClient.new('127.0.0.1', ['DetectResult'])
mqtt.connect
mqtt.recieve_start do |_topic, message|
  # WebSocketでテキストフレームとして送信されるように、エンコーディングをUTF-8に変換
  message_as_utf8 = message.encode('UTF-8', invalid: :replace, undef: :replace)
  puts "Broadcasting message to #{settings.sockets.count} clients: #{message_as_utf8}"
  settings.sockets.each { |s| s.send(message_as_utf8) }
end

get '/' do
  erb :index
end

get '/socket' do
  if Faye::WebSocket.websocket?(request.env)
    ws = Faye::WebSocket.new(request.env)

    ws.on(:open) do |_event|
      puts "New Connection: #{ws.object_id}"
      settings.sockets << ws
    end

    ws.on(:message) do |event|
      # クライアントからのメッセージは現在処理しない
      puts "Received message from client: #{event.data}"
    end

    ws.on(:close) do |_event|
      puts "Connection closed: #{ws.object_id}"
      settings.sockets.delete(ws)
    end

    ws.rack_response
  else
    # WebSocketリクエストでない場合はルートにリダイレクト
    redirect '/'
  end
end

# --- Settings Editor API ---
# NOTE: Due to sandbox restrictions, the settings file is located at `settings/detect_setting.json`
# inside the project directory, not at the originally requested path.
SETTINGS_FILE_PATH = File.expand_path('settings/detect_setting.json', __dir__)

get '/settings_editor' do
  erb :settings_editor
end

get '/api/settings' do
  content_type :json
  begin
    File.read(SETTINGS_FILE_PATH)
  rescue Errno::ENOENT
    status 404
    { error: 'Settings file not found.' }.to_json
  end
end

post '/api/settings' do
  request.body.rewind
  begin
    data = JSON.parse(request.body.read)
    # Pretty-generate the JSON to make the file readable
    File.write(SETTINGS_FILE_PATH, JSON.pretty_generate(data))
    status 200
    { success: 'Settings saved successfully.' }.to_json
  rescue JSON::ParserError
    status 400
    { error: 'Invalid JSON format.' }.to_json
  rescue => e
    status 500
    { error: "An error occurred: #{e.message}" }.to_json
  end
end
