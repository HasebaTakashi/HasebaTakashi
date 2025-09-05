# frozen_string_literal: true

require 'sinatra'
require 'faye/websocket'
require 'puma'
require 'json'
require_relative 'mqtt_client'

# --- Server and MQTT Setup ---
set :server, 'puma'
set :environment, :production
set :bind, '0.0.0.0'
set :port, 46_000
set :sockets, []

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

# --- Page Routes ---
get '/' do
  erb :index
end

get '/settings_editor' do
  erb :settings_editor
end

# --- WebSocket Connection ---
get '/socket' do
  if Faye::WebSocket.websocket?(request.env)
    ws = Faye::WebSocket.new(request.env)
    ws.on(:open) { |_| settings.sockets << ws }
    ws.on(:close) { |_| settings.sockets.delete(ws) }
    ws.rack_response
  else
    redirect '/'
  end
end

# --- API Routes ---
post '/api/request_settings' do
  mqtt.send_data('RequestSetting', '')
  status 204
end

post '/api/publish_settings' do
  content_type :json
  request.body.rewind
  begin
    settings_data = JSON.parse(request.body.read)
    fields = ["DetectID", "DetectName", "AnalyzeID", "OperationMode", "LearningNo", "MoveAveNo", "Coefficient1", "Coefficient2", "Coefficient3", "Threshold1", "Threshold2", "Threshold3", "Enable"]

    message_parts = []
    # The JSON from frontend is {"Detect1":{...}, "Detect2":{...}}
    # We need to sort by DetectID to ensure consistent message order
    sorted_settings = settings_data.values.sort_by { |v| v['DetectID'] }

    sorted_settings.each do |setting|
      fields.each do |field|
        message_parts << setting[field]
      end
    end

    mqtt.send_data('SettingChange', message_parts.join(','))
    { success: 'Settings published to MQTT.' }.to_json
  rescue => e
    status 500
    { error: "Failed to publish settings: #{e.message}" }.to_json
  end
end

post '/api/delete_setting' do
  content_type :json
  request.body.rewind
  begin
    setting_to_delete = JSON.parse(request.body.read)
    fields = ["DetectID", "DetectName", "AnalyzeID", "OperationMode", "LearningNo", "MoveAveNo", "Coefficient1", "Coefficient2", "Coefficient3", "Threshold1", "Threshold2", "Threshold3", "Enable"]

    message_parts = fields.map { |field| setting_to_delete[field] }

    mqtt.send_data('SettingDelete', message_parts.join(','))
    { success: 'Delete request published to MQTT.' }.to_json
  rescue => e
    status 500
    { error: "Failed to publish delete request: #{e.message}" }.to_json
  end
end
