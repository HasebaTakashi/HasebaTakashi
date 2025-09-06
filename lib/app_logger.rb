# frozen_string_literal: true

require 'logger'
require 'singleton'

# AppLogger: A singleton logging service for the application.
#
# Usage:
#   AppLogger.instance.configure(log_path: '/path/to/log', level: 'INFO')
#   AppLogger.instance.info('This is a message')
#
class AppLogger
  include Singleton

  attr_reader :logger

  def initialize
    # Default logger to STDOUT until configured
    @logger = Logger.new($stdout)
    @logger.level = Logger::INFO
  end

  # Configures the logger with a file path and log level.
  # @param log_path [String] The path to the log file.
  # @param level [String] The log level ('DEBUG', 'INFO', 'WARN', 'ERROR', 'FATAL').
  def configure(log_path:, level:)
    # Ensure the directory for the log file exists
    log_dir = File.dirname(log_path)
    FileUtils.mkdir_p(log_dir) unless File.directory?(log_dir)

    @logger = Logger.new(log_path, 'daily') # Rotates daily
    @logger.level = Logger.const_get(level.upcase)
  rescue NameError
    @logger.warn("Invalid log level '#{level}'. Defaulting to INFO.")
    @logger.level = Logger::INFO
  end

  # Delegate logging methods to the underlying logger instance
  def method_missing(method_name, *args, &block)
    if @logger.respond_to?(method_name)
      @logger.send(method_name, *args, &block)
    else
      super
    end
  end

  def respond_to_missing?(method_name, include_private = false)
    @logger.respond_to?(method_name) || super
  end
end
