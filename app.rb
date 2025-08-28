# frozen_string_literal: true

require 'sinatra'
require 'faye/websocket'
require 'puma'

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
  puts "Broadcasting message to #{settings.sockets.count} clients: #{message}"
  settings.sockets.each { |s| s.send(message) }
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
