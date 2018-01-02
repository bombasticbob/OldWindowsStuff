
OleRequestData (OLE_WAIT_FOR_RELEASE, OLE_RELEASE)
OleGetData
OleSetData
OleExecute
OleCopyToClipboard
OleBlockServer
OleSetHostNames
OleBlockServer
OleUnblockServer



typedef struct {
   HWND hClient;
   char szTopic[32];
   LPSTR lpLinks;     /* points to list of 'link' id's for hot/warm links */
                      /* as well as 'poked' data items and their values.  */
   LPCLIENT_PROP lpClientProp;      /* pointer to 'client prop' structure */
   WORD wAckStatus;                 /* acknowledgement status */
   } CLIENTS;

typedef CLIENTS FAR *LPCLIENTS;


typedef struct {
   WORD wType;     /* hot, warm, otherwise - 'LINK_TYPE_' flags */
   WORD wFlags;    /* indicates status of link - 'LINK_STATUS_' flags */
   WORD cfFormat;
   char item[32];
   } LINKS;

typedef LINKS FAR *LPLINKS;


typedef struct {           /* the first item in 'lpLinks' */
   WORD wMax, wCount;      /* maximum # and total # of entries */
   WORD wReserve1, wReserve2;
   char reserve3[32];
   } LINK_HEADER;

typedef LINK_HEADER FAR *LPLINK_HEADER;


typedef struct {
   HWND hClient;           /* client application expected to 'WM_DDE_ACK'  */
   ATOM aItem;             /* the atom representing the data item          */
   HGLOBAL hData;          /* the data handle - here is where I keep track */
                           /* of it so I can purge it later if required.   */
   char szItem[32];        /* I hope *THAT* is enough space for it!        */
   BOOL  wRequested;       /* the REQUESTED status of this item...         */
   WORD  cfFormat;         /* the clipboard format the data is in!         */
   DWORD dwTime;           /* posting time of entry in QUEUE for tracking. */
   } DDEACKQ;

typedef DDEACKQ FAR *LPDDEACKQ;




      default:

	 if(message>=WM_DDE_FIRST && message<=WM_DDE_LAST)
	 {
            return(Cls_OnDde(hWnd, message, (HWND)wParam,
			     LOWORD(lParam), HIWORD(lParam)));
	 }



                   /*** Global Ptr 'additional' macros ***/

#define GlobalSizePtr(x) GlobalSize(GlobalPtrHandle(x))

                   /* DDE processes defined as macros */

#define DDE_ACK_ERROR(hWnd, hClient, err, aItem)\
   PostMessage(hClient, WM_DDE_ACK, (WPARAM)(hWnd), MAKELPARAM((WORD)(err),(WORD)(aItem)))
#define DDE_ACK_BUSY(hWnd, hClient, code, aItem)\
   PostMessage(hClient, WM_DDE_ACK, (WPARAM)(hWnd), MAKELPARAM(((WORD)(code) | 0x4000),(WORD)(aItem)))
#define DDE_ACK_OK(hWnd, hClient, code, aItem)\
   PostMessage(hClient, WM_DDE_ACK, (WPARAM)(hWnd), MAKELPARAM(((WORD)(code) | 0x8000),(WORD)(aItem)))

                  /*** Global Ptr 'additional' macros ***/

#define GlobalSizePtr(x) GlobalSize(GlobalPtrHandle(x))



LPCLIENTS lpClients       = NULL;
WORD      wClientCount    = 0;    /* actual number of registered clients */
WORD      wMaxClients     = 256;  /* current maximum number of clients */
				  /* initially set to 256, can be increased */
WORD      wCustomCBFormat = 0;    /* storage for custom clipboard format ID */

LPDDEACKQ lpAckQueue      = NULL; /* pointer to 'WM_DDE_ACK expected' QUEUE */
WORD      wAckQCount      = 0;    /* current number of entries in above 'Q' */
WORD      wAckQSize       = 256;  /* total size of above QUEUE */


