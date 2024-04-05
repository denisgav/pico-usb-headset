#include "pico/volume_ctrl.h"

uint16_t vol_to_db_convert(bool channel_mute, uint16_t channel_volume){
  if(channel_mute)
    	return 0;

    // todo interpolate
  channel_volume += CENTER_VOLUME_INDEX * 256;
  if (channel_volume < 0) channel_volume = 0;
  if (channel_volume >= count_of(db_to_vol) * 256) channel_volume = count_of(db_to_vol) * 256 - 1;
  uint16_t vol_mul = db_to_vol[((uint16_t)channel_volume) >> 8u];

  return vol_mul;
}