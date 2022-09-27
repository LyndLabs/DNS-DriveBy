/*
  Base32.cpp - Library for encoding to/decoding from Base32.
  Compatible with RFC 4648 ( http://tools.ietf.org/html/rfc4648 )
  Created by Vladimir Tarasow, December 18, 2012.
  Released into the public domain.
*/
#include "Arduino.h"
#include "Base32.h"
#include "stdint.h"

Base32::Base32() 
{
}

int Base32::toBase32(byte* in, long length, byte*& out)
{
  return toBase32(in, length, out, false);
}

int Base32::toBase32(byte* in, long length, byte*& out, boolean usePadding)
{
  char base32StandardAlphabet[] = {"ABCDEFGHIJKLMNOPQRSTUVWXYZ234567"};
  char standardPaddingChar = '='; 

  int result = 0;
  int count = 0;
  int bufSize = 8;
  int index = 0;
  int size = 0; // size of temporary array
  byte* temp = NULL;

  if (length < 0 || length > 268435456LL) 
  { 
    return 0;
  }

  size = 8 * ceil(length / 4.0); // Calculating size of temporary array. Not very precise.
  temp = (byte*)malloc(size); // Allocating temporary array.

  if (length > 0)
  {
    int buffer = in[0];
    int next = 1;
    int bitsLeft = 8;
    
    while (count < bufSize && (bitsLeft > 0 || next < length))
    {
      if (bitsLeft < 5)
      {
        if (next < length)
        {
          buffer <<= 8;
          buffer |= in[next] & 0xFF;
          next++;
          bitsLeft += 8;
        }
        else
        {
          int pad = 5 - bitsLeft;
          buffer <<= pad;
          bitsLeft += pad;
        }
      }
      index = 0x1F & (buffer >> (bitsLeft -5));

      bitsLeft -= 5;
      temp[result] = (byte)base32StandardAlphabet[index];
      result++;
    }
  }
  
  if (usePadding)
  {
    int pads = (result % 8);
    if (pads > 0)
    {
      pads = (8 - pads);
      for (int i = 0; i < pads; i++) 
      {
        temp[result] = standardPaddingChar;
        result++;
      }
    }
  }

  out = (byte*)malloc(result);
  memcpy(out, temp, result);
  free(temp);
  
  return result;
}

int Base32::fromBase32(byte* in, long length, byte*& out)
{
  int result = 0; // Length of the array of decoded values.
  int buffer = 0;
  int bitsLeft = 0;
  byte* temp = NULL;

  temp = (byte*)malloc(length); // Allocating temporary array.

  for (int i = 0; i < length; i++)
  {
    byte ch = in[i];

    // ignoring some characters: ' ', '\t', '\r', '\n', '='
    if (ch == 0xA0 || ch == 0x09 || ch == 0x0A || ch == 0x0D || ch == 0x3D) continue;

    // recovering mistyped: '0' -> 'O', '1' -> 'L', '8' -> 'B'
    if (ch == 0x30) { ch = 0x4F; } else if (ch == 0x31) { ch = 0x4C; } else if (ch == 0x38) { ch = 0x42; }
    

    // look up one base32 symbols: from 'A' to 'Z' or from 'a' to 'z' or from '2' to '7'
    if ((ch >= 0x41 && ch <= 0x5A) || (ch >= 0x61 && ch <= 0x7A)) { ch = ((ch & 0x1F) - 1); }
    else if (ch >= 0x32 && ch <= 0x37) { ch -= (0x32 - 26); }
    else { free(temp); return 0; }

    buffer <<= 5;    
    buffer |= ch;
    bitsLeft += 5;
    if (bitsLeft >= 8)
    {
      temp[result] = (unsigned char)((unsigned int)(buffer >> (bitsLeft - 8)) & 0xFF);
      result++;
      bitsLeft -= 8;
    }
  }

  out = (byte*)malloc(result);
  memcpy(out, temp, result);
  free(temp);

  return result;
}
