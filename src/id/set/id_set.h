#ifndef ID_SET_H
#define ID_SET_H
#include "../id.h"
/*
  This is for large sets of IDs that should only be referenced once

  This assumes that at least two IDs are from the same client (and thus, the
  same hash), and it just lists this down

  Order of the list needs to be preserved as well
 */

std::vector<id_t_> expand_id_set(std::vector<uint8_t> id_set, bool *order = nullptr);
std::vector<uint8_t> compact_id_set(std::vector<id_t_> id_set, bool order);

std::vector<uint8_t> add_id_to_set(std::vector<uint8_t> id_set, id_t_ id);
std::vector<uint8_t> del_id_from_set(std::vector<uint8_t> id_set, id_t_ id);

uint64_t size_of_id_set(std::vector<uint8_t> id_set);

#define ID_SET_SCHEME_UNDEFINED 0
// copy the entire ID vector
#define ID_SET_SCHEME_COPY 1
// list all UUIDs of a hash, all zero UUID, hash, repeat
#define ID_SET_SCHEME_UUID_LIST 2

#endif
