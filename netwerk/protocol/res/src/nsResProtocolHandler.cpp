/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *
 *
 * This Original Code has been modified by IBM Corporation.
 * Modifications made by IBM described herein are
 * Copyright (c) International Business Machines
 * Corporation, 2000
 *
 * Modifications to Mozilla code or documentation
 * identified per MPL Section 3.3
 *
 * Date         Modified by     Description of modification
 * 03/23/2000   IBM Corp.       Fixed bug with OS2_SystemDirectory.
 * 03/27/2000   IBM Corp.       Added PR_CALLBACK for Optlink
 *                               use in OS2
 */

#include "nsResProtocolHandler.h"
#include "nsAutoLock.h"
#include "nsIURL.h"
#include "nsIIOService.h"
#include "nsIServiceManager.h"
#include "prenv.h"
#include "prmem.h"
#include "prprf.h"
#include "nsXPIDLString.h"
#include "nsIFile.h"
#include "nsDirectoryServiceDefs.h"
#include "nsNetCID.h"
#include "nsResURL.h"

static NS_DEFINE_CID(kStandardURLCID, NS_STANDARDURL_CID);
static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);

////////////////////////////////////////////////////////////////////////////////

nsResProtocolHandler *nsResProtocolHandler::mGlobalInstance = nsnull;

nsResProtocolHandler::nsResProtocolHandler()
    : mLock(nsnull), mSubstitutions(32)
{
    NS_INIT_REFCNT();

    NS_ASSERTION(!mGlobalInstance, "res handler already created!");
    mGlobalInstance = this;
}

nsResProtocolHandler::~nsResProtocolHandler()
{
    if (mLock)
        PR_DestroyLock(mLock);

    mGlobalInstance = nsnull;
}

nsresult
nsResProtocolHandler::SetSpecialDir(const char* rootName, const char* sysDir)
{
    nsresult rv;
    nsCOMPtr<nsIFile> file;
    rv = NS_GetSpecialDirectory(sysDir, getter_AddRefs(file));
    if (NS_FAILED(rv)) return rv;

    nsXPIDLCString spec;
    rv = file->GetURL(getter_Copies(spec));
    if (NS_FAILED(rv)) return rv;

    return AppendSubstitution(rootName, spec);
}

