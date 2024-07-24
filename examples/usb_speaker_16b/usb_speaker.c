#include "usb_speaker.h"
#include "common_types.h"
#include "pico/volume_ctrl.h"

// List of supported sample rates
#if defined(__RX__)
  const uint32_t sample_rates[] = {44100, 48000};
#else
  const uint32_t sample_rates[] = {44100, 48000, 88200, 96000};
#endif

uint32_t current_sample_rate  = 48000;

#define N_SAMPLE_RATES  TU_ARRAY_SIZE(sample_rates)

// Audio controls
// Current states
int8_t mute[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX + 1];       // +1 for master channel 0
int16_t volume[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX + 1];    // +1 for master channel 0

// Current resolution, update on format change
uint8_t current_resolution = CFG_TUD_AUDIO_FUNC_1_RESOLUTION_RX;

void audio_task(void);

void usb_speaker_init()
{
	board_init();

  // init device stack on configured roothub port
  tud_init(BOARD_TUD_RHPORT);

  if (board_init_after_tusb) {
    board_init_after_tusb();
  }

  for(int ch_idx = 0; ch_idx < CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX + 1; ch_idx++){
    mute[ch_idx] = 0;
    volume[ch_idx] = MAX_VOLUME;
  }

  TU_LOG1("Headset running\r\n");
}

void usb_speaker_task()
{
	tud_task(); // TinyUSB device task
  audio_task();
}

static usb_speaker_mute_set_cb_t usb_speaker_mute_set_handler = NULL;
static usb_speaker_volume_set_cb_t usb_speaker_volume_set_handler = NULL;
static usb_speaker_current_sample_rate_set_cb_t usb_speaker_current_sample_rate_set_handler = NULL;
static usb_speaker_current_resolution_set_cb_t usb_speaker_current_resolution_set_handler = NULL;
static usb_speaker_current_status_set_cb_t usb_speaker_current_status_set_handler = NULL;
static usb_speaker_tud_audio_rx_done_pre_read_cb_t usb_speaker_tud_audio_rx_done_pre_read_handler = NULL;

void usb_speaker_set_mute_set_handler(usb_speaker_mute_set_cb_t handler){
	usb_speaker_mute_set_handler = handler;
}
void usb_speaker_set_volume_set_handler(usb_speaker_volume_set_cb_t handler){
	usb_speaker_volume_set_handler = handler;
}

void usb_speaker_set_current_sample_rate_set_handler(usb_speaker_current_sample_rate_set_cb_t handler){
	usb_speaker_current_sample_rate_set_handler = handler;
}

void usb_speaker_set_current_resolution_set_handler(usb_speaker_current_resolution_set_cb_t handler){
	usb_speaker_current_resolution_set_handler = handler;
}

void usb_speaker_set_current_status_set_handler(usb_speaker_current_status_set_cb_t handler){
  usb_speaker_current_status_set_handler = handler;
}

void usb_speaker_set_tud_audio_rx_done_pre_read_set_handler(usb_speaker_tud_audio_rx_done_pre_read_cb_t handler){
	usb_speaker_tud_audio_rx_done_pre_read_handler = handler;
}


//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
  if(usb_speaker_current_status_set_handler){
    usb_speaker_current_status_set_handler(BLINK_MOUNTED);
  }
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
  if(usb_speaker_current_status_set_handler){
    usb_speaker_current_status_set_handler(BLINK_NOT_MOUNTED);
  }
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void)remote_wakeup_en;
  if(usb_speaker_current_status_set_handler){
    usb_speaker_current_status_set_handler(BLINK_SUSPENDED);
  }
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
  if(usb_speaker_current_status_set_handler){
    usb_speaker_current_status_set_handler(tud_mounted() ? BLINK_MOUNTED : BLINK_NOT_MOUNTED);
  }
}

