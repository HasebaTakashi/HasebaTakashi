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
mqtt = MqttClient.new('127.0.0.1', ['DetectResult', 'ResponseSetting'])
mqtt.connect
mqtt.recieve_start do |topic, message|
  payload = message.encode('UTF-8', invalid: :replace, undef: :replace)

  ws_message = case topic
               when 'DetectResult'
                 { type: 'detect_result', data: payload }.to_json
               when 'ResponseSetting'
                 { type: 'settings_data', data: payload }.to_json
               else
                 nil
               end

  if ws_message
    puts "Broadcasting #{topic} to #{settings.sockets.count} clients"
    settings.sockets.each { |s| s.send(ws_message) }
  end
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

# --- Page Routes ---
get '/settings_editor' do
  erb :settings_editor
end

# --- API for requesting settings from the diagnostic app ---
post '/api/request_settings' do
  mqtt.send_data('RequestSetting', '') # Message content can be empty
  status 204 # No content
end

# --- API for publishing settings changes ---
post '/api/publish_settings' do
  content_type :json
  request.body.rewind
  begin
    settings_data = JSON.parse(request.body.read)

    # Define the order of fields for the CSV message
    fields = [
      "DetectID", "DetectName", "AnalyzeID", "OperationMode",
      "LearningNo", "MoveAveNo", "Coefficient1", "Coefficient2",
      "Coefficient3", "Threshold1", "Threshold2", "Threshold3", "Enable"
    ]

    # Build the long CSV string from the received JSON object
    message_parts = []
    settings_data.each do |_, setting|
      fields.each do |field|
        message_parts << setting[field]
      end
    end

    mqtt_message = message_parts.join(',')

    # Publish to the 'SettingChange' topic
    mqtt.send_data('SettingChange', mqtt_message)

    status 200
    { success: 'Settings published to MQTT successfully.' }.to_json
  rescue JSON::ParserError
    status 400
    { error: 'Invalid JSON format.' }.to_json
  rescue => e
    status 500
    { error: "An error occurred while publishing to MQTT: #{e.message}" }.to_json
  end
end
