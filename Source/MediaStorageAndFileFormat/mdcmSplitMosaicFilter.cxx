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

#include "mdcmSplitMosaicFilter.h"
#include "mdcmCSAHeader.h"
#include "mdcmAttribute.h"
#include "mdcmImageHelper.h"
#include "mdcmDirectionCosines.h"
#include <cmath>

namespace
{

template<typename T> bool
reorganize_mosaic(const T *            input,
                  const unsigned int * inputdims,
                  unsigned int         square,
                  const unsigned int * outputdims,
                  T *                  output)
{
  const size_t outputdims_0 = static_cast<size_t>(outputdims[0]);
  const size_t outputdims_1 = static_cast<size_t>(outputdims[1]);
  const size_t outputdims_2 = static_cast<size_t>(outputdims[2]);
  for (size_t x = 0; x < outputdims_0; ++x)
  {
    for (size_t y = 0; y < outputdims_1; ++y)
    {
      for (size_t z = 0; z < outputdims_2; ++z)
      {
        const size_t outputidx =
          x + y * outputdims_0 + z * outputdims_0 * outputdims_1;
        const size_t inputidx =
          (x + (z % square) * outputdims_0) + (y + (z / square) * outputdims_1) * inputdims[0];
        output[outputidx] = input[inputidx];
      }
    }
  }
  return true;
}

#ifdef SNVINVERT
template<typename T> bool
reorganize_mosaic_invert(const T *            input,
                         const unsigned int * inputdims,
                         unsigned int         square,
                         const unsigned int * outputdims,
                         T *                  output)
{
  const size_t outputdims_0 = static_cast<size_t>(outputdims[0]);
  const size_t outputdims_1 = static_cast<size_t>(outputdims[1]);
  const size_t outputdims_2 = static_cast<size_t>(outputdims[2]);
  for (size_t x = 0; x < outputdims_0; ++x)
  {
    for (size_t y = 0; y < outputdims_1; ++y)
    {
      for (size_t z = 0; z < outputdims_2; ++z)
      {
        const size_t outputidx =
          x + y * outputdims_0 + (outputdims_2 - 1 - z) * outputdims_0 * outputdims_1;
        const size_t inputidx =
          (x + (z % square) * outputdims_0) + (y + (z / square) * outputdims_1) * inputdims[0];
        output[outputidx] = input[inputidx];
      }
    }
  }
  return true;
}
#endif

}

