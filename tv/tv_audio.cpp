#include "tv_frame_audio.h"
#include "tv_audio.h"
#include "tv_window.h"
#include "tv_channel.h"
#include "tv_item.h"
#include "tv.h"
#include "../convert.h"
#include "../settings.h"
#include "transcode/tv_transcode.h"
#include "../util.h"

#include <ao/ao.h>

#include "tv_audio_mixer.h"

// When I get a proper audio sink setup going, just torch this entire file

// libao > SDL_audio

static uint32_t output_sampling_rate = 0;
static uint8_t output_bit_depth = 0;
static uint8_t output_channel_count = 0;
static uint32_t output_chunk_size = 0;

static int32_t ao_default_driver = 0;
static ao_sample_format ao_format;
static ao_device *ao_device_ptr = nullptr;

tv_audio_prop_t::tv_audio_prop_t(){
}

tv_audio_prop_t::~tv_audio_prop_t(){
}

void tv_audio_prop_t::list_virtual_data(data_id_t *id){
	id->add_data_raw(&flags, sizeof(flags));
	id->add_data_raw(&format, sizeof(format));
	id->add_data_raw(&bit_depth, sizeof(bit_depth));
	id->add_data_raw(&sampling_freq, sizeof(sampling_freq));
	id->add_data_raw(&channel_count, sizeof(channel_count));

	id->add_data_raw(&snippet_duration_micro_s, sizeof(snippet_duration_micro_s));
	id->add_data_eight_byte_vector(&encoder_flags, ~0);
}

// static std::thread playing_thread;
// static std::mutex playing_mutex;
static std::vector<std::tuple<id_t_, id_t_, uint64_t, uint64_t, std::vector<uint8_t>, bool> > playing_now;
// static std::vector<std::tuple<id_t_, id_t_, uint64_t> > playing_now;


// static void tv_audio_play_wave_file(){
// 	while(true){
// 		const uint64_t timestamp =
// 			get_time_microseconds();
// 		playing_mutex.lock();
// 		for(uint64_t i = 0;i < playing_now.size();i++){
// 			if(unlikely(
// 				   BETWEEN(
// 					   std::get<2>(playing_now[i]),
// 					   timestamp,
// 					   std::get<3>(playing_now[i])))
// 			   && std::get<5>(playing_now[i]) == false){
// 				ao_play(ao_device_ptr,
// 					(char*)std::get<4>(playing_now[i]).data(),
// 					std::get<4>(playing_now[i]).size());
// 				std::get<5>(playing_now[i]) = true; // played
// 			}
// 		}
// 		playing_mutex.unlock();
// 	}
// }

void tv_audio_ao_init(){
	if(settings::get_setting("audio") == "true"){
		try{
			output_sampling_rate =
				std::stoi(
					settings::get_setting(
						"snd_output_sampling_rate"));
			output_bit_depth =
				std::stoi(
					settings::get_setting(
						"snd_output_bit_depth"));
			output_channel_count =
				std::stoi(
					settings::get_setting(
						"snd_output_channel_count"));
			output_chunk_size =
				std::stoi(
					settings::get_setting(
						"snd_output_chunk_size"));
		}catch(...){
			print("cannot read sound settings from file, setting default", P_WARN);
			output_sampling_rate = TV_AUDIO_DEFAULT_SAMPLING_RATE;
			output_bit_depth = TV_AUDIO_DEFAULT_BIT_DEPTH;
			output_channel_count = TV_AUDIO_DEFAULT_CHANNEL_COUNT;
			output_chunk_size = TV_AUDIO_DEFAULT_CHUNK_SIZE;
		}
		ao_initialize();
		ao_default_driver = ao_default_driver_id();
		CLEAR(ao_format);
		ao_format.bits = output_bit_depth;
		ao_format.channels = output_channel_count;
		ao_format.rate = output_sampling_rate;
		ao_format.byte_format = AO_FMT_NATIVE;
		ao_device_ptr =
			ao_open_live(
				ao_default_driver,
				&ao_format,
				nullptr);
		ASSERT(ao_device_ptr != nullptr, P_ERR);
		ASSERT(ao_format.bits == output_bit_depth, P_ERR);
		ASSERT(ao_format.channels == output_channel_count, P_ERR);
		ASSERT(ao_format.rate == output_sampling_rate, P_ERR);
		ASSERT(ao_format.byte_format = AO_FMT_NATIVE, P_ERR);
		// playing_thread =
		// 	std::move(
		// 		std::thread(tv_audio_play_wave_file));
	}else{
		print("audio has been disabled in the settings", P_NOTE);
	}
}

