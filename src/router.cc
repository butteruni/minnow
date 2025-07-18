#include "router.hh"
#include "debug.hh"

#include <iostream>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  // debug( "unimplemented add_route() called" );
  routing_table_.push_back({route_prefix, prefix_length, next_hop, interface_num});
}
void Router::route_one_datagram( InternetDatagram& dgram )
{
  const RouteRule* best_match_rule = nullptr;
  for ( const auto& rule : routing_table_ ) {
    uint32_t mask = ( rule.prefix_length == 0 ) ? 0 : ( 0xFFFFFFFF << ( 32 - rule.prefix_length ) );
    if ( ( dgram.header.dst & mask ) == rule.route_prefix ) {
      if ( best_match_rule == nullptr || rule.prefix_length > best_match_rule->prefix_length ) {
        best_match_rule = &rule;
      }
    }
  }

  if ( best_match_rule ) {
    if ( dgram.header.ttl <= 1 ) {
      return;
    }
    dgram.header.ttl--;
    Address next_hop = best_match_rule->next_hop.value_or( Address::from_ipv4_numeric( dgram.header.dst ) );
    interface( best_match_rule->interface_num )->send_datagram( dgram, next_hop );
  }
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  // debug( "unimplemented route() called" );
  for (auto &interface : interfaces_) {
    auto queue = interface->datagrams_received();
    while(queue.size()) {
      route_one_datagram(queue.front());
      queue.pop();
    }
  }
}
