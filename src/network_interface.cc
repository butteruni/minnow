#include <iostream>
#include <queue>
#include <utility>
#include <unordered_map>
#include <algorithm>

#include "arp_message.hh"
#include "debug.hh"
#include "ethernet_frame.hh"
#include "exception.hh"
#include "helpers.hh"
#include "network_interface.hh"

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  // debug( "unimplemented send_datagram called" );
  // (void)dgram;
  // (void)next_hop;
  auto next_ip = next_hop.ipv4_numeric();

  auto arp_iter = arp_table_.find(next_ip);

  if
  (arp_iter == arp_table_.end()) {
    if (!pending_arp_requests_.count(next_ip)) {
      
      ARPMessage arp_request;
      arp_request.opcode = ARPMessage::OPCODE_REQUEST;
      arp_request.sender_ethernet_address = ethernet_address_;
      arp_request.sender_ip_address = ip_address_.ipv4_numeric();
      arp_request.target_ip_address = next_ip;

      EthernetFrame arp_frame;
      arp_frame.header.src = ethernet_address_;
      arp_frame.header.dst = ETHERNET_BROADCAST; 
      arp_frame.header.type = EthernetHeader::TYPE_ARP;
      
      arp_frame.payload = serialize(arp_request);
      transmit( arp_frame );
      pending_arp_requests_[next_ip] = arp_response_ttl_;
    }
    pending_datagrams_.push_back({dgram, next_hop});
  }else {
    EthernetFrame frame;
    frame.header.src = ethernet_address_;
    frame.header.dst = arp_iter->second.eth_addr;
    frame.header.type = EthernetHeader::TYPE_IPv4;
    frame.payload = serialize(dgram);
    transmit( frame );
  }

}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( EthernetFrame frame )
{
  // debug( "unimplemented recv_frame called" );
  // (void)frame;

  if(frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST) {
    return;
  }
  
  if(frame.header.type == EthernetHeader::TYPE_IPv4) {
    InternetDatagram dgram;
    if ( parse( dgram, frame.payload ) ) {
      datagrams_received_.push(dgram);
    }
  } else if(frame.header.type == EthernetHeader::TYPE_ARP) {
    ARPMessage arp_msg;
    if ( parse( arp_msg, frame.payload ) ) {
      arp_table_[arp_msg.sender_ip_address] = {arp_msg.sender_ethernet_address, arp_entry_ttl_};
      pending_arp_requests_.erase( arp_msg.sender_ip_address );
      bool arp_request = arp_msg.opcode == ARPMessage::OPCODE_REQUEST && 
          arp_msg.target_ip_address == ip_address_.ipv4_numeric();
      if (arp_request) {
        ARPMessage arp_reply;
        arp_reply.opcode = ARPMessage::OPCODE_REPLY;
        arp_reply.sender_ethernet_address = ethernet_address_;
        arp_reply.sender_ip_address = ip_address_.ipv4_numeric();
        arp_reply.target_ethernet_address = arp_msg.sender_ethernet_address;
        arp_reply.target_ip_address = arp_msg.sender_ip_address;
        
        EthernetFrame reply_frame;
        reply_frame.header.src = ethernet_address_;
        reply_frame.header.dst = arp_msg.sender_ethernet_address;
        reply_frame.header.type = EthernetHeader::TYPE_ARP;
        
        reply_frame.payload = serialize(arp_reply);
        transmit(reply_frame);
      } 
      // 
      for (auto iter = pending_datagrams_.begin(); iter != pending_datagrams_.end();) {
        if (iter->second.ipv4_numeric() == arp_msg.sender_ip_address) {
          send_datagram(iter->first, iter->second);
          iter = pending_datagrams_.erase(iter);
        } else
          ++iter;
      }
      
    }
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // debug( "unimplemented tick({}) called", ms_since_last_tick );

  for ( auto it = arp_table_.begin(); it != arp_table_.end(); ) {
    if ( it->second.ttl <= ms_since_last_tick) {
      it = arp_table_.erase( it );
    } else {
      it->second.ttl -= ms_since_last_tick;
      ++it;
    }
  }
  for (auto iter = pending_arp_requests_.begin(); iter != pending_arp_requests_.end();) {
    if(iter->second <= ms_since_last_tick) {
      // erase all related dgrams
      const uint32_t expired_ip = iter->first;
      pending_datagrams_.erase(
          std::remove_if(pending_datagrams_.begin(), pending_datagrams_.end(), 
                          [&](const auto& p) { return p.second.ipv4_numeric() == expired_ip; }),
          pending_datagrams_.end()
      );
      iter = pending_arp_requests_.erase(iter);
    }else {
      iter->second -= ms_since_last_tick;
      ++iter;
    }
  }
}
