#include "net_http.h"
#include "net_http_file_driver.h"
#include "net_http_file_driver_atom.h"

#include "../../state.h"

/*
  Atom shouldn't worry about populating the channel_vector right now, but it
  should sometime in the future.

  The BasicTV Atom feed allows for downloading different bitrate audio/video
  files from the node, which don't inherently have to exist at those bitrates,
  but can be generated on the fly
 */

static std::string atom_get_prefix(){
	return "<?xml version=\"1.0\" encoding=\"utf-8\"?>";
}

static std::string atom_wrap_to_feed(std::string data){
	return "<feed xmlns=\"http://www.w3.org/2005/Atom\">" + data + "</feed>";
}

static std::string atom_wrap_title(std::string data){
	return "<title>" + data + "</title>";
}

NET_HTTP_FILE_DRIVER_MEDIUM_INIT(atom){	
	STD_STATE_INIT(
		net_http_file_driver_state_t,
		file_driver_state_ptr,
		net_http_file_driver_atom_state_t,
		atom_state_ptr);
	return file_driver_state_ptr;
}

NET_HTTP_FILE_DRIVER_MEDIUM_CLOSE(atom){
	STD_STATE_CLOSE(
		file_driver_state_ptr,
		net_http_file_driver_atom_state_t);
}

NET_HTTP_FILE_DRIVER_MEDIUM_PULL(atom){
	STD_STATE_GET_PTR(
		file_driver_state_ptr,
		net_http_file_driver_atom_state_t,
		atom_state_ptr);
	std::vector<uint8_t> retval;
	return retval;
}
