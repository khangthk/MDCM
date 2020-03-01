/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "mdcmUNExplicitDataElement.h"
#include "mdcmSequenceOfItems.h"
#include "mdcmSequenceOfFragments.h"

namespace mdcm
{

VL UNExplicitDataElement::GetLength() const
{
  if(ValueLengthField.IsUndefined())
  {
    assert(ValueField->GetLength().IsUndefined());
    Value *p = ValueField;
    SequenceOfItems *sq = dynamic_cast<SequenceOfItems*>(p);
    if(sq)
    {
      return (TagField.GetLength() + VRField.GetLength() +
        ValueLengthField.GetLength() + sq->ComputeLength<UNExplicitDataElement>());
    }
    SequenceOfFragments *sf = dynamic_cast<SequenceOfFragments*>(p);
    if(sf)
    {
      assert(VRField & (VR::OB | VR::OW));
      return (TagField.GetLength() + VRField.GetLength()
        + ValueLengthField.GetLength() + sf->ComputeLength());
    }
    assert(0);
  return 0;
  }
  else
  {
    // Each time VR::GetLength() is 2 then Value Length is coded in 2
    //                              4 then Value Length is coded in 4
    assert(!ValueField || ValueField->GetLength() == ValueLengthField);
    return TagField.GetLength() + 2*VRField.GetLength() + ValueLengthField;
  }
}


} // end namespace mdcm