nsresult
nsResProtocolHandler::Init()
{
    nsresult rv;

    mIOService = do_GetService(kIOServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    mLock = PR_NewLock();
    if (mLock == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;

    // set up initial mappings
    rv = SetSpecialDir("ProgramDir", NS_OS_CURRENT_PROCESS_DIR);
    if (NS_FAILED(rv)) return rv;

    // make "res:///" == "resource:/"
    rv = SetSpecialDir("", NS_XPCOM_CURRENT_PROCESS_DIR);
    if (NS_FAILED(rv)) return rv;

    rv = SetSpecialDir("CurrentDir", NS_OS_CURRENT_WORKING_DIR);
    if (NS_FAILED(rv)) return rv;

    rv = SetSpecialDir("CurrentDrive", NS_OS_DRIVE_DIR);
    if (NS_FAILED(rv)) return rv;

    rv = SetSpecialDir("TempDir", NS_OS_TEMP_DIR);
    if (NS_FAILED(rv)) return rv;

    rv = SetSpecialDir("ComponentsDir", NS_XPCOM_COMPONENT_DIR);
    if (NS_FAILED(rv)) return rv;

    rv = SetSpecialDir("SystemDir",
#ifdef XP_MAC
                       NS_OS_SYSTEM_DIR
#elif XP_OS2
                       NS_OS_SYSTEM_DIR
#elif XP_PC
                       NS_OS_SYSTEM_DIR
#elif XP_BEOS
                       NS_OS_SYSTEM_DIR
#else
                       NS_UNIX_LIB_DIR  // XXX ???
#endif
                       );
    if (NS_FAILED(rv)) return rv;

    // Set up the "Resource" root to point to the old resource location 
    // such that:
    //     resource://<path>  ==  res://Resource/<path>
    rv = SetSpecialDir("Resource", NS_XPCOM_CURRENT_PROCESS_DIR);
    if (NS_FAILED(rv)) return rv;

    return rv;
}

NS_IMPL_THREADSAFE_ISUPPORTS3(nsResProtocolHandler, nsIResProtocolHandler, nsIProtocolHandler, nsISupportsWeakReference)

NS_METHOD
nsResProtocolHandler::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    if (aOuter)
        return NS_ERROR_NO_AGGREGATION;

    nsResProtocolHandler* ph = new nsResProtocolHandler();
    if (ph == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(ph);
    nsresult rv = ph->Init();
    if (NS_SUCCEEDED(rv)) {
        rv = ph->QueryInterface(aIID, aResult);
    }
    NS_RELEASE(ph);
    return rv;
}

////////////////////////////////////////////////////////////////////////////////
// nsIProtocolHandler methods:

NS_IMETHODIMP
nsResProtocolHandler::GetScheme(char* *result)
{
    *result = nsCRT::strdup("resource");
    if (*result == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}

NS_IMETHODIMP
nsResProtocolHandler::GetDefaultPort(PRInt32 *result)
{
    *result = -1;        // no port for res: URLs
    return NS_OK;
}

NS_IMETHODIMP
nsResProtocolHandler::GetURIType(PRInt16 *result)
{
    *result = URI_STD;
    return NS_OK;
}

NS_IMETHODIMP
nsResProtocolHandler::NewURI(const char *aSpec, nsIURI *aBaseURI,
                             nsIURI **result)
{
    nsresult rv;

    nsCOMPtr<nsIURI> uri(do_CreateInstance(kStandardURLCID, &rv));
    if (NS_FAILED(rv)) return rv;

    if (aBaseURI) {
        nsXPIDLCString aResolvedURI;
        rv = aBaseURI->Resolve(aSpec, getter_Copies(aResolvedURI));
        if (NS_FAILED(rv)) return rv;
        rv = uri->SetSpec(aResolvedURI);
    } else {
        rv = uri->SetSpec((char*)aSpec);
    }

    if (NS_FAILED(rv))
        return rv;

    nsResURL *resURL;
    NS_NEWXPCOM(resURL, nsResURL);
    if (!resURL)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(resURL);

    rv = resURL->Init(uri);
    if (NS_SUCCEEDED(rv))
        rv = CallQueryInterface(resURL, result);
    NS_RELEASE(resURL);
    return rv;
}

NS_IMETHODIMP
nsResProtocolHandler::NewChannel(nsIURI* uri, nsIChannel* *result)
{
    nsresult rv;
    nsXPIDLCString spec;

    rv = ResolveURI(uri, getter_Copies(spec));
    if (NS_FAILED(rv)) return rv;

    rv = mIOService->NewChannel(spec, nsnull, result);
    if (NS_FAILED(rv)) return rv;

    return (*result)->SetOriginalURI(uri);
}

NS_IMETHODIMP 
nsResProtocolHandler::AllowPort(PRInt32 port, const char *scheme, PRBool *_retval)
{
    // don't override anything.  
    *_retval = PR_FALSE;
    return NS_OK;
}
////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP
nsResProtocolHandler::PrependSubstitution(const char *root, const char *urlStr)
{
    nsresult rv;
    nsAutoLock lock(mLock);

    nsCOMPtr<nsIURI> url;
    rv = mIOService->NewURI(urlStr, nsnull, getter_AddRefs(url));
    if (NS_FAILED(rv)) return rv;

    nsCStringKey key(root);
    nsCOMPtr<nsISupportsArray> strings;
    nsCOMPtr<nsISupportsArray> newStrings;

    strings = dont_AddRef(NS_STATIC_CAST(nsISupportsArray*,
                                         mSubstitutions.Get(&key)));
    if (strings) {
        // we have to snapshot the array when inserting a new element because
        // someone could be iterating over the existing array
        rv = strings->Clone(getter_AddRefs(newStrings));
        if (NS_FAILED(rv)) return rv;
    }
    else {
        rv = NS_NewISupportsArray(getter_AddRefs(newStrings));
        if (NS_FAILED(rv)) return rv;
    }

    rv = newStrings->InsertElementAt(url, 0) ? NS_OK : NS_ERROR_FAILURE;
    if (NS_SUCCEEDED(rv)) {
        // replace existing array
        (void)mSubstitutions.Put(&key, newStrings);
    }
    return rv;
}

NS_IMETHODIMP
nsResProtocolHandler::AppendSubstitution(const char *root, const char *urlStr)
{
    nsresult rv;
    nsAutoLock lock(mLock);

    nsCOMPtr<nsIURI> url;
    rv = mIOService->NewURI(urlStr, nsnull, getter_AddRefs(url));
    if (NS_FAILED(rv)) return rv;

    nsCStringKey key(root);
    nsCOMPtr<nsISupportsArray> strings;
    nsCOMPtr<nsISupportsArray> newStrings;

    strings = getter_AddRefs((nsISupportsArray*)mSubstitutions.Get(&key));
    if (strings) {
        // we have to snapshot the array when inserting a new element because
        // someone could be iterating over the existing array
        rv = strings->Clone(getter_AddRefs(newStrings));
        if (NS_FAILED(rv)) return rv;
    }
    else {
        rv = NS_NewISupportsArray(getter_AddRefs(newStrings));
        if (NS_FAILED(rv)) return rv;
    }

    rv = newStrings->AppendElement(url) ? NS_OK : NS_ERROR_FAILURE;
    if (NS_SUCCEEDED(rv)) {
        // replace existing array
        (void)mSubstitutions.Put(&key, newStrings);
    }
    return rv;
}

struct CopyMostArgs {
    nsISupportsArray* mNewArray;
    nsIURI* mRemoveURL;
};

static PRBool
CopyMost(nsISupports* aElement, void *aData)
{
    nsresult rv = NS_OK;
    CopyMostArgs* args = (CopyMostArgs*)aData;
    nsIURI* thisURL = (nsIURI*)aElement;
    PRBool eq;
    rv = thisURL->Equals(args->mRemoveURL, &eq);
    if (NS_FAILED(rv)) return PR_FALSE;
    if (!eq) {
        rv = args->mNewArray->AppendElement(thisURL) ? NS_OK : NS_ERROR_FAILURE;
    }
    return NS_SUCCEEDED(rv);
}

NS_IMETHODIMP
nsResProtocolHandler::RemoveSubstitution(const char *root, const char *urlStr)
{
    nsresult rv;
    nsAutoLock lock(mLock);

    nsCOMPtr<nsIURI> url;
    rv = mIOService->NewURI(urlStr, nsnull, getter_AddRefs(url));
    if (NS_FAILED(rv)) return rv;

    nsCStringKey key(root);
    nsCOMPtr<nsISupportsArray> strings;
    nsCOMPtr<nsISupportsArray> newStrings;

    strings = getter_AddRefs((nsISupportsArray*)mSubstitutions.Get(&key));
    if (strings == nsnull) {
        // can't remove element if it doesn't exist
        return NS_ERROR_FAILURE;
    }

    rv = NS_NewISupportsArray(getter_AddRefs(newStrings));
    if (NS_FAILED(rv)) return rv;

    CopyMostArgs args;
    args.mNewArray = newStrings;
    args.mRemoveURL = url;
    PRBool ok = strings->EnumerateForwards(CopyMost, &args);
    rv = ok ? NS_OK : NS_ERROR_FAILURE;
    if (NS_SUCCEEDED(rv)) {
        // replace existing array
        (void)mSubstitutions.Put(&key, newStrings);
    }
    return rv;
}

NS_IMETHODIMP
nsResProtocolHandler::GetSubstitutions(const char *root, nsISupportsArray* *result)
{
    nsresult rv;
    nsAutoLock lock(mLock);

    nsCStringKey key(root);
    nsISupportsArray* strings = (nsISupportsArray*)mSubstitutions.Get(&key);
    if (strings == nsnull) {
        rv = NS_NewISupportsArray(&strings);
        if (NS_FAILED(rv)) return rv;
        (void)mSubstitutions.Put(&key, strings);
    }
    *result = strings;
    return NS_OK;
}

NS_IMETHODIMP
nsResProtocolHandler::HasSubstitutions(const char *root, PRBool *result)
{
    nsCStringKey key(root);
    *result = mSubstitutions.Exists(&key);
    return NS_OK;
}

NS_IMETHODIMP
nsResProtocolHandler::ResolveURI(nsIURI *uri, char **result)
{
    nsresult rv;
    nsXPIDLCString host, path;

    rv = uri->GetHost(getter_Copies(host));
    if (NS_FAILED(rv)) return rv;

    rv = uri->GetPath(getter_Copies(path));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsISupportsArray> substitutions;
    rv = GetSubstitutions(host.get() ?
                          host.get() : "", getter_AddRefs(substitutions));
    if (NS_FAILED(rv)) return rv;

    // always use the first substitution
    nsCOMPtr<nsIURI> substURI;
    rv = substitutions->GetElementAt(0, getter_AddRefs(substURI));
    if (NS_FAILED(rv)) return rv;

    return substURI->Resolve(path[0] == '/' ? path+1 : path, result);
}

////////////////////////////////////////////////////////////////////////////////