// Helper for clock get requests
static bool tud_audio_clock_get_request(uint8_t rhport, audio_control_request_t const *request)
{
  TU_ASSERT(request->bEntityID == UAC2_ENTITY_CLOCK);

  if (request->bControlSelector == AUDIO_CS_CTRL_SAM_FREQ)
  {
    if (request->bRequest == AUDIO_CS_REQ_CUR)
    {
      TU_LOG1("Clock get current freq %lu\r\n", current_sample_rate);

      audio_control_cur_4_t curf = { (int32_t) tu_htole32(current_sample_rate) };
      return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &curf, sizeof(curf));
    }
    else if (request->bRequest == AUDIO_CS_REQ_RANGE)
    {
      audio_control_range_4_n_t(N_SAMPLE_RATES) rangef =
      {
        .wNumSubRanges = tu_htole16(N_SAMPLE_RATES)
      };
      TU_LOG1("Clock get %d freq ranges\r\n", N_SAMPLE_RATES);
      for(uint8_t i = 0; i < N_SAMPLE_RATES; i++)
      {
        rangef.subrange[i].bMin = (int32_t) sample_rates[i];
        rangef.subrange[i].bMax = (int32_t) sample_rates[i];
        rangef.subrange[i].bRes = 0;
        TU_LOG1("Range %d (%d, %d, %d)\r\n", i, (int)rangef.subrange[i].bMin, (int)rangef.subrange[i].bMax, (int)rangef.subrange[i].bRes);
      }

      return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &rangef, sizeof(rangef));
    }
  }
  else if (request->bControlSelector == AUDIO_CS_CTRL_CLK_VALID &&
           request->bRequest == AUDIO_CS_REQ_CUR)
  {
    audio_control_cur_1_t cur_valid = { .bCur = 1 };
    TU_LOG1("Clock get is valid %u\r\n", cur_valid.bCur);
    return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &cur_valid, sizeof(cur_valid));
  }
  TU_LOG1("Clock get request not supported, entity = %u, selector = %u, request = %u\r\n",
          request->bEntityID, request->bControlSelector, request->bRequest);
  return false;
}

// Helper for clock set requests
static bool tud_audio_clock_set_request(uint8_t rhport, audio_control_request_t const *request, uint8_t const *buf)
{
  (void)rhport;

  TU_ASSERT(request->bEntityID == UAC2_ENTITY_CLOCK);
  TU_VERIFY(request->bRequest == AUDIO_CS_REQ_CUR);

  if (request->bControlSelector == AUDIO_CS_CTRL_SAM_FREQ)
  {
    TU_VERIFY(request->wLength == sizeof(audio_control_cur_4_t));

    current_sample_rate = (uint32_t) ((audio_control_cur_4_t const *)buf)->bCur;

    if(usb_speaker_current_sample_rate_set_handler)
    {
    	usb_speaker_current_sample_rate_set_handler(current_sample_rate);
    }

    TU_LOG1("Clock set current freq: %ld\r\n", current_sample_rate);

    return true;
  }
  else
  {
    TU_LOG1("Clock set request not supported, entity = %u, selector = %u, request = %u\r\n",
            request->bEntityID, request->bControlSelector, request->bRequest);
    return false;
  }
}

