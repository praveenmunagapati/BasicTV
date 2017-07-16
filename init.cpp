#include "main.h"
#include "init.h"

#include "encrypt/encrypt.h"
#include "tv/tv.h"
#include "input/input.h"
#include "net/proto/net_proto.h"
#include "console/console.h"
#include "id/id_api.h"
#include "settings.h"

#include "id/tier/memory/id_tier_memory_helper.h"
#include "id/tier/memory/id_tier_memory_special.h"

/*
  All information imported has the SHA256 hash of the public key. It follows
  the following rules:

  1. The first item to be created is the owner's encrypt_priv_key_t, this
  doesn't need a SHA256 hash (as it shouldn't be networked and doesn't add
  any security). 
  2. The second item to be created is the corresponding public key.
  3. All information created will have the encryption fingerprint included
  in the ID, as well as being encrypted with that information for transport
  (not relevant here)

  Anything can be loaded perfectly fine, but it cannot request the ID of the
  public key to link it to the private key. id_throw_exception is ONLY used
  to make this connection.

  SHA256 keys aren't needed for array lookups, and no transporting should be
  done to keep this property for long enough (entire code is only around five
  lines), so this isn't as hacky as I make myself believe it is.
*/


/*
  The production private key is the private key that is associated with every
  data type created on this machine. If the information were to be imported, the
  hash would be applied to the ID, but it would be overridden by the ID
  associated with the imported data (logical, and it doesn't break the hashing)

  I made a write up on why this is needed above. In case that was deleted, then
  a tl;dr is that the public key can't reference itself because the hash needs
  to be known for get_id(), and that needs to be ran to link the private and
  public keys together (production_priv_key_id).
*/

static void bootstrap_production_priv_key_id(){
	std::vector<id_t_> all_public_keys =
		id_tier::lookup::type::from_tier(
			std::vector<std::pair<uint8_t, uint8_t> > ({
					std::make_pair(ID_TIER_MAJOR_MEM, 0)}),
			TYPE_ENCRYPT_PUB_KEY_T);
	std::vector<id_t_> all_private_keys =
		id_tier::lookup::type::from_tier(
			std::vector<std::pair<uint8_t, uint8_t> > ({
					std::make_pair(ID_TIER_MAJOR_MEM, 0)}),
			TYPE_ENCRYPT_PUB_KEY_T);
	encrypt_priv_key_t *priv_key = nullptr;
	encrypt_pub_key_t *pub_key = nullptr;
	if(all_private_keys.size() == 0){
		print("detected first boot, creating production id", P_NOTE);
		uint64_t bits_to_use = 4096;
		try{
			bits_to_use =
				std::stoi(
					settings::get_setting(
						"rsa_key_length"));
		}catch(...){}
		std::pair<id_t_, id_t_> key_pair =
			rsa::gen_key_pair(bits_to_use);
		priv_key =
			PTR_DATA(key_pair.first,
				 encrypt_priv_key_t);
		if(priv_key == nullptr){
			print("priv_key is a nullptr", P_ERR);
		}
		pub_key =
			PTR_DATA(key_pair.second,
				 encrypt_pub_key_t);
		if(pub_key == nullptr){
			print("pub_key is a nullptr", P_ERR);
		}
		priv_key->set_pub_key_id(key_pair.second);
		production_priv_key_id = priv_key->id.get_id();
	}else if(all_private_keys.size() == 1){
		if(all_public_keys.size() == 0){
			print("no public keys can possibly match private key, error in loading?", P_CRIT);
		}
		production_priv_key_id = all_private_keys[0];
		P_V_S(convert::array::id::to_hex(all_private_keys[0]), P_VAR);
		priv_key = PTR_DATA(all_private_keys[0], encrypt_priv_key_t);
		P_V_S(convert::array::id::to_hex(all_public_keys[0]), P_VAR);
		pub_key = PTR_DATA(all_public_keys[0], encrypt_pub_key_t);
	}else if(all_private_keys.size() > 1){
		print("I have more than one private key, make a prompt to choose one", P_ERR);
	}
	const hash_t_ hash =
		encrypt_api::hash::sha256::gen_raw(
			pub_key->get_encrypt_key().second);
	std::vector<data_id_t*> id_vector =
		mem_helper::get_data_id_vector();
	for(uint64_t i = 0;i < id_vector.size();i++){
		id_t_ id_tmp =
			id_vector[i]->get_id();
		ASSERT(get_id_hash(id_tmp) == blank_hash, P_ERR);
		set_id_hash(&id_tmp, hash);
		id_vector[i]->set_id(id_tmp);
		P_V_S(id_breakdown(id_vector[i]->get_id()), P_NOTE);
	}
	production_priv_key_id =
		priv_key->id.get_id();
}

static id_tier_state_t* bootstrap_id_tier_mem(){
	id_tier_medium_t memory_medium_ptr =
		id_tier::get_medium(
			ID_TIER_MEDIUM_MEM);
	return	PTR_DATA(memory_medium_ptr.init_state(),
			 id_tier_state_t); // auto-updates
}

