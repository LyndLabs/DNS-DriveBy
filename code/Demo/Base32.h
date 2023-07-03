/*
  Base32.h - Library for encoding to/decoding from Base32.
  Compatible with RFC 4648 ( http://tools.ietf.org/html/rfc4648 )
  Created by Vladimir Tarasow, December 18, 2012.
  Released into the public domain.
*/

#ifndef Base32_h
#define Base32_h

#include "Arduino.h"
#include "stdint.h"

class Base32
{
  public:
    Base32();
    int toBase32(byte*, long, byte*&);
    int toBase32(byte*, long, byte*&, boolean);
    int fromBase32(byte*, long, byte*&);
};

#endif
