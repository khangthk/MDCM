/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "mdcmCryptoFactory.h"
#ifdef _WIN32
#include "mdcmCAPICryptoFactory.h"
#endif
#ifdef MDCM_USE_SYSTEM_OPENSSL
#ifdef MDCM_HAVE_CMS_RECIPIENT_PASSWORD
#include "mdcmOpenSSLCryptoFactory.h"
#endif
#include "mdcmOpenSSLP7CryptoFactory.h"
#endif

namespace mdcm
{

CryptoFactory * CryptoFactory::GetFactoryInstance(CryptoLib id)
{
#ifdef _WIN32
  static CAPICryptoFactory capi(CryptoFactory::CAPI);
#endif
#ifdef MDCM_USE_SYSTEM_OPENSSL
#ifdef MDCM_HAVE_CMS_RECIPIENT_PASSWORD
  static OpenSSLCryptoFactory ossl(CryptoFactory::OPENSSL);
#endif
  static OpenSSLP7CryptoFactory osslp7(CryptoFactory::OPENSSLP7);
#endif
  if(id == DEFAULT)
  {
#ifdef MDCM_USE_SYSTEM_OPENSSL
#ifdef MDCM_HAVE_CMS_RECIPIENT_PASSWORD
    id = CryptoFactory::OPENSSL;
#else
    id = CryptoFactory::OPENSSLP7;
#endif
#endif
#ifdef _WIN32
    id = CryptoFactory::CAPI;
#endif
  }
  std::map<CryptoLib, CryptoFactory*>::iterator it =
    getInstanceMap().find(id);
  if (it == getInstanceMap().end())
  {
    mdcmErrorMacro("No crypto factory registered with id " << (int)id);
    return NULL;
  }
  assert(it->second);
  return it->second;
}

} // end native mdcm
