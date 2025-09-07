# frozen_string_literal: true

# ダミーデバイスクラス
class DummyDevice
  CH_NO = 17
  SAMPLING_FREQUENCY = 25_600
  DI_NO = 4
  DO_NO = 8

  def initialize
    @datas = Array.new(CH_NO - 1) { Array.new(SAMPLING_FREQUENCY) { |n| n } }
    @datas << Array.new(1) { 200 }

    # ゲイン設定
    @gains = Array.new(CH_NO - 1) { 1 }
    # 入力種別
    @input_types = Array.new(CH_NO - 1) { 0 }
    # DI
    @digital_in_status = Array.new(DI_NO) { 0 }
    # DO
    @digital_out_status = Array.new(DO_NO) { 0 }
  end

  # サンプリング周波数
  def sampling_freq
    SAMPLING_FREQUENCY
  end

  # サンプリング周波数確認
  def check_sampling_freq(ch_index, sampling_freq)
    if ch_index.positive? && ch_index < CH_NO
      sampling_freq == SAMPLING_FREQUENCY
    elsif ch_index == CH_NO
      sampling_freq == 1
    else
      false
    end
  end

  def open
    p 'DummyDevice Open'
  end

  def close
    p 'DummyDevice Close'
  end

  # ADCH数取得
  def ad_ch_no
    CH_NO - 1
  end

  def start_sampling
    p 'Sampling Start'
  end

  def stop_sampling
    p 'Sampling Stop'
  end

  def get_digital_in(ch_index)
    p "GetDI DINo:#{ch_index}"
    @digital_in_status[ch_index - 1]
  end

  def get_digital_out(ch_index)
    p "GetDO DONo:#{ch_index}"
    @digital_out_status[ch_index - 1]
  end

  def set_digital_out(ch_index, status)
    p "SetDO DONo:#{ch_index} Status:#{status}"
    @digital_out_status[ch_index - 1] = status
    0
  end

  def check_data
    sleep 1
    true
  end

  def get_data(sampling_frequency:)
    # This dummy ignores the passed frequency and uses its own constant.
    data_hash = {}

    # Generate sine wave data for analog channels
    (0...CH_NO - 1).each do |i|
      samples = Array.new(SAMPLING_FREQUENCY) do |j|
        tmp = j.to_f / SAMPLING_FREQUENCY
        (100 * Math.sin(2 * Math::PI * (i + 1) * 2 * tmp)).to_i # Use channel index to vary frequency
      end
      data_hash[i] = samples
    end

    # Generate simple data for the pulse channel (index 16)
    pulse_channel_index = CH_NO - 1
    data_hash[pulse_channel_index] = Array.new(SAMPLING_FREQUENCY) { |n| n % 100 } # Simple pulse pattern

    data_hash
  end

  def get_terminal_ad(ch_index)
    # Return an integer value like the VMonitor2Board does
    ((ch_index + 1) * 2.0).to_i
  end

  def get_gain(ch_index)
    @gains[ch_index - 1]
  end

  def set_gain(ch_index, gain)
    return -1 unless (ch_index >= 1) && (ch_index < CH_NO)

    puts "Channel:#{ch_index} Gain:#{gain}"
    @gains[ch_index - 1] = gain
  end

  def get_input(ch_index)
    @input_types[ch_index - 1]
  end

  def set_input(ch_index, input_type)
    return -1 unless (ch_index >= 1) && (ch_index < CH_NO)

    puts "Channel:#{ch_index} InputType:#{input_type}"
    @input_types[ch_index - 1] = input_type
  end

  def reset_device
    p 'Reset Device Called'
  end

  # バージョン取得
  def get_version
    # 8桁でhex表示
    format('%08x', 1000)
  end
end