namespace mdcm
{

SplitMosaicFilter::SplitMosaicFilter()
  : F(new File)
  , I(new Image)
{}

void
SplitMosaicFilter::SetImage(const Image & image)
{
  I = image;
}

bool
SplitMosaicFilter::ComputeMOSAICDimensions(unsigned int dims[3])
{
  CSAHeader          csa;
  DataSet &          ds = GetFile().GetDataSet();
  const PrivateTag & t1 = csa.GetCSAImageHeaderInfoTag();
  int                numberOfImagesInMosaic = 0;
  if (csa.LoadFromDataElement(ds.GetDataElement(t1)))
  {
    if (csa.FindCSAElementByName("NumberOfImagesInMosaic"))
    {
      const CSAElement & csael4 = csa.GetCSAElementByName("NumberOfImagesInMosaic");
      if (!csael4.IsEmpty())
      {
        Element<VR::IS, VM::VM1> el4 = { { 0 } };
        el4.Set(csael4.GetValue());
        numberOfImagesInMosaic = el4.GetValue();
      }
    }
  }
  else
  {
    // Some weird anonymizer remove the private creator but leave the actual element.
    // (0019,100a) US 72   # 2,1 NumberOfImagesInMosaic
    PrivateTag t2(0x0019, 0x0a, "SIEMENS MR HEADER");
    if (ds.FindDataElement(t2))
    {
      const DataElement & de = ds.GetDataElement(t2);
      const ByteValue *   bv = de.GetByteValue();
      if (bv)
      {
        Element<VR::US, VM::VM1> el1 = { { 0 } };
        std::istringstream       is;
        is.str(std::string(bv->GetPointer(), bv->GetLength()));
        el1.Read(is);
        numberOfImagesInMosaic = el1.GetValue();
      }
    }
  }
  if (!numberOfImagesInMosaic)
  {
    mdcmErrorMacro("Could not find NumberOfImagesInMosaic");
    return false;
  }
  std::vector<unsigned int> colrow = ImageHelper::GetDimensionsValue(GetFile());
  dims[0] = colrow[0];
  dims[1] = colrow[1];
  const unsigned int div = static_cast<unsigned int>(ceil(sqrt(static_cast<double>(numberOfImagesInMosaic))));
  dims[0] /= div;
  dims[1] /= div;
  dims[2] = numberOfImagesInMosaic;
  return true;
}

bool
SplitMosaicFilter::ComputeMOSAICSliceNormal(double slicenormalvector[3], bool & inverted)
{
  CSAHeader          csa;
  DataSet &          ds = GetFile().GetDataSet();
  double             normal[3];
  bool               snvfound = false;
  const PrivateTag & t1 = csa.GetCSAImageHeaderInfoTag();
  static const char  snvstr[] = "SliceNormalVector";
  if (csa.LoadFromDataElement(ds.GetDataElement(t1)))
  {
    if (csa.FindCSAElementByName(snvstr))
    {
      const CSAElement & snv_csa = csa.GetCSAElementByName(snvstr);
      if (!snv_csa.IsEmpty())
      {
        const ByteValue *  bv = snv_csa.GetByteValue();
        const std::string  str(bv->GetPointer(), bv->GetLength());
        std::istringstream is;
        is.str(str);
        char sep;
        if (is >> normal[0] >> sep >> normal[1] >> sep >> normal[2])
        {
          snvfound = true;
        }
      }
    }
  }
  if (snvfound)
  {
    Attribute<0x20, 0x37> iop;
    iop.SetFromDataSet(ds);
    DirectionCosines dc(iop.GetValues());
    double           z[3];
    dc.Cross(z);
    const double snv_dot = dc.Dot(normal, z);
    if ((1. - snv_dot) < 1e-6)
    {
      mdcmDebugMacro("Same direction");
      inverted = false;
    }
    else if ((-1. - snv_dot) < 1e-6)
    {
      mdcmAlwaysWarnMacro("SliceNormalVector seems to be inverted");
      inverted = true;
    }
    else
    {
      mdcmAlwaysWarnMacro("Unexpected normal for SliceNormalVector, dot is: " << snv_dot);
      return false;
    }
    for (unsigned int i = 0; i < 3; ++i)
    {
      slicenormalvector[i] = normal[i];
    }
  }
  return snvfound;
}

bool
SplitMosaicFilter::ComputeMOSAICSlicePosition(double pos[3], bool)
{
  CSAHeader  csa;
  DataSet &  ds = GetFile().GetDataSet();
  MrProtocol mrprot;
  if (!csa.GetMrProtocol(ds, mrprot))
  {
    return false;
  }
  MrProtocol::SliceArray sa;
  const bool             b = mrprot.GetSliceArray(sa);
  if (!b)
  {
    return false;
  }
  const size_t size = sa.Slices.size();
  if (!size)
  {
    return false;
  }
/*
  {
    double z[3];
    for(int i = 0; i < size; ++i)
    {
      MrProtocol::Slice & slice = sa.Slices[i];
      MrProtocol::Vector3 & p = slice.Position;
      z[0] = p.dSag;
      z[1] = p.dCor;
      z[2] = p.dTra;
      const double snv_dot = DirectionCosines::Dot(slicenormalvector, z);
      if((1. - snv_dot) < 1e-6)
      {
        mdcmErrorMacro("Invalid direction found");
        return false;
      }
    }
  }
*/
  const size_t          index = 0;
  MrProtocol::Slice &   slice = sa.Slices[index];
  MrProtocol::Vector3 & p = slice.Position;
  pos[0] = p.dSag;
  pos[1] = p.dCor;
  pos[2] = p.dTra;
  return true;
}

bool
SplitMosaicFilter::Split()
{
  bool         success = true;
  DataSet &    ds = GetFile().GetDataSet();
  unsigned int dims[3] = { 0, 0, 0 };
  if (!ComputeMOSAICDimensions(dims))
  {
    return false;
  }
  const unsigned int div = static_cast<unsigned int>(ceil(sqrt(static_cast<double>(dims[2]))));
  bool               inverted;
  double             normal[3];
  if (!ComputeMOSAICSliceNormal(normal, inverted))
  {
    return false;
  }
  double origin[3];
  if (!ComputeMOSAICSlicePosition(origin, inverted))
  {
    return false;
  }
  const Image & inputimage = GetImage();
  unsigned long long l = inputimage.GetBufferLength();
  if (l >= 0xffffffff)
  {
    mdcmAlwaysWarnMacro("SplitMosaicFilter: buffer length = " << l);
    return false;
  }
  std::vector<char> buf;
  buf.resize(l);
  inputimage.GetBuffer(buf.data());
  DataElement       pixeldata(Tag(0x7fe0, 0x0010));
  std::vector<char> outbuf;
  outbuf.resize(l);
  void * vbuf = static_cast<void*>(buf.data());
  void * voutbuf = static_cast<void*>(outbuf.data());
  bool b = false;
#ifdef SNVINVERT
  if (inverted)
  {
    if (inputimage.GetPixelFormat() == PixelFormat::UINT16)
    {
      b = reorganize_mosaic_invert<unsigned short>(static_cast<unsigned short *>(vbuf),
                                                   inputimage.GetDimensions(),
                                                   div,
                                                   dims,
                                                   static_cast<unsigned short *>(voutbuf));
    }
    else if (inputimage.GetPixelFormat() == PixelFormat::INT16)
    {
      b = reorganize_mosaic_invert<signed short>(static_cast<signed short *>(vbuf),
                                                 inputimage.GetDimensions(),
                                                 div,
                                                 dims,
                                                 static_cast<signed short *>(voutbuf));
    }
    else if (inputimage.GetPixelFormat() == PixelFormat::UINT8)
    {
      b = reorganize_mosaic_invert<unsigned char>(static_cast<unsigned char *>(vbuf),
                                                  inputimage.GetDimensions(),
                                                  div,
                                                  dims,
                                                  static_cast<unsigned char *>(voutbuf));
    }
    else if (inputimage.GetPixelFormat() == PixelFormat::INT8)
    {
      b = reorganize_mosaic_invert<signed char>(static_cast<signed char *>(vbuf),
                                                inputimage.GetDimensions(),
                                                div,
                                                dims,
                                                static_cast<signed char *>(voutbuf));
    }
  }
  else
#else
  if (inverted)
  {
    mdcmAlwaysWarnMacro("SplitMosaicFilter: slice normal may be inverted");
  }
#endif
  {
    if (inputimage.GetPixelFormat() == PixelFormat::UINT16)
    {
      b = reorganize_mosaic<unsigned short>(static_cast<unsigned short *>(vbuf),
                                            inputimage.GetDimensions(),
                                            div,
                                            dims,
                                            static_cast<unsigned short *>(voutbuf)); 
    }
    else if (inputimage.GetPixelFormat() == PixelFormat::INT16)
    {
      b = reorganize_mosaic<signed short>(static_cast<signed short *>(vbuf),
                                          inputimage.GetDimensions(),
                                          div,
                                          dims,
                                          static_cast<signed short *>(voutbuf));
    }
    else if (inputimage.GetPixelFormat() == PixelFormat::UINT8)
    {
      b = reorganize_mosaic<unsigned char>(static_cast<unsigned char *>(vbuf),
                                           inputimage.GetDimensions(),
                                           div,
                                           dims,
                                           static_cast<unsigned char *>(voutbuf));
    }
    else if (inputimage.GetPixelFormat() == PixelFormat::INT8)
    {
      b = reorganize_mosaic<signed char>(static_cast<signed char *>(vbuf),
                                         inputimage.GetDimensions(),
                                         div,
                                         dims,
                                         static_cast<signed char *>(voutbuf));
    }
  }
  if (!b)
    return false;
  const unsigned long long outbuf_size = outbuf.size();
  if (outbuf_size >= 0xffffffff)
  {
    mdcmAlwaysWarnMacro("outbuf_size=" << outbuf_size);
    return false;
  }
  pixeldata.SetByteValue(outbuf.data(), static_cast<VL::Type>(outbuf_size));
  Image &                image = GetImage();
  const TransferSyntax & ts = image.GetTransferSyntax();
  if (ts.IsExplicit())
  {
    image.SetTransferSyntax(TransferSyntax::ExplicitVRLittleEndian);
  }
  else
  {
    image.SetTransferSyntax(TransferSyntax::ImplicitVRLittleEndian);
  }
  image.SetNumberOfDimensions(3);
  image.SetDimension(0, dims[0]);
  image.SetDimension(1, dims[1]);
  image.SetDimension(2, dims[2]);
  // Fix origin (direction is ok since we reorganize the tiles):
  image.SetOrigin(origin);
  PhotometricInterpretation pi;
  pi = PhotometricInterpretation::MONOCHROME2;
  image.SetDataElement(pixeldata);
  // Second part need to fix the Media Storage, now that this is not a single slice anymore
  MediaStorage ms = MediaStorage::SecondaryCaptureImageStorage;
  ms.SetFromFile(GetFile());
  if (ms != MediaStorage::MRImageStorage)
  {
    mdcmAlwaysWarnMacro("SplitMosaicFilter: expected MR Image Storage");
  }
  DataElement de(Tag(0x0008, 0x0016));
  const char *   msstr = MediaStorage::GetMSString(ms);
  const VL::Type strlenMsstr = static_cast<VL::Type>(strlen(msstr));
  de.SetByteValue(msstr, strlenMsstr);
  de.SetVR(Attribute<0x0008, 0x0016>::GetVR());
  ds.Replace(de);
  return success;
}

} // end namespace mdcm
