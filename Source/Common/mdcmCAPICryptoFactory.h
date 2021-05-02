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
#ifndef MDCMCAPICRYPTOFACTORY_H
#define MDCMCAPICRYPTOFACTORY_H

#include "mdcmCryptoFactory.h"

namespace mdcm
{

class MDCM_EXPORT CAPICryptoFactory : public CryptoFactory
{
public:
  CAPICryptoFactory(CryptoLib);
  CryptographicMessageSyntax *
  CreateCMSProvider() override;

private:
  CAPICryptoFactory() {}
};

} // end namespace mdcm

#endif // MDCMCAPICRYPTOFACTORY_H