static void tv_audio_play_frame_audio(
        std::pair<id_t_, id_t_> audio_data){
	for(uint64_t i = 0;i < playing_now.size();i++){
		if(std::get<0>(playing_now[i]) == audio_data.first &&
		   std::get<1>(playing_now[i]) == audio_data.second){
			return;
		}
	}
	tv_window_t *window_ptr =
		PTR_DATA(audio_data.first,
			 tv_window_t);
	ASSERT(window_ptr != nullptr, P_ERR);
	tv_frame_audio_t *frame_audio_ptr =
		PTR_DATA(audio_data.second,
			 tv_frame_audio_t);
	ASSERT(frame_audio_ptr != nullptr, P_ERR);
	uint32_t sampling_freq = 0;
	uint8_t bit_depth = 0;
	uint8_t channel_count = 0;
	print("decoding " + convert::array::id::to_hex(audio_data.second), P_NOTE);
	std::vector<uint8_t> raw =
		transcode::audio::frames::to_raw(
			{audio_data.second},
			&sampling_freq,
			&bit_depth,
			&channel_count);
	P_V(sampling_freq, P_VAR);
	P_V(bit_depth, P_VAR);
	P_V(channel_count, P_VAR);
	// playing_mutex.lock();
	
	playing_now.push_back(
		std::make_tuple(
			audio_data.first,
			audio_data.second,
			frame_audio_ptr->get_start_time_micro_s() + window_ptr->get_timestamp_offset(),
			frame_audio_ptr->get_end_time_micro_s() + window_ptr->get_timestamp_offset(),
			raw,
			false));
	ao_play(ao_device_ptr,
		(char*)std::get<4>(playing_now[playing_now.size()-1]).data(),
		std::get<4>(playing_now[playing_now.size()-1]).size());
	// playing_mutex.unlock();
}

static std::vector<std::pair<id_t_, id_t_> > tv_audio_get_current_frame_audios(){
	std::vector<id_t_> windows =
		ID_TIER_CACHE_GET(
			TYPE_TV_WINDOW_T);
	const uint64_t cur_timestamp_micro_s =
		get_time_microseconds();
	std::vector<std::pair<id_t_, id_t_> > frame_audios;
	for(uint64_t i = 0;i < windows.size();i++){
		tv_window_t *window =
			PTR_DATA(windows[i],
				 tv_window_t);
		if(window == nullptr){
			print("window is a nullptr", P_WARN);
			continue;
		}
		const std::vector<id_t_> active_streams =
			window->get_active_streams();
		if(active_streams.size() == 0){
			print("no designated active streams", P_WARN);
		}
		const uint64_t play_time =
			cur_timestamp_micro_s+window->get_timestamp_offset();
		for(uint64_t c = 0;c < active_streams.size();c++){
			tv_frame_audio_t *audio_frame_tmp =
				PTR_DATA(active_streams[c],
					 tv_frame_audio_t);
			if(audio_frame_tmp == nullptr){
				print("supposed active stream is a nullptr", P_DEBUG);
				continue;
			}
			id_t_ curr_id =
				tv_frame_scroll_to_time(
					audio_frame_tmp,
					play_time);
			if(curr_id != ID_BLANK_ID){
				// don't set it to a non-live one
				window->set_active_streams(
					{curr_id}); // probably speed this up A LOT
			}
			if(curr_id != ID_BLANK_ID){
				frame_audios.push_back(
					std::make_pair(
						windows[i],
						curr_id));
			}else{
				print("curr_id is a nullptr, probably an invalid timestamp", P_SPAM);
			}
		}
	}
	return frame_audios;
}

void tv_audio_ao_loop(){
	if(settings::get_setting("audio") == "true"){
		std::vector<std::pair<id_t_, id_t_> > current_frame_audios =
			tv_audio_get_current_frame_audios();
		ASSERT(current_frame_audios.size() <= 1, P_ERR); // temporary
		for(uint64_t i = 0;i < current_frame_audios.size();i++){
			tv_audio_play_frame_audio(
				current_frame_audios[i]);
		}
	}
}

void tv_audio_ao_close(){
}

/*
  No defined or known outputs, let's the encoder and decoder fill in the gaps,
  useful for making tv_audio_prop_t for output functions.
 */

tv_audio_prop_t gen_format_only_audio_prop(uint8_t fmt){
	tv_audio_prop_t retval;
	retval.set_format(fmt);
	retval.set_flags(TV_AUDIO_PROP_FORMAT_ONLY);
	return retval;
}


void tv_audio_init(){
	if(settings::get_setting("audio") == "true"){
		if(settings::get_setting("audio_playback") == "sdl"){
			tv_audio_mixer_init();
		}else if(settings::get_setting("audio_playback") == "ao"){
			tv_audio_ao_init();
		}else{
			print("invalid audio_playback option", P_ERR);
		}
	}
}

void tv_audio_loop(){
	if(settings::get_setting("audio") == "true"){
		if(settings::get_setting("audio_playback") == "sdl"){
			tv_audio_mixer_loop();
		}else if(settings::get_setting("audio_playback") == "ao"){
			tv_audio_ao_loop();
		}else{
			print("invalid audio_playback option", P_ERR);
		}
	}
}

void tv_audio_close(){
	if(settings::get_setting("audio") == "true"){
		if(settings::get_setting("audio_playback") == "sdl"){
			tv_audio_mixer_close();
		}else if(settings::get_setting("audio_playback") == "ao"){
			tv_audio_ao_close();
		}else{
			print("invalid audio_playback option", P_ERR);
		}
	}
}

