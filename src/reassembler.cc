#include "reassembler.hh"
#include "debug.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // debug( "unimplemented insert({}, {}, {}) called", first_index, data, is_last_substring );

  // if is EOF
  if ( is_last_substring ) {
    eof_flag_ = true;
    eof_index_ = first_index + data.size();
  }
  const uint64_t capacity = output_.writer().available_capacity();
  const uint64_t window_limit = next_index_ + capacity;

  // the whole part has been received
  if ( first_index + data.size() <= next_index_ or first_index >= window_limit ) {
    // empty data
    if ( eof_flag_ && count_bytes_pending() == 0 && next_index_ == eof_index_ ) {
      output_.writer().close();
    }
    return;
  }

  // the part has been received
  if ( first_index < next_index_ ) {
    data = data.substr( next_index_ - first_index );
    first_index = next_index_;
  }

  // the part out of capacity
  if ( first_index + data.size() > window_limit ) {
    data.resize( window_limit - first_index );
  }

  if ( data.empty() ) {
    return;
  }

  // insert into map
  uint64_t current_end = first_index + data.size();

  // try to merge
  auto it = unassembled_strs_.lower_bound( first_index );

  // merge to left side
  if ( it != unassembled_strs_.begin() ) {
    auto prev_it = prev( it );
    if ( prev_it->first + prev_it->second.size() >= first_index ) {
      // totally overlaped by left part
      if ( prev_it->first + prev_it->second.size() >= current_end ) {
        return;
      }
      // drop overlap part
      data = data.substr( prev_it->first + prev_it->second.size() - first_index );
      first_index = prev_it->first + prev_it->second.size();
    }
  }

  // merge to right side one by one
  while ( it != unassembled_strs_.end() and it->first <= current_end ) {
    // old one totally overlaped by new one
    if ( it->first + it->second.size() <= current_end ) {
      unassembled_bytes_ -= it->second.size();
      it = unassembled_strs_.erase( it );
    } else { // merge
      data += it->second.substr( current_end - it->first );
      unassembled_bytes_ -= it->second.size();
      it = unassembled_strs_.erase( it );
      break;
    }
  }

  unassembled_strs_[first_index] = data;
  unassembled_bytes_ += data.size();

  // write to byteStream
  auto it_push = unassembled_strs_.find( next_index_ );
  while ( it_push != unassembled_strs_.end() ) {
    output_.writer().push( it_push->second );
    next_index_ += it_push->second.size();
    unassembled_bytes_ -= it_push->second.size();
    unassembled_strs_.erase( it_push );
    it_push = unassembled_strs_.find( next_index_ );
  }

  if ( eof_flag_ && count_bytes_pending() == 0 && next_index_ == eof_index_ ) {
    output_.writer().close();
  }
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  // debug( "unimplemented count_bytes_pending() called" );
  // return {};
  return unassembled_bytes_;
}
