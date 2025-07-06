#include "tcp_sender.hh"
#include "debug.hh"
#include "tcp_config.hh"

using namespace std;

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // debug( "unimplemented sequence_numbers_in_flight() called" );
  // return {};
  return sequence_numbers_in_flight_;
}

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::consecutive_retransmissions() const
{
  // debug( "unimplemented consecutive_retransmissions() called" );
  // return {};
  return consecutive_retransmissions_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  // debug( "unimplemented push() called" );
  // (void)transmit;
  // if 0 send 1
  uint64_t window_size = last_window_size_ ? last_window_size_ : 1;
  while ( window_size > sequence_numbers_in_flight_ ) {
    // set msg
    TCPSenderMessage ms;
    // first msg sent
    if ( !syn_sent_ ) {
      ms.SYN = 1;
      syn_sent_ = 1;
    }
    size_t payload_size = min( TCPConfig::MAX_PAYLOAD_SIZE, window_size - sequence_numbers_in_flight_ - ms.SYN );
    auto payload_view = input_.reader().peek();
    payload_size = min( payload_size, payload_view.length() );
    ms.payload = std::string( payload_view.substr( 0, payload_size ) );
    input_.reader().pop( payload_size );
    if ( !fin_sent_ && input_.reader().is_finished() && sequence_numbers_in_flight_ + payload_size < window_size ) {
      ms.FIN = 1;
      fin_sent_ = 1;
    }
    ms.RST = input_.reader().has_error();
    if ( ms.sequence_length() == 0 ) {
      break;
    }

    // send msg
    ms.seqno = Wrap32::wrap( next_seqno_, isn_ );
    next_seqno_ += ms.sequence_length();
    sequence_numbers_in_flight_ += ms.sequence_length();
    if ( !timer_running_ ) {
      timer_running_ = true;
      timer_ = 0;
    }
    outstanding_segments_.push( ms );
    transmit( ms );
    if ( ms.FIN ) {
      break;
    }
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  // debug( "unimplemented make_empty_message() called" );
  // return {};
  TCPSenderMessage ms;
  ms.seqno = Wrap32::wrap( next_seqno_, isn_ );
  ms.RST = true;
  return ms;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // debug( "unimplemented receive() called" );
  // (void)msg;
  last_window_size_ = msg.window_size;
  if ( msg.RST ) {
    input_.writer().set_error();
    return;
  }
  auto ackno = msg.ackno->unwrap( isn_, next_seqno_ );
  if ( ackno > next_seqno_ ) {
    return;
  }
  // check if recevied by recevier
  while ( !outstanding_segments_.empty() ) {
    auto it = outstanding_segments_.front();
    auto seqno = it.seqno.unwrap( isn_, next_seqno_ );
    if ( seqno + it.sequence_length() <= ackno ) {
      sequence_numbers_in_flight_ -= it.sequence_length();
      outstanding_segments_.pop();
      timer_ = 0;
      current_RTO_ms_ = initial_RTO_ms_;
    } else {
      break;
    }
  }
  if ( outstanding_segments_.empty() ) {
    timer_ = 0;
    timer_running_ = 0;
  }
  consecutive_retransmissions_ = 0;
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // debug( "unimplemented tick({}, ...) called", ms_since_last_tick );
  // (void)transmit;
  if ( !timer_running_ || outstanding_segments_.empty() ) {
    return;
  }
  timer_ += ms_since_last_tick;
  if ( timer_ >= current_RTO_ms_ ) {
    transmit( outstanding_segments_.front() );
    consecutive_retransmissions_++;
    if ( last_window_size_ > 0 )
      current_RTO_ms_ *= 2;
    timer_ = 0;
  }
}