// Helper for feature unit get requests
static bool tud_audio_feature_unit_get_request(uint8_t rhport, audio_control_request_t const *request)
{
  TU_ASSERT(request->bEntityID == UAC2_ENTITY_FEATURE_UNIT);

  if (request->bControlSelector == AUDIO_FU_CTRL_MUTE && request->bRequest == AUDIO_CS_REQ_CUR)
  {
    audio_control_cur_1_t mute1 = { .bCur = mute[request->bChannelNumber] };
    TU_LOG1("Get channel %u mute %d\r\n", request->bChannelNumber, mute1.bCur);
    return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &mute1, sizeof(mute1));
  }
  else if (request->bControlSelector == AUDIO_FU_CTRL_VOLUME)
  {
    if (request->bRequest == AUDIO_CS_REQ_RANGE)
    {
      audio_control_range_2_n_t(1) range_vol = {
        .wNumSubRanges = tu_htole16(1),
        .subrange[0] = { 
          .bMin = tu_htole16(MIN_VOLUME), 
          .bMax = tu_htole16(MAX_VOLUME), 
          .bRes = tu_htole16(VOLUME_RESOLUTION) }
      };
      TU_LOG1("Get channel %u volume range (%d, %d, %u) dB\r\n", request->bChannelNumber,
              range_vol.subrange[0].bMin / 256, range_vol.subrange[0].bMax / 256, range_vol.subrange[0].bRes / 256);
      return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &range_vol, sizeof(range_vol));
    }
    else if (request->bRequest == AUDIO_CS_REQ_CUR)
    {
      audio_control_cur_2_t cur_vol = { .bCur = tu_htole16(volume[request->bChannelNumber]) };
      TU_LOG1("Get channel %u volume %d dB\r\n", request->bChannelNumber, cur_vol.bCur / 256);
      return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &cur_vol, sizeof(cur_vol));
    }
  }
  TU_LOG1("Feature unit get request not supported, entity = %u, selector = %u, request = %u\r\n",
          request->bEntityID, request->bControlSelector, request->bRequest);

  return false;
}

// Helper for feature unit set requests
static bool tud_audio_feature_unit_set_request(uint8_t rhport, audio_control_request_t const *request, uint8_t const *buf)
{
  (void)rhport;

  TU_ASSERT(request->bEntityID == UAC2_ENTITY_FEATURE_UNIT);
  TU_VERIFY(request->bRequest == AUDIO_CS_REQ_CUR);

  if (request->bControlSelector == AUDIO_FU_CTRL_MUTE)
  {
    TU_VERIFY(request->wLength == sizeof(audio_control_cur_1_t));

    mute[request->bChannelNumber] = ((audio_control_cur_1_t const *)buf)->bCur;

    if(usb_speaker_mute_set_handler)
    {
    	usb_speaker_mute_set_handler(request->bChannelNumber, mute[request->bChannelNumber]);
    }

    TU_LOG1("Set channel %d Mute: %d\r\n", request->bChannelNumber, mute[request->bChannelNumber]);

    return true;
  }
  else if (request->bControlSelector == AUDIO_FU_CTRL_VOLUME)
  {
    TU_VERIFY(request->wLength == sizeof(audio_control_cur_2_t));

    volume[request->bChannelNumber] = ((audio_control_cur_2_t const *)buf)->bCur;

    if(usb_speaker_volume_set_handler)
    {
    	usb_speaker_volume_set_handler(request->bChannelNumber, volume[request->bChannelNumber]);
    }

    TU_LOG1("Set channel %d volume: %d dB\r\n", request->bChannelNumber, volume[request->bChannelNumber] / 256);

    return true;
  }
  else
  {
    TU_LOG1("Feature unit set request not supported, entity = %u, selector = %u, request = %u\r\n",
            request->bEntityID, request->bControlSelector, request->bRequest);
    return false;
  }
}

//--------------------------------------------------------------------+
// Application Callback API Implementations
//--------------------------------------------------------------------+

// Invoked when audio class specific get request received for an entity
bool tud_audio_get_req_entity_cb(uint8_t rhport, tusb_control_request_t const *p_request)
{
  audio_control_request_t const *request = (audio_control_request_t const *)p_request;

  if (request->bEntityID == UAC2_ENTITY_CLOCK)
    return tud_audio_clock_get_request(rhport, request);
  if (request->bEntityID == UAC2_ENTITY_FEATURE_UNIT)
    return tud_audio_feature_unit_get_request(rhport, request);
  else
  {
    TU_LOG1("Get request not handled, entity = %d, selector = %d, request = %d\r\n",
            request->bEntityID, request->bControlSelector, request->bRequest);
  }
  return false;
}

