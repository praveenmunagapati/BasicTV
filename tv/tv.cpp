#include "tv.h"
#include "tv_video.h"
#include "sink/tv_sink.h"

#include "tv_window.h"

#include "tv_frame_audio.h"
#include "tv_frame_video.h"
#include "tv_frame_standard.h"

#include "../util.h"
#include "../id/id_api.h"

void tv_init(){
	// auto-initializes the PortAudio system since this is the first
	// right now we pull this from ID_TIER_CACHE_GET
	tv::sink::state::init(
		TV_SINK_MEDIUM_AUDIO_HARDWARE);
}

#define TV_LOOP_FRAME_SEARCH_SEEK_SINK(format)\
	void tv_##format##_search_seek_sink(){	\
		std::vector<id_t_> window_vector =	\
		ID_API_CACHE_GET(			\
			TYPE_TV_WINDOW_T);		\
		for(uint64_t i  = 0;i < window_vector.size();i++	\
	}						\
	
void tv_loop(){
	std::vector<id_t_> window_vector =
		ID_TIER_CACHE_GET(
			TYPE_TV_WINDOW_T);
	for(uint64_t i = 0;i < window_vector.size();i++){
		tv_window_t *window_ptr =
			PTR_DATA(window_vector[i],
				 tv_window_t);	
		CONTINUE_IF_NULL(window_ptr, P_WARN);
		const int64_t window_offset =
			window_ptr->get_timestamp_offset_micro_s();
		const std::vector<
			std::tuple<
				id_t_,
				id_t_,
				std::vector<uint8_t> > > active_streams =
			window_ptr->get_active_streams();
		CONTINUE_IF_TRUE(active_streams.size() == 0);
		if(active_streams.size() > 1){
			print("more than one active stream at one time, weird...", P_WARN);
		}
		id_t_ latest_id = ID_BLANK_ID;
		switch(get_id_type(std::get<0>(active_streams[0]))){
		case TYPE_TV_FRAME_AUDIO_T:
			if(true){
				tv_frame_audio_t *frame_audio_ptr =
					PTR_DATA(std::get<0>(active_streams[0]),
						 tv_frame_audio_t);
				CONTINUE_IF_NULL(frame_audio_ptr, P_NOTE);
				latest_id =
					tv_frame_scroll_to_time(
						frame_audio_ptr,
						get_time_microseconds() + window_offset);
			}
			break;
		case TYPE_TV_FRAME_VIDEO_T:
			print("no formal video support exitts yet", P_WARN);
			continue;
			break;
		default:
			print("ID type given isn't a valid frame type", P_WARN);
			continue;
		}
		tv_sink_state_t *sink_state_ptr =
			PTR_DATA(std::get<1>(active_streams[0]),
				 tv_sink_state_t);
		ASSERT(sink_state_ptr->get_frame_type() ==
		       TV_FRAME_TYPE_AUDIO, P_ERR);
		CONTINUE_IF_NULL(sink_state_ptr, P_WARN);
		std::vector<id_t_> frames_to_load =
			id_api::linked_list::list::by_distance(
				latest_id,
				10); // matches default frame linked list
		
	}
}

void tv_close(){
}
