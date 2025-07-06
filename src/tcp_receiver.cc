#include "tcp_receiver.hh"
#include "debug.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // abort connection if error
  if ( message.RST ) {
    reassembler_.reader().set_error();
    return;
  }
  // set ISN at first
  if ( message.SYN ) {
    if ( !isn_.has_value() ) {
      isn_ = message.seqno;
    }
  }

  // no connectino has been set
  if ( !isn_.has_value() ) {
    return;
  }

  // convet 32bit seqno into 64bit stream index
  uint64_t checkpoint = reassembler_.writer().bytes_pushed();
  uint64_t absolute_seqno = message.seqno.unwrap( isn_.value(), checkpoint );

  uint64_t stream_idx = message.SYN ? 0 : ( absolute_seqno > 0 ? absolute_seqno - 1 : 0 );

  if ( !message.SYN && absolute_seqno == 0 ) {
    // invalid
  } else {
    // insert to Reassembler
    reassembler_.insert( stream_idx, message.payload, message.FIN );
  }
}

TCPReceiverMessage TCPReceiver::send() const
{
  TCPReceiverMessage msg {};

  // windowsize = reassembler capacity
  size_t capacity = reassembler_.writer().available_capacity();
  msg.window_size = min( capacity, static_cast<size_t>( UINT16_MAX ) );
  msg.RST = reassembler_.writer().has_error();
  // only send ackno after connection has been set
  if ( isn_.has_value() ) {
    // absolute_seqno = stream_index + 1
    uint64_t next_abs_seqno = reassembler_.writer().bytes_pushed() + 1;
    // add FIN if end
    if ( reassembler_.writer().is_closed() ) {
      next_abs_seqno++;
    }
    msg.ackno = Wrap32::wrap( next_abs_seqno, isn_.value() );
  }

  return msg;
}
