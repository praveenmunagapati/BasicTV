#include "net_interface_ip_address.h"
#include "net_interface_api.h"

net_interface_ip_address_t::net_interface_ip_address_t() : id(this, TYPE_NET_INTERFACE_IP_ADDRESS_T){
	list_virtual_data(&id);
	id.add_data_one_byte_vector(&address, 1024, public_ruleset); // beyond sane
	ADD_DATA(address_type, public_ruleset);
	ADD_DATA(nat_type, public_ruleset);
	ADD_DATA(port, public_ruleset);
}

net_interface_ip_address_t::~net_interface_ip_address_t(){
}

void net_interface_ip_address_t::set_address_data(
	std::string address_,
	uint16_t port_,
	uint8_t nat_type_){
	std::pair<std::vector<uint8_t>, uint8_t> address_tmp =
		net_interface::ip::readable::to_raw(address_);
	address = address_tmp.first;
	address_type = address_tmp.second;
	port = port_;
	nat_type = nat_type_;
}

#pragma message("net_interface_ip_address_t::get_address_resolvable() isn't implemented")

std::pair<std::vector<uint8_t>, uint8_t> net_interface_ip_address_t::get_address(){
	return std::make_pair(
		address, address_type);
}
