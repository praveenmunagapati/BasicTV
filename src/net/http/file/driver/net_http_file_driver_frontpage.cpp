#include "net_http_file_driver_frontpage.h"
#include "net_http_file_driver_frontpage_forms.h"
#include "net_http_file_driver_frontpage_get_logic.h"

#include "../../parse/net_http_parse.h" // standard_header

/*
  Frontpage is where a lot of the control happens
 */

#define TEMPLATE_INSERT_FUNCTION(x) "<p>" #x "</p>"

const std::string frontpage_template =
	"<html><head><title>BasicTV</title></head><body><p>PREPEND</p>"
	TEMPLATE_INSERT_FUNCTION(CREATE_WALLET_SET)
	TEMPLATE_INSERT_FUNCTION(ADD_WALLET_TO_SET)
	TEMPLATE_INSERT_FUNCTION(CREATE_TV_CHANNEL)
	TEMPLATE_INSERT_FUNCTION(CREATE_TV_ITEM_PLAIN)
	TEMPLATE_INSERT_FUNCTION(CREATE_TV_ITEM_UPLOAD)
	TEMPLATE_INSERT_FUNCTION(BIND_TV_SINK_ITEM)
	TEMPLATE_INSERT_FUNCTION(UNBIND_TV_SINK_ITEM)
	TEMPLATE_INSERT_FUNCTION(BIND_TV_SINK_TO_ITEM)
	TEMPLATE_INSERT_FUNCTION(CONTROL_WINDOW)
	"</body></html>";

NET_HTTP_FILE_DRIVER_MEDIUM_INIT(frontpage){
	STD_STATE_INIT(
		net_http_file_driver_state_t,
		file_driver_state_ptr,
		net_http_file_driver_frontpage_state_t,
		frontpage_state_ptr);
	file_driver_state_ptr->set_socket_id(
		socket_id);
	file_driver_state_ptr->set_medium(
		NET_HTTP_FILE_DRIVER_MEDIUM_FRONTPAGE);

	file_driver_state_ptr->request_payload =
		request_payload;
	file_driver_state_ptr->response_payload.set_direction(
		NET_HTTP_PAYLOAD_OUT);
	return file_driver_state_ptr;
}

NET_HTTP_FILE_DRIVER_MEDIUM_CLOSE(frontpage){
	STD_STATE_CLOSE(
		file_driver_state_ptr,
		net_http_file_driver_frontpage_state_t);
}

#define REPLACE_WITH_STR(temp, temp_str, string_)			\
	if(true){							\
		uint64_t pos = temp.find(temp_str);			\
		if(pos != temp.size()){					\
			temp.erase(pos, strlen(temp_str));		\
			const std::string tmp_retval = string_;		\
			temp.insert(pos, tmp_retval.data(), tmp_retval.size());	\
		}							\
	}								\
	
static std::string net_http_file_driver_frontpage_generate(
	std::string prepend){
	std::string retval;
	retval = frontpage_template;
	REPLACE_WITH_STR(
		retval,
		"PREPEND",
		prepend);
	REPLACE_WITH_STR(
		retval,
		"CREATE_WALLET_SET",
		net_http_file_driver_frontpage_form_create_wallet_set());
	REPLACE_WITH_STR(
		retval,
		"ADD_WALLET_TO_SET",
		net_http_file_driver_frontpage_form_add_wallet_to_set());
	REPLACE_WITH_STR(
		retval,
		"CREATE_TV_CHANNEL",
		net_http_file_driver_frontpage_form_create_tv_channel());
	REPLACE_WITH_STR(
		retval,
		"CREATE_TV_ITEM_PLAIN",
		net_http_file_driver_frontpage_form_create_tv_item_plain());
	REPLACE_WITH_STR(
		retval,
		"CREATE_TV_ITEM_UPLOAD",
		net_http_file_driver_frontpage_form_create_tv_item_upload());
	REPLACE_WITH_STR(
		retval,
		"BIND_TV_SINK_ITEM",
		net_http_file_driver_frontpage_form_bind_tv_sink_item_window());
	REPLACE_WITH_STR(
		retval,
		"UNBIND_TV_SINK_ITEM",
		net_http_file_driver_frontpage_form_unbind_tv_sink_item_window());
	REPLACE_WITH_STR(
		retval,
		"CONTROL_WINDOW",
		net_http_file_driver_frontpage_form_control_window());
	return retval;
}

NET_HTTP_FILE_DRIVER_MEDIUM_LOOP(frontpage){
	// everything here is generated OTF and returned in full, so
	ASSERT(file_driver_state_ptr->response_payload.get_size_chunks() == 0, P_WARN);

	// TODO: should use this in the caller
	if(file_driver_state_ptr->response_payload.get_finished() == false){
		net_http_chunk_t payload_chunk;
		payload_chunk.set_header(
			standard_header);
		payload_chunk.set_payload(
			convert::string::to_bytes(
				net_http_file_driver_frontpage_generate(
					net_http_file_driver_frontpage_get_logic(
						file_driver_state_ptr))));
		file_driver_state_ptr->response_payload.add_chunks(
			payload_chunk);
		file_driver_state_ptr->response_payload.set_finished(
			true);
	}
}