LRESULT NEAR PASCAL Cls_OnDde(HWND hWnd, WORD message, HWND hClient,
			      WORD wLo, WORD wHi)
{
ATOM aApp, aTopic, aItem;
HGLOBAL hCommands, hOptions, h1, hPoke;
static char buf[64];
LPLINK_HEADER lpLinkHeader;
LPLINKS lpLinks;
LPSTR lpCommands;
DDEADVISE FAR *lpAdvise;
DDEPOKE FAR *lpPoke;
WORD i, j, w, Topic, cfFormat;
WORD NEAR *pW;
DDEACK wDdeAck;
LPDDEACKQ lpQ;


   switch(message)
   {
      case WM_DDE_INITIATE:       /*** INITIATE MESSAGE ***/

         aApp = (ATOM)wLo;
	 aTopic = (ATOM)wHi;

	 if(aApp)
	 {
	    GlobalGetAtomName(aApp, buf, sizeof(buf));
	    if(!*buf) aApp = NULL;
	 }

	 if(!aApp || stricmp(buf,DDE_SERVER_NAME)==0)
	 {
	    /* right application - now, is it a valid topic? */

	    if(aTopic)
	    {
	       GlobalGetAtomName(aTopic, buf, sizeof(buf));
	       if(!*buf)  aTopic=NULL;
	    }

		 /*** CHECK VALID TOPIC ***/

	    if(aTopic)  /** FIND TOPIC IN LIST OF VALID TOPICS **/
	    {
	       for(i=0; i<wNTopicList; i++)
	       {
		  if(stricmp(buf, pTopicList[i])==0) break;
	       }
	       if(i>=wNTopicList)
	       {
		  break;  /* topic not found - ignore message */
	       }
	    }

		 /*** RESPOND TO CLIENT ***/

	    if(!aApp) aApp=GlobalAddAtom(DDE_SERVER_NAME);
	    if(!aTopic)
	    {
	      /* if no topic specified, assume 1st topic, */
	      /* which happens to be the SYSTEM topic!    */

	       aTopic=GlobalAddAtom(pTopicList[0]);

	    }


	     /* see if window already in internal list */

	    for(i=0; i<wClientCount; i++)
	    {
	       if(lpClients[i].hClient == hClient)
	       {
		  /* window already in list - 'switch' topics! */
		  /* assume the other is being terminated.     */

		  if(lpClients[i].lpLinks)
		  {

		     GlobalFreePtr((LPSTR)(lpClients[i].lpLinks));
		     lpClients[i].lpLinks = NULL;
		     lpClients[i].wAckStatus = 0;
		     _fstrcpy(lpClients[i].szTopic,buf);

		  }
	       }
	    }


	    if(i>=wClientCount)  /* add window to internal list */
	    {
	       lpClients[wClientCount].hClient = hClient;
	       lpClients[wClientCount].lpLinks = NULL;
	       lpClients[wClientCount].wAckStatus = 0;

	       GlobalGetAtomName(aTopic, buf, sizeof(buf));
	       _fstrcpy(lpClients[wClientCount].szTopic,buf);

		     /* allocate space for properties */

	       lpClients[wClientCount].lpClientProp =
			      GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
					     sizeof(CLIENT_PROP));

               lpClients[wClientCount].lpClientProp->lpData = NULL;

               GetPrivateProfileString("config","CurrentFPRD","",
                                        buffer, sizeof(buffer),
                                        GetProfileName());

               if(!*buffer)               /* no entry - use current yyyymm */
               {
                static struct dosdate_t td;

                  _dos_getdate(&td);

                  wsprintf(buffer,"%04d%02d",
                           td.year, td.month);
               }

               spaces(lpClients[wClientCount].lpClientProp->CurrentFPRD,
                      buffer, sizeof(lpClients->lpClientProp->CurrentFPRD));


	       wClientCount++;
	    }

	    SendMessage(hClient, WM_DDE_ACK, (WPARAM)hWnd,
			MAKELPARAM((WORD)aApp, (WORD)aTopic));

            TerminateFlag = FALSE;  /* if I was waiting to exit, don't! */

	    if((WORD)aApp!=wLo) GlobalDeleteAtom(aApp);
	    if((WORD)aTopic!=wHi) GlobalDeleteAtom(aTopic);

	 }

	 StatusHasChanged();

	 break;



      case WM_DDE_TERMINATE:      /*** TERMINATE MESSAGE ***/

           /* acknowledge terminate messages with a terminate message */

         if(IsWindow(hClient))     /* **ONLY** if Client is still there */
         {
            PostMessage(hClient, WM_DDE_TERMINATE, (WPARAM)hWnd, NULL);
         }

	  /* check the 'WM_DDE_ACK' queue for items which may be there */
	  /* and delete them, as appropriate.                          */

	 while(!DelAckQItem(hWnd, hClient, NULL))
	   ;  /* delete all items in QUEUE for this client */


	  /* remove entry for this client window in table (if there) */

	 for(i=0; i<wClientCount; i++)
	 {
	    if(lpClients[i].hClient == hClient)
	    {
	       wClientCount --;   /* in preparation */

	       /* window already in list - 'switch' topics! */
	       /* assume the other is being terminated.     */

	       if(lpClients[i].lpLinks)
	       {
		  GlobalFreePtr((LPSTR)(lpClients[i].lpLinks));
	       }

	       if(lpClients[i].lpClientProp)
               {
                   /** See if I'm sharing with anyone **/
               /* note: I decremented 'wClientCount' already */

                  for(j=0; j<=wClientCount; j++)
                  {
                     if(i==j) continue;

                     if(lpClients[i].lpClientProp
                        == lpClients[j].lpClientProp)
                     {
                        break;  /* found one! */
                     }
                  }
                  if(j>wClientCount)  /* did I find another sharing data? */
                  {
                       /* If I'm here, NOPE! */

    /* if there are any threads using this property, make sure they end */

                     MthreadDisableMessages();

                     KillAllClientPropThreads(lpClients[i].lpClientProp);

                     FreeAllClientPropData(lpClients[i].lpClientProp);


                     GlobalFreePtr((LPSTR)(lpClients[i].lpClientProp));

                     MthreadEnableMessages();
                  }
               }

               /* appropriately erase entry and compress others */

               if(i<wClientCount)
               {
                  _fmemcpy(lpClients+i, lpClients+i+1,
                           sizeof(CLIENTS) * (wClientCount - i));
               }

               _fmemset(lpClients + wClientCount, 0, sizeof(CLIENTS));
                     /* the 'old last item' must be zeroed out */

               break;  /* exit loop - I'm done now! */
            }
         }

         StatusHasChanged();

         break;


      case WM_DDE_ADVISE:         /*** ADVISE MESSAGE - HOT LINKS ***/

         aItem = (ATOM)wHi;       /* atom of advisory item */
         hOptions = (HGLOBAL)wLo; /* handle to 'options' structure */

         for(i=0; i<wClientCount; i++)
         {
            if(lpClients[i].hClient == hClient)
            {
               break;
            }
         }

         if(i>=wClientCount)  /* link or item not found - error code 0! */
         {
            DDE_ACK_ERROR(hWnd, hClient, 0, aItem);
            break;
         }

          /* right application - now, is it a valid item? */

         if(aItem)
         {
            GlobalGetAtomName(aItem, buf, sizeof(buf));
            if(!*buf)  aItem=NULL;
         }

         if(!aItem    /* also need to check (later) if item is valid */)
         {
                      /* link or item not found - error code 0! */

            DDE_ACK_ERROR(hWnd, hClient, 0, aItem);
            break;
         }


         if(!lpClients[i].lpLinks)  /** Allocate memory for link info **/
         {
            if(!(lpClients[i].lpLinks =
                 GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                sizeof(LINKS) * 256)))
            {
                /* an error - acknowledge with an error code 1 */
                /*            which means 'out of memory'.     */

               DDE_ACK_ERROR(hWnd, hClient, 1, aItem);
               break;
            }


            lpLinkHeader = (LPLINK_HEADER)lpClients[i].lpLinks;

            lpLinkHeader->wMax = (WORD)
                   ((GlobalSizePtr(lpLinkHeader)-sizeof(LINK_HEADER))
                     / sizeof(LINKS));

            lpLinkHeader->wCount = 0;  /* for now, eh? */
         }

         lpLinkHeader = (LPLINK_HEADER)lpClients[i].lpLinks;

            /* verify the 'links' area is large enough, and */
            /* increase the size if it is not.              */

         if(lpLinkHeader->wCount >= (lpLinkHeader->wMax - 1))
         {
          LPSTR lp1;

            if(!(lp1 = GlobalReAllocPtr(lpLinkHeader,
                                 sizeof(LINKS) * (lpLinkHeader->wCount + 256)
                                 + sizeof(LINK_HEADER),
                                 GMEM_MOVEABLE | GMEM_ZEROINIT)))
            {
                /* an error - acknowledge with an error code 1 */
                /*            which means 'out of memory'.     */

               DDE_ACK_ERROR(hWnd, hClient, 1, aItem);
               break;
            }

            lpLinkHeader = (LPLINK_HEADER)lp1;  /* new pointer! */
            lpClients[i].lpLinks = lp1;         /* and here, too. */

            lpLinkHeader->wMax = (WORD)
                   ((GlobalSizePtr(lpLinkHeader)-sizeof(LINK_HEADER))
                     / sizeof(LINKS));
         }


         lpLinks = ((LPLINKS)(((LPLINK_HEADER)(lpClients[i].lpLinks)) + 1))
                   + lpLinkHeader->wCount;

         lpLinkHeader->wCount ++;    /* add another link to the list */

         lpAdvise = (DDEADVISE FAR *)GlobalLock(hOptions);

         lpLinks->wType = LINK_TYPE_ADVISE;  /* a 'WM_DDE_ADVISE' link */
         if(lpAdvise->fAckReq) lpLinks->wType |= LINK_TYPE_ACK;
         if(lpAdvise->fDeferUpd) lpLinks->wType |= LINK_TYPE_DEFER;

         lpLinks->wFlags = 0;               /* all flags are off (for now) */
         lpLinks->cfFormat = lpAdvise->cfFormat;

         _fstrcpy((LPSTR)lpLinks->item, buf);/* copy item name into struct */

         GlobalUnlock(hOptions);
         GlobalFree(hOptions);      /* server needs to free 'hOptions' */

         DDE_ACK_OK(hWnd, hClient, 0, aItem);  /* a-ok! */

         break;


      case WM_DDE_UNADVISE:       /*** UNADVISE MESSAGE - HOT LINKS ***/

         aItem    = (ATOM)wHi;       /* atom of advisory item */
         cfFormat = wLo;          /* clipboard format to 'UNADVISE' */

         for(i=0; i<wClientCount; i++)
         {
            if(lpClients[i].hClient == hClient)
            {
               break;
            }
         }

         if(i>=wClientCount)  /* link or item not found - error code 0! */
         {
            DDE_ACK_ERROR(hWnd, hClient, 0, aItem);
            break;
         }

          /* right application - now, is it a valid item? */

         if(aItem)
         {
            GlobalGetAtomName(aItem, buf, sizeof(buf));
            if(!*buf)  aItem=NULL;
         }

         if(!aItem    /* also need to check (later) if item is valid */)
         {
                      /* link or item not found - error code 0! */

            DDE_ACK_ERROR(hWnd, hClient, 0, aItem);
            break;
         }


         if(!lpClients[i].lpLinks)  /** Allocate memory for link info **/
         {
             /* an warning - acknowledge 'ok' with an error code 3 */
             /*              which means 'no entry found'.         */

            DDE_ACK_OK(hWnd, hClient, 3, aItem);

         }

         lpLinkHeader = (LPLINK_HEADER)lpClients[i].lpLinks;

         lpLinks = (LPLINKS)(((LPLINK_HEADER)(lpClients[i].lpLinks)) + 1);

         for(i=0; i<lpLinkHeader->wCount; )
         {
            if(lpLinks[i].wType>=LINK_TYPE_ADVMIN &&
               lpLinks[i].wType<=LINK_TYPE_ADVMAX &&
               _fstricmp(lpLinks[i].item,buf)==0   &&
               (cfFormat==NULL || lpLinks[i].cfFormat==cfFormat))
            {
               /*** Delete the entry that we're currently on!! ***/

               lpLinkHeader->wCount --;

               if(i<lpLinkHeader->wCount)
               {
                  _fmemcpy(lpLinks + i, lpLinks + i + 1,
                           sizeof(LINKS) * (lpLinkHeader->wCount - i));
               }

               _fmemset(lpLinks + lpLinkHeader->wCount, 0, sizeof(LINKS));

               if(!(lpLinkHeader->wCount))
               {
                  break;  /* ensure break out of loop if 0 items left */
               }
            }
            else
            {
               i++;  /* only increment counter if item not deleted */
            }
         }

         DDE_ACK_OK(hWnd, hClient, 0, aItem);  /* a-ok! */

         break;



      case WM_DDE_REQUEST:

         aItem    = (ATOM)wHi;
         cfFormat = wLo;

         for(i=0; i<wClientCount; i++)
         {
            if(lpClients[i].hClient == hClient)
            {
               break;
            }
         }

         if(i>=wClientCount)  /* link or item not found - error code 0! */
         {
            DDE_ACK_ERROR(hWnd, hClient, 0, aItem);
            break;
         }

            /** Requesting a NORMAL data value.  To prevent **/
            /** 'Hanging' the system up in the message loop **/
            /** I shall spawn a thread to send the answer.  **/

         if(!(h1 = LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT,
                             16 * sizeof(WORD)))
            || !(pW = LocalLock(h1)))
         {
            if(h1) LocalFree(h1);
            DDE_ACK_BUSY(hWnd, hClient, 0, wHi);
                                             /* 'out of memory' - error #0 */

            break;
         }

         pW[0] = (WORD)hWnd;
         pW[1] = (WORD)hClient;
         pW[2] = wLo;           /* 'cfFormat' */
         pW[3] = wHi;           /* 'aItem' */
         pW[4] = TRUE;          /* 'wRequested' */
         pW[5] = NULL;          /* what type of link (if not requested)? */

         LocalUnlock(h1);

         if(SpawnThread(SendThread, h1, THREADSTYLE_GOON |
                                               THREADSTYLE_INTERNAL |
                                               THREADSTYLE_FARPROC)
            ==0xffff)   /* error spawning thread */
         {

            DDE_ACK_ERROR(hWnd, hClient, 6, wHi);

            LocalFree(h1);

                  /* this is a fatal error!! error code 6 - spawn error */
         }

         StatusHasChanged();

         break;


      case WM_DDE_POKE:

         aItem = (ATOM)wHi;
         hPoke = (HGLOBAL)wLo;

         if(!IsHandleValid(hPoke))
         {
            DDE_ACK_ERROR(hWnd, hClient, 0, aItem);
            break;
         }


         for(i=0; i<wClientCount; i++)
         {
            if(lpClients[i].hClient == hClient)
            {
               break;
            }
         }

         if(i>=wClientCount)  /* link or item not found - error code 0! */
         {
            DDE_ACK_ERROR(hWnd, hClient, 0, aItem);
            break;
         }

          /* to simplify matters, this does not require a thread,   */
          /* simply because it's entirely synchronous from this end */

          /* STEP 1:  Find out which topic I am trying to communicate */

         for(Topic=0; Topic<wNTopicList; Topic++)
         {
            if(_fstricmp(lpClients[i].szTopic, pTopicList[Topic])==0)
               break;
         }

         if(Topic>=wNTopicList)
         {
            DDE_ACK_ERROR(hWnd, hClient, 7, aItem); /* bad topic - error 7 */
            break;
         }

         if(Topic==0)  /* SYSTEM topic - can't poke to these!! */
         {

            DDE_ACK_ERROR(hWnd, hClient, 8, wHi); /* error 8 - can't poke! */
         }

          /* STEP 2:  Obtain the item and dispatch to appropriate proc */

         GlobalGetAtomName(aItem, buf, sizeof(buf));

         for(w=0; w<wNItemList; w++)
         {

            if((Topic==0 && _fstricmp((LPSTR)buf, pSystemList[w])==0) ||
               _fstricmp((LPSTR)buf, pItemList[w])==0)
            {

               if(!hPoke ||
                  !(lpPoke = (DDEPOKE FAR *)GlobalLock(hPoke)))
               {
                  DDE_ACK_ERROR(hWnd, hClient, 0, wHi);
                                             /* error #0 - memory problems */
                  break;
               }

                /* returns length of data, places into 'buf', */
                /* and updates the clipboard format.          */

               if((Topic==0 && lpfnSysSetProp[w] &&
                   lpfnSysSetProp[w](hClient, lpClients[i].lpClientProp,
                                     (LPSTR)lpPoke->Value, lpPoke->cfFormat))
                  || lpfnSetProp[w](hClient, lpClients[i].lpClientProp,
                                    (LPSTR)lpPoke->Value, lpPoke->cfFormat))
               {
                  GlobalUnlock(hPoke);

                  DDE_ACK_ERROR(hWnd, hClient, 9, wHi);
                             /* error #9 - can't poke, but topic/item ok */
                             /* implies that maybe format is incorrect.  */
                             /* or possibly 'out of memory' [SetData()]  */
               }
               else
               {
                  DDE_ACK_OK(hWnd, hClient, 0, wHi);

                  if(lpPoke->fRelease)  /* if TRUE, I must free memory */
                  {
                     GlobalUnlock(hPoke);
                     GlobalFree(hPoke);
                  }
                  else
                  {
                     GlobalUnlock(hPoke);
                  }

               }
               break;
            }
         }

         break;


      case WM_DDE_EXECUTE:


         hCommands = (HGLOBAL)wHi; /* handle to 'commands' string */

         for(i=0; i<wClientCount; i++)
         {
            if(lpClients[i].hClient == hClient)
            {
               break;
            }
         }

         if(i>=wClientCount)  /* link or item not found - error code 0! */
         {
            DDE_ACK_ERROR(hWnd, hClient, 0, hCommands);
            break;
         }

         lpCommands = GlobalLock(hCommands);

         if(ExecuteCommand(hClient, lpClients + i, lpCommands))
         {
            GlobalUnlock(hCommands);

              /* return error code 2 - illegal command string */
            DDE_ACK_ERROR(hWnd, hClient, 2, hCommands);
         }
         else
         {
            GlobalUnlock(hCommands);

            DDE_ACK_OK(hWnd, hClient, 0, hCommands);
         }

         break;


      case WM_DDE_ACK:

           /** In this we check for acknowledgement of sent data **/
           /** if we're waiting for an acknowledgement, we must  **/
           /** delete the associated data that belongs to it,    **/
           /** whether the acknowledgement was favorable or not. **/
           /** If an 'un-favorable' acknowledgement was received **/
           /** then we have the potential to resend 'WM_DDE_DATA'**/
           /** messages at this time.                            **/


         /* since we never expect responses to 'WM_DDE_INITIATE' or */
         /* 'WM_DDE_EXECUTE' messages, it's safe to assume that the */
         /* parameters will behave in the normal fashion.           */

         aItem = (ATOM)wHi;
         *((WORD FAR *)((LPSTR)&wDdeAck)) = wLo;


         if(!(lpQ = FindAckQItem(hWnd, hClient, aItem)))
         {
            break;   /* no need to search any further now! */
         }


         if(!wDdeAck.fAck)        /* a 'bad' acknowledgement */
         {
            if(!wDdeAck.fBusy)  /* the client was busy - try try again! */
            {
               if(!(h1 = LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT,
                                    16 * sizeof(WORD)))
                  || !(pW = LocalLock(h1)))
               {
                  if(h1) LocalFree(h1);
                  DDE_ACK_ERROR(hWnd, hClient, 9, wHi);
                     /* this is a fatal error!! error code 9 - error */
                     /* while attempting to re-send information!     */

                  break;
               }

               pW[0] = (WORD)hWnd;
               pW[1] = (WORD)hClient;
               pW[2] = lpQ->cfFormat;    /* 'cfFormat' */
               pW[3] = aItem;            /* 'aItem' */
               pW[4] = lpQ->wRequested;  /* 'wRequested' */
               pW[5] = NULL;     /* what type of link (if not requested)? */
                                 /* I assume 'HOT' at this time */

               LocalUnlock(h1);

               DelAckQItem(hWnd, hClient, aItem);
                      /* before spawning thread, clear the 'QUEUE' item */
                      /* this frees any memory, etc. that's being used. */
                      /* and updates the 'QUEUE WAIT' flags.            */

                       /* Re-Submit DATA item to client */

               if(SpawnThread(SendThread, h1, THREADSTYLE_GOON |
                                                     THREADSTYLE_INTERNAL |
                                                     THREADSTYLE_FARPROC)
                  ==0xffff)   /* error spawning thread */
               {

                   DDE_ACK_ERROR(hWnd, hClient, 9, wHi);
                     /* this is a fatal error!! error code 9 - error */
                     /* while attempting to re-send information!     */
               }

               StatusHasChanged();

               break;      /* done with processing now! */

            }

                   /** This section reserved for 'BAD ACK's **/

         }


          /** Here the request was assumed GOOD - delete item in QUEUE **/

         DelAckQItem(hWnd, hClient, aItem);    /* delete item in QUEUE! */
                                    /* automatically updates WAIT count */

         StatusHasChanged();

         break;


      default:

         return((LRESULT)1);       /* an error */
   }

   return(NULL);

}
