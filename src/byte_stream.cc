#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ), buffer_( std::string() ) {}

void Writer::push( string data )
{
  // Your code here.
  if ( is_closed() ) {
    return;
  }
  uint64_t len_to_push = min( data.size(), available_capacity() );
  buffer_.append( data.substr( 0, len_to_push ) );
  total_bytes_pushed_ += len_to_push;
}

void Writer::close()
{
  // Your code here.
  is_closed_ = true;
}

bool Writer::is_closed() const
{
  return is_closed_; // Your code here.
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - buffer_.size(); // Your code here.
}

uint64_t Writer::bytes_pushed() const
{
  return total_bytes_pushed_; // Your code here.
}

string_view Reader::peek() const
{
  return buffer_; // Your code here.
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  const uint64_t len_to_pop = min( len, buffer_.size() );
  buffer_.erase( 0, len_to_pop );
  total_bytes_popped_ += len_to_pop;
}

bool Reader::is_finished() const
{
  return is_closed_ && buffer_.empty(); // Your code here.
}

uint64_t Reader::bytes_buffered() const
{
  return buffer_.size(); // Your code here.
}

uint64_t Reader::bytes_popped() const
{
  return total_bytes_popped_; // Your code here.
}
