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

#include "mdcmWriter.h"
#include "mdcmFileMetaInformation.h"
#include "mdcmDataSet.h"
#include "mdcmTrace.h"
#include "mdcmSwapper.h"
#include "mdcmDataSet.h"
#include "mdcmExplicitDataElement.h"
#include "mdcmImplicitDataElement.h"
#include "mdcmValue.h"
#include "mdcmValue.h"
#include "mdcmItem.h"
#include "mdcmSequenceOfItems.h"
#include "mdcmParseException.h"
#include "mdcmDeflateStream.h"

namespace mdcm
{

Writer::Writer() :
  Stream(NULL),
  Ofstream(NULL),
  F(new File),
  CheckFileMetaInformation(true),
  WriteDataSetOnly(false)
{
}

Writer::~Writer()
{
  if (Ofstream)
  {
    delete Ofstream;
    Ofstream = NULL;
    Stream = NULL;
  }
}

bool Writer::Write()
{
  if(!Stream || !*Stream)
  {
    mdcmErrorMacro("No Filename");
    return false;
  }

  std::ostream &os = *Stream;
  FileMetaInformation &Header = F->GetHeader();
  DataSet &DS = F->GetDataSet();

  if(DS.IsEmpty())
  {
    mdcmErrorMacro("DS empty");
    return false;
  }

  // Check that 0002,0002 / 0008,0016 and 0002,0003 / 0008,0018 match?

  if(!WriteDataSetOnly)
  {
    if(CheckFileMetaInformation)
    {
      FileMetaInformation duplicate(Header);
      try
      {
        duplicate.FillFromDataSet(DS);
      }
      catch(mdcm::Exception & ex)
      {
#if defined(NDEBUG)
        (void)ex;
#else
        mdcmErrorMacro("Could not recreate the File Meta Header, please report:" << ex.what());
#endif
        return false;
      }
      duplicate.Write(os);
    }
    else
    {
      Header.Write(os);
    }
  }

  const TransferSyntax & ts = Header.GetDataSetTransferSyntax();
  if(!ts.IsValid())
  {
    mdcmErrorMacro("Invalid Transfer Syntax");
    return false;
  }

  if(ts == TransferSyntax::DeflatedExplicitVRLittleEndian)
  {
    try
    {
      zlib_stream::zip_ostream gzos(os);
      assert(ts.GetNegociatedType() == TransferSyntax::Explicit);
      DS.Write<ExplicitDataElement,SwapperNoOp>(gzos);
    }
    catch (...)
    {
      return false;
    }
    return true;
  }

  try
  {
    if(ts.GetSwapCode() == SwapCode::BigEndian)
    {
      //US-RGB-8-epicard.dcm is big endian
      if(ts.GetNegociatedType() == TransferSyntax::Implicit)
      {
        // There is no such thing as Implicit Big Endian... oh well
        // LIBIDO-16-ACR_NEMA-Volume.dcm
        DS.Write<ImplicitDataElement,SwapperDoOp>(os);
      }
      else
      {
        assert(ts.GetNegociatedType() == TransferSyntax::Explicit);
        DS.Write<ExplicitDataElement,SwapperDoOp>(os);
      }
    }
    else // LittleEndian
    {
      if(ts.GetNegociatedType() == TransferSyntax::Implicit)
      {
        DS.Write<ImplicitDataElement,SwapperNoOp>(os);
      }
      else
      {
        assert(ts.GetNegociatedType() == TransferSyntax::Explicit);
        DS.Write<ExplicitDataElement,SwapperNoOp>(os);
      }
    }
  }
  catch(std::exception & ex)
  {
#if defined(NDEBUG)
    (void)ex;
#else
    mdcmErrorMacro(ex.what());
#endif
    return false;
  }
  catch(...)
  {
    mdcmErrorMacro("unknown exception");
    return false;
  }

  os.flush();
  if (Ofstream)
  {
    Ofstream->close();
  }

  return true;
}

void Writer::SetFileName(const char * filename)
{
    if (Ofstream)
    {
      if (Ofstream->is_open())
      {
        Ofstream->close();
      }
      delete Ofstream;
    }
    Ofstream = new std::ofstream();
    Ofstream->open(filename, std::ios::out | std::ios::binary);
    assert(Ofstream->is_open());
    assert(!Ofstream->fail());
    Stream = Ofstream;
}

} // end namespace mdcm
