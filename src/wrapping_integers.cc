#include "wrapping_integers.hh"
#include "debug.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  // debug( "unimplemented wrap( {}, {} ) called", n, zero_point.raw_value_ );
  // return Wrap32 { 0 };
  return Wrap32( ( zero_point.raw_value_ + static_cast<uint32_t>( n ) ) % ( 1ul << 32 ) );
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  // debug( "unimplemented unwrap( {}, {} ) called", zero_point.raw_value_, checkpoint );
  // return {};
  const uint32_t offset = this->raw_value_ - zero_point.raw_value_;
  uint64_t base = ( checkpoint & 0xFFFFFFFF00000000 ) | offset;
  if ( base > checkpoint && base - checkpoint > ( 1UL << 31 ) ) {
    if ( base >= ( 1UL << 32 ) ) {
      base -= ( 1UL << 32 );
    }
  } else if ( checkpoint > base && checkpoint - base > ( 1UL << 31 ) ) {
    base += ( 1UL << 32 );
  }
  return base;
}