// don't need pointers to this since we can do all lookups, mem can't at the
// time of creation
static void bootstrap_id_tier_cache(){
	id_tier_medium_t cache_medium_ptr =
		id_tier::get_medium(
			ID_TIER_MEDIUM_CACHE);
	// can have a vector of std::pair<uint8_t> (allowed extra), but
	// the cache is set up so there's only one extra configuration per
	// minor tier
	std::vector<std::tuple<id_tier_state_t*, uint8_t, uint8_t, std::pair<uint8_t, uint8_t> > > cache_data =
		std::vector<std::tuple<id_tier_state_t*, uint8_t, uint8_t, std::pair<uint8_t, uint8_t> > >({
				std::make_tuple(
					PTR_DATA(cache_medium_ptr.init_state(), id_tier_state_t),
					0,
					ID_TIER_MEDIUM_CACHE,
					std::make_pair(
						ID_TIER_MAJOR_CACHE,
						ID_TIER_MINOR_CACHE_UNENCRYPTED_UNCOMPRESSED)),
				std::make_tuple(
					PTR_DATA(cache_medium_ptr.init_state(), id_tier_state_t),
					ID_EXTRA_COMPRESS,
					ID_TIER_MEDIUM_CACHE,
					std::make_pair(
						ID_TIER_MAJOR_CACHE,
						ID_TIER_MINOR_CACHE_UNENCRYPTED_COMPRESSED)),
				std::make_tuple(
					PTR_DATA(cache_medium_ptr.init_state(), id_tier_state_t),
					ID_EXTRA_ENCRYPT & ID_EXTRA_COMPRESS,
					ID_TIER_MEDIUM_CACHE,
					std::make_pair(
						ID_TIER_MAJOR_CACHE,
						ID_TIER_MINOR_CACHE_ENCRYPTED_COMPRESSED)),
					});
	for(uint64_t i = 0;i < cache_data.size();i++){
		std::get<0>(cache_data[i])->set_allowed_extra(
			std::vector<uint8_t>({std::get<1>(cache_data[i])}));
		std::get<0>(cache_data[i])->set_medium(
			std::get<2>(cache_data[i]));
		std::get<0>(cache_data[i])->set_tier_major(
			std::get<3>(cache_data[i]).first);
		std::get<0>(cache_data[i])->set_tier_minor(
			std::get<3>(cache_data[i]).second);
	}
}

#define SHORT_TO_FULL(short_, full) try{settings::set_setting(full, settings::get_setting(short_));}catch(...){}

void init(){
	// debugging information for OpenSSL's error printing
	ERR_load_crypto_strings();
	// loads OpenSSL stuff (AES only)
	OpenSSL_add_all_algorithms();
	OPENSSL_config(nullptr);
	/*
	  settings_init() only reads from the file, it doesn't do anything
	  critical to setting default values
	*/
	// // default port for ID networking
	// settings::set_setting("net_port", "58486");
	// settings::set_setting("net_hostname", "");
	// settings::set_setting("net_open_tcp_port", "false");
	// console port
	settings::set_setting("console_port", "59000");
	// disable socks
	settings::set_setting("socks_enable", "false");
	// if SOCKS cannot be set up properly, then terminate
	settings::set_setting("socks_strict", "true");
	// SOCKS proxy ip address in ASCII
	settings::set_setting("socks_proxy_ip", "127.0.0.1");
	// SOCKS proxy port in ASCII
	settings::set_setting("socks_proxy_port", "9050");
	// enable subsystems in settings
	settings::set_setting("video", "true");
	settings::set_setting("audio", "true");
	// throw level
	settings::set_setting("throw_level", std::to_string(P_CRIT));
	// shouldn't need to disable other stuff
	settings::set_setting("run_tests", "false");
	settings::set_setting("data_folder", ((std::string)getenv("HOME"))+"/.BasicTV/");
	settings::set_setting("print_backtrace", "false");
	settings::set_setting("print_color", "true");

	settings::set_setting("print_delay", "0");
		
	settings_init();
	
	// copy shortcut settings over to full names for in-program use
	// most calls still need console_port
	SHORT_TO_FULL("hostname", "net_interface_ip_hostname");
	SHORT_TO_FULL("conport", "console_port");
	SHORT_TO_FULL("port", "net_interface_ip_tcp_port");
	SHORT_TO_FULL("bsip", "net_proto_ip_tcp_bootstrap_ip");
	SHORT_TO_FULL("bsp", "net_proto_ip_tcp_bootstrap_port");
	SHORT_TO_FULL("df", "data_folder");
	
	
	/*
	  Using "~" doesn't work with C++, so get the information from getenv()

	  TODO: convert the input string from settings.cfg to this

	  TODO: use getuid and that stuff when getenv doesn't work (?)
	 */
	
	
	id_tier_state_t *tier_state_ptr =
		bootstrap_id_tier_mem();
	bootstrap_production_priv_key_id();
	id_tier_mem_regen_state_cache();
	id_tier_mem_update_state_cache(
		tier_state_ptr);
	bootstrap_id_tier_cache();

	
	// SDL2_net throws a SIGPIPE on client disconnects, I seriously need to
	// upgrade to something better
	signal(SIGPIPE, SIG_IGN);
	
	tv_init();
	input_init();
	net_proto_init();
	console_init();
	id_tier_init();
}

