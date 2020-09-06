/*********************************************************
 *
 * MDCM
 *
 * Modifications github.com/issakomi
 *
 *********************************************************/

/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "mdcmUnpacker12Bits.h"

namespace mdcm
{

bool Unpacker12Bits::Unpack(char * out, const char * in, size_t n)
{
  // 3bytes are actually 2 words
  // http://groups.google.com/group/comp.lang.c/msg/572bc9b085c717f3
  if(n % 3) return false;
  short * q = (short*)out;
  const unsigned char * p = (const unsigned char*)in;
  const unsigned char * end = p+n;
  unsigned char b0, b1, b2;
  while (p != end)
  {
    b0 = *p++;
    b1 = *p++;
    b2 = *p++;
    *q++ = (short)(((b1 & 0xf) << 8) + b0);
    *q++ = (short)((b1>>4) + (b2<<4));
  }
  return true;
}

bool Unpacker12Bits::Pack(char * out, const char * in, size_t n)
{
  // we need an even number of 'words' so that 2 words are split in 3 bytes
  if(n % 4) return false;
  unsigned char * q = (unsigned char*)out;
  const unsigned short * p = (const unsigned short*)(const void*)in;
  const unsigned short * end = (const unsigned short*)(const void*)(in+n);
  unsigned short b0, b1;
  while(p != end)
  {
    b0 = *p++;
    b1 = *p++;
    *q++ = (unsigned char)(b0 & 0xff);
    *q++ = (unsigned char)((b0 >> 8) + ((b1 & 0xf) << 4));
    *q++ = (unsigned char)(b1 >> 4);
  }
  return true;
}

} // end namespace mdcm