// Invoked when audio class specific set request received for an entity
bool tud_audio_set_req_entity_cb(uint8_t rhport, tusb_control_request_t const *p_request, uint8_t *buf)
{
  audio_control_request_t const *request = (audio_control_request_t const *)p_request;

  if (request->bEntityID == UAC2_ENTITY_FEATURE_UNIT)
    return tud_audio_feature_unit_set_request(rhport, request, buf);
  if (request->bEntityID == UAC2_ENTITY_CLOCK)
    return tud_audio_clock_set_request(rhport, request, buf);
  TU_LOG1("Set request not handled, entity = %d, selector = %d, request = %d\r\n",
          request->bEntityID, request->bControlSelector, request->bRequest);

  return false;
}

bool tud_audio_set_itf_close_EP_cb(uint8_t rhport, tusb_control_request_t const * p_request)
{
  (void)rhport;

  uint8_t const itf = tu_u16_low(tu_le16toh(p_request->wIndex));
  uint8_t const alt = tu_u16_low(tu_le16toh(p_request->wValue));

  if (ITF_NUM_AUDIO_STREAMING == itf && alt == 0)
  {
    if(usb_speaker_current_status_set_handler){
      usb_speaker_current_status_set_handler(BLINK_MOUNTED);
    }
  }

  return true;
}

bool tud_audio_set_itf_cb(uint8_t rhport, tusb_control_request_t const * p_request)
{
  (void)rhport;
  uint8_t const itf = tu_u16_low(tu_le16toh(p_request->wIndex));
  uint8_t const alt = tu_u16_low(tu_le16toh(p_request->wValue));

  TU_LOG2("Set interface %d alt %d\r\n", itf, alt);
  if (ITF_NUM_AUDIO_STREAMING == itf && alt != 0)
  {
    if(usb_speaker_current_status_set_handler){
      usb_speaker_current_status_set_handler(BLINK_STREAMING);
    }
  }

  // Clear buffer when streaming format is changed
  //spk_data_size = 0;
  if(alt != 0)
  {
    // current_resolution = resolutions_per_format[alt-1];
    // if(usb_speaker_current_resolution_set_handler)
    // {
    // 	usb_speaker_current_resolution_set_handler(current_resolution);
    // }
  }

  return true;
}

// void tud_audio_feedback_params_cb(uint8_t func_id, uint8_t alt_itf, audio_feedback_params_t* feedback_param)
// {

//   (void)func_id;
//   (void)alt_itf;
//   // Set feedback method to fifo counting
//   feedback_param->method = AUDIO_FEEDBACK_METHOD_FIFO_COUNT;
//   feedback_param->sample_freq = current_sample_rate;
// }

bool tud_audio_rx_done_pre_read_cb(uint8_t rhport, uint16_t n_bytes_received, uint8_t func_id, uint8_t ep_out, uint8_t cur_alt_setting)
{
  (void)rhport;
  (void)func_id;
  (void)ep_out;
  (void)cur_alt_setting;

  if(usb_speaker_tud_audio_rx_done_pre_read_handler)
  {
  	usb_speaker_tud_audio_rx_done_pre_read_handler(rhport, n_bytes_received, func_id, ep_out, cur_alt_setting);
  }  

  return true;
}

// Invoked when received GET_REPORT control request
// Unused here
// uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
// {
//   // TODO not Implemented
//   (void) itf;
//   (void) report_id;
//   (void) report_type;
//   (void) buffer;
//   (void) reqlen;

//   return 0;
// }

// Invoked when received SET_REPORT control request or
// Unused here
// void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
// {
//   // This example doesn't use multiple report and report ID
//   (void) itf;
//   (void) report_id;
//   (void) report_type;
//   (void) buffer;
//   (void) bufsize;
// }

//--------------------------------------------------------------------+
// AUDIO Task
//--------------------------------------------------------------------+

void audio_task(void)
{
}
