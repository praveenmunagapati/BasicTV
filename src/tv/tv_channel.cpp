#include "../main.h"
#include "../util.h"
#include "tv_channel.h"
#include "tv_frame_standard.h"
#include "tv.h"

tv_channel_t::tv_channel_t() : id(this, TYPE_TV_CHANNEL_T){
	list_virtual_data(&id); // tv_meta_t
}

tv_channel_t::~tv_channel_t(){
}
