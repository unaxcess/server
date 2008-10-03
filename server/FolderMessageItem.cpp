/*
** UNaXcess Conferencing System
** (c) 1998 Michael Wood (mike@compsoc.man.ac.uk)
**
** Concepts based on Bradford UNaXcess (c) 1984-87 Brandon S Allbery
** Extensions (c) 1989, 1990 Andrew G Minter
** Manchester UNaXcess extensions by Rob Partington, Gryn Davies,
** Michael Wood, Andrew Armitage, Francis Cook, Brian Widdas
**
** The look and feel was reproduced. No code taken from the original
** UA was someone else's inspiration. Copyright and 'nuff respect due
**
** FolderMessageItem.cpp: Implmentation of FolderMessageItem class
*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "ua.h"

#include "FolderMessageItem.h"
#include "MessageTreeItem.h"

// #define MESSAGEWRITE_HIDDEN 1

FolderMessageItem *FolderMessageItem::m_pInit = NULL;

// Constructor with no data
FolderMessageItem::FolderMessageItem(long lID, FolderMessageItem *pParent, MessageTreeItem *pTree) : MessageItem(lID, pParent, pTree)
{
   STACKTRACE

   init(pTree);
}

// Constructor with EDF data
FolderMessageItem::FolderMessageItem(EDF *pEDF, FolderMessageItem *pParent, MessageTreeItem *pTree) : MessageItem(pEDF, pParent, pTree)
{
   STACKTRACE

   // printf("FolderMessageItem::FolderMessageItem %p %p %p\n", pEDF, pParent, pTree);

   init(pTree);

   // ExtractMember(pEDF, "topid", &m_lTopID, -1);

   EDFFields(pEDF);

   setup();
}

FolderMessageItem::FolderMessageItem(DBTable *pTable, FolderMessageItem *pParent, MessageTreeItem *pTree) : MessageItem(pTable, pParent, pTree, 8)
{
   init(pTree);

   // GET_INT_TABLE(7, m_lTopID, -1)

   EDFFields(NULL);

   setup();
}

// Destructor
FolderMessageItem::~FolderMessageItem()
{
   STACKTRACE
   // printf("FolderMessageItem::~FolderMessageItem %p(%ld)\n", this, GetID());
}

/* const char *FolderMessageItem::GetClass()
{
   return typeid(this).name();
} */

/* long FolderMessageItem::GetTopID()
{
   return m_lTopID;
}

bool FolderMessageItem::SetTopID(long lTopID)
{
   SET_INT(m_lTopID, lTopID)
} */

int FolderMessageItem::GetVoteType()
{
   int iVoteType = 0;

   if(mask(GetType(), MSGTYPE_VOTE) == false)
   {
      return 0;
   }

   m_pEDF->Root();

   if(m_pEDF->Child("votes") == false)
   {
      m_pEDF->Add("votes");
   }

   m_pEDF->GetChild("votetype", &iVoteType);

   m_pEDF->Parent();

   return iVoteType;
}

/* bool FolderMessageItem::SetVoteType(int iVoteType, int iMinValue, int iMaxValue)
{
   int iVoteEDF = 0;
   bool bReturn = false;

   if(mask(GetType(), MSGTYPE_VOTE) == false)
   {
      return false;
   }

   m_pEDF->Root();

   if(m_pEDF->Child("votes") == false)
   {
      m_pEDF->Add("votes");
   }

   m_pEDF->GetChild("votetype", &iVoteEDF);
   debug("FolderMessageItem::SetVoteType %d -vs- %d\n", iVoteEDF, iVoteType);
   if(iVoteType != iVoteEDF)
   {
      m_pEDF->SetChild("votetype", iVoteType);
      m_bChanged = true;
      bReturn = true;
   }

   if(mask(iVoteType, VOTE_INTVALUES) == true)
   {
      m_pEDF->SetChild("minvalue", iMinValue);
      m_pEDF->SetChild("maxvalue", iMaxValue);
   }

   m_pEDF->Parent();

   return bReturn;
} */

bool FolderMessageItem::SetVoteType(int iVoteType)
{
   int iVoteEDF = 0;
   bool bReturn = false;

   if(mask(GetType(), MSGTYPE_VOTE) == false)
   {
      return false;
   }

   m_pEDF->Root();

   if(m_pEDF->Child("votes") == false)
   {
      m_pEDF->Add("votes");
   }

   m_pEDF->GetChild("votetype", &iVoteEDF);
   debug("FolderMessageItem::SetVoteType %d -vs- %d\n", iVoteEDF, iVoteType);
   if(iVoteType != iVoteEDF)
   {
      m_pEDF->SetChild("votetype", iVoteType);
      SetChanged(true);
      bReturn = true;
   }

   m_pEDF->Parent();

   return bReturn;
}

bool FolderMessageItem::SetVoteMinValue(int iValue)
{
   return SetVoteValue("minvalue", VOTE_INTVALUES, iValue, 0);
}

bool FolderMessageItem::SetVoteMaxValue(int iValue)
{
   return SetVoteValue("maxvalue", VOTE_INTVALUES, iValue, 0);
}

bool FolderMessageItem::SetVoteMinValue(double dValue)
{
   return SetVoteValue("minvalue", VOTE_FLOATVALUES, 0, dValue);
}

bool FolderMessageItem::SetVoteMaxValue(double dValue)
{
   return SetVoteValue("maxvalue", VOTE_FLOATVALUES, 0, dValue);
}

bool FolderMessageItem::AddVote(bytes *pText)
{
   int iMaxID = 0;

   if(mask(GetType(), MSGTYPE_VOTE) == false || pText == NULL || pText->Length() == 0)
   {
      return false;
   }

   m_pEDF->Root();

   if(m_pEDF->Child("votes") == false)
   {
      m_pEDF->Add("votes");
   }

   iMaxID = EDFMax(m_pEDF, "vote", false);

   iMaxID++;

   m_pEDF->Add("vote", iMaxID);
   m_pEDF->AddChild("text", pText);
   m_pEDF->Parent();

   m_pEDF->Parent();

   SetChanged(true);

   return true;
}

int FolderMessageItem::GetVoteUser(long lUserID)
{
   int iVoteType = 0, iVoteID = -1;
   bool bLoop = false;

   if(mask(GetType(), MSGTYPE_VOTE) == false)
   {
      debug("FolderMessageItem::GetVoteUser not a vote\n");
      return -1;
   }

   m_pEDF->Root();

   if(m_pEDF->Child("votes") == false)
   {
      debug("FolderMessageItem::GetVoteUser no vote section\n");
      return -1;
   }

   // EDFPrint("FolderMessageItem::GetVoteUser whole EDF", m_pEDF);

   m_pEDF->GetChild("votetype", &iVoteType);

   if(mask(iVoteType, VOTE_NAMED) == true)
   {
      bLoop = m_pEDF->Child("vote");
      while(bLoop == true && iVoteID == -1)
      {
         if(EDFFind(m_pEDF, "userid", lUserID, false) == true)
         {
            m_pEDF->Parent();
            m_pEDF->Get(NULL, &iVoteID);
         }
         else
         {
            bLoop = m_pEDF->Next("vote");
            if(bLoop == false)
            {
               m_pEDF->Parent();
            }
         }
      }
   }
   else
   {
      if(EDFFind(m_pEDF, "userid", lUserID, false) == true)
      {
         iVoteID = 0;
      }
   }

   m_pEDF->Parent();

   debug("FolderMessageItem::GetVoteUser exit %d\n", iVoteID);
   return iVoteID;
}

bool FolderMessageItem::SetVoteUser(long lUserID, int iVoteID, char *szVoteValue, int iVoteValue, double dVoteValue, int iVoteType)
{
   STACKTRACE
   int iVoteEDF = 0, iNumVotes = 0, iMinValue = 0, iMaxValue = 0;
   double dMinValue = 0, dMaxValue = 0;
   bool bLoop = true;
   char *szText = NULL;
   char szTemp[100];
   EDFElement *pVote = NULL;

   if(mask(GetType(), MSGTYPE_VOTE) == false)
   {
      debug("FolderMessageItem::SetVoteUser not a vote\n");
      return false;
   }

   m_pEDF->Root();

   if(m_pEDF->Child("votes") == false)
   {
      debug("FolderMessageItem::SetVoteUser no vote section\n");
      return false;
   }

   debug("FolderMessageItem::SetVoteUser entry %ld %d %s %d %g %d:\n", lUserID, iVoteID, szVoteValue, iVoteValue, dVoteValue, iVoteType);
   EDFParser::debugPrint(m_pEDF, EDFElement::EL_CURR | EDFElement::PR_SPACE);

   m_pEDF->GetChild("votetype", &iVoteEDF);

   if(mask(iVoteEDF, VOTE_CLOSED) == true)
   {
      m_pEDF->Root();

      debug("FolderMessageItem::SetVoteUser exit false, already closed\n");
      return false;
   }

   if(iVoteID > 0)
   {
      if(EDFFind(m_pEDF, "vote", iVoteID, false) == true)
      {
         pVote = m_pEDF->GetCurr();
         m_pEDF->Parent();
      }
      else
      {
         m_pEDF->Root();

         debug("FolderMessageItem::SetVoteUser exit fales, vote ID invlalid\n");
         return false;
      }
   }
   else
   {
      // Vote value

      if(mask(iVoteEDF, VOTE_INTVALUES) == true)
      {
         if(m_pEDF->GetChild("minvalue", &iMinValue) == true && iVoteValue < iMinValue)
         {
            m_pEDF->Root();

            debug("FolderMessageItem::SetVoteUser exit fales, value less than %d\n", iMinValue);
            return false;
         }
         else if(m_pEDF->GetChild("maxvalue", &iMaxValue) == true && iVoteValue > iMaxValue)
         {
            m_pEDF->Root();

            debug("FolderMessageItem::SetVoteUser exit false, value more %d\n", iMaxValue);
            return false;
         }
      }
      else if(mask(iVoteEDF, VOTE_PERCENT) == true && (iVoteValue < 0 || iVoteValue > 100))
      {
         m_pEDF->Root();

         debug("FolderMessageItem::SetVoteUser exit false, percent out of range\n");
         return false;
      }
      else if(mask(iVoteEDF, VOTE_FLOATVALUES) == true)
      {
         if(m_pEDF->GetChild("minvalue", &dMinValue) == true && dVoteValue < dMinValue)
         {
            m_pEDF->Root();

            debug("FolderMessageItem::SetVoteUser exit fales, value less than %g\n", dMinValue);
            return false;
         }
         else if(m_pEDF->GetChild("maxvalue", &dMaxValue) == true && dVoteValue > dMaxValue)
         {
            m_pEDF->Root();

            debug("FolderMessageItem::SetVoteUser exit false, value more %g\n", dMaxValue);
            return false;
         }
      }
      else if(mask(iVoteEDF, VOTE_FLOATPERCENT) == true && (dVoteValue < 0 || dVoteValue > 100))
      {
         m_pEDF->Root();

         debug("FolderMessageItem::SetVoteUser exit false, percent out of range\n");
         return false;
      }
      else if(mask(iVoteEDF, VOTE_STRVALUES) == true && (szVoteValue == NULL || strlen(szVoteValue) == 0))
      {
         m_pEDF->Root();

         debug("FolderMessageItem::SetVoteUser exit false, null / empty string\n");
         return false;
      }

      bLoop = m_pEDF->Child("vote");
      while(bLoop == true)
      {
         m_pEDF->GetChild("text", &szText);
         if((IS_INT_VOTE(iVoteEDF) == true && iVoteValue == atoi(szText)) ||
            (IS_FLOAT_VOTE(iVoteEDF) == true && dVoteValue == atof(szText)) ||
            (mask(iVoteEDF, VOTE_STRVALUES) == true && strcmp(szVoteValue, szText) == 0))
         {
            bLoop = false;
            pVote = m_pEDF->GetCurr();
            m_pEDF->Parent();
         }
         else
         {
            bLoop = m_pEDF->Next("vote");
            if(bLoop == false)
            {
               m_pEDF->Parent();
            }
         }
         delete[] szText;
      }
   }

   // printf("FolderMessageItem::SetVoteUser %ld %d %d, %d\n", lUserID, iVoteID, iVoteType, iVoteEDF);

   EDFParser::debugPrint("FolderMessageItem::SetVoteUser user checkpoint", m_pEDF, EDFElement::EL_CURR + EDFElement::PR_SPACE);

   // Has use already voted?
   if(mask(iVoteEDF, VOTE_NAMED) == true)
   {
      // Check all options
      bLoop = m_pEDF->Child("vote");
      while(bLoop == true)
      {
         EDFParser::debugPrint("FolderMessageItem::SetVoteUser named check point", m_pEDF, EDFElement::EL_CURR);
         if(EDFFind(m_pEDF, "userid", lUserID, false) == true)
         {
            if(mask(iVoteEDF, VOTE_CHANGE) == true)
            {
               debug("FolderMessageItem::SetVoteUser removing vote\n");

               m_pEDF->Delete();

               bLoop = false;
            }
            else
            {
               m_pEDF->Root();

               debug("MesageItem::SetVoteUser exit false, already voted\n");
               return false;
            }
         }

         if(bLoop == true)
         {
            bLoop = m_pEDF->Next("vote");
            if(bLoop == false)
            {
               m_pEDF->Parent();
            }
         }
      }
   }
   else
   {
      if(EDFFind(m_pEDF, "userid", lUserID, false) == true)
      {
         m_pEDF->Root();

         debug("MesageItem::SetVoteUser exit false, already voted\n");
         return false;
      }
   }

   if(pVote != NULL)
   {
      // Reset field add point
      m_pEDF->SetCurr(pVote);
   }
   else
   {
      iVoteID = EDFMax(m_pEDF, "vote");
      iVoteID++;

      m_pEDF->Add("vote", iVoteID);
      if(IS_INT_VOTE(iVoteEDF) == true)
      {
         sprintf(szTemp, "%d", iVoteValue);
         m_pEDF->AddChild("text", szTemp);
      }
      else if(IS_FLOAT_VOTE(iVoteEDF) == true)
      {
         sprintf(szTemp, "%g", dVoteValue);
         m_pEDF->AddChild("text", szTemp);
      }
      else
      {
         m_pEDF->AddChild("text", szVoteValue);
      }
   }

   if(mask(iVoteEDF, VOTE_NAMED) == false)
   {
      // Add anonymous vote to total
      m_pEDF->GetChild("numvotes", &iNumVotes);
      m_pEDF->SetChild("numvotes", iNumVotes + 1);
      m_pEDF->Parent();
   }

   m_pEDF->AddChild("userid", lUserID);

   // Putting the users in numerical order to prevent mapping of data for an anonymous own option vote
   m_pEDF->Sort("userid");

   m_pEDF->Root();

   SetChanged(true);

   EDFParser::debugPrint("FolderMessageItem::SetVoteUser exit true", m_pEDF);
   return true;
}

long FolderMessageItem::GetVoteClose()
{
   long lReturn = -1;

   if(mask(GetType(), MSGTYPE_VOTE) == false)
   {
      debug("FolderMessageItem::GetVoteClose not a vote\n");
      return false;
   }

   m_pEDF->Root();

   if(m_pEDF->Child("votes") == true)
   {
      m_pEDF->GetChild("voteclose", &lReturn);
      m_pEDF->Parent();
   }

   return lReturn;
}

bool FolderMessageItem::SetVoteClose(long lTime)
{
   if(mask(GetType(), MSGTYPE_VOTE) == false)
   {
      debug("FolderMessageItem::GetVoteClose not a vote\n");
      return false;
   }

   m_pEDF->Root();

   if(m_pEDF->Child("votes") == false)
   {
      m_pEDF->Add("votes");
   }

   if(lTime != -1)
   {
      m_pEDF->SetChild("voteclose", lTime);
   }
   else
   {
      m_pEDF->DeleteChild("voteclose");
   }

   m_pEDF->Parent();

   SetChanged(true);

   return true;
}

/* MessageTreeItem *FolderMessageItem::GetReply()
{
   return m_pReply;
}

bool FolderMessageItem::SetReply(MessageTreeItem *pReply)
{
   m_pReply = pReply;
   if(m_pReply != NULL)
   {
      m_lReplyID = m_pReply->GetID();
   }
   else
   {
      m_lReplyID = -1;
   }

   m_bChanged = true;

   return true;
} */

long FolderMessageItem::GetReplyID()
{
   /* if(m_pReply != NULL)
   {
      return m_pReply->GetID();
   } */

   return m_lReplyID;
}

bool FolderMessageItem::SetReplyID(long lReplyID)
{
   /* if(m_pReply != NULL)
   {
      debug("FolderMessageItem::SetReplyID exit false, folder %p %ld\n", m_pReply, m_pReply->GetID());
      return false;
   } */

   SET_INT(m_lReplyID, lReplyID)
}

long FolderMessageItem::GetExpire()
{
   return m_lExpire;
}

bool FolderMessageItem::SetExpire(long lExpire)
{
   SET_INT(m_lExpire, lExpire)
}

FolderMessageItem *FolderMessageItem::NextRow(DBTable *pTable, FolderMessageItem *pParent, MessageTreeItem *pTree)
{
   STACKTRACE

   if(m_pInit == NULL)
   {
      m_pInit = new FolderMessageItem((long)0, NULL, NULL);
      // debug("FolderMessageItem::NextRow class %s\n", m_pInit->GetClass());
   }

   m_pInit->InitFields(pTable);

   if(pTable->NextRow() == false)
   {
      return NULL;
   }

   return new FolderMessageItem(pTable, pParent, pTree);
}

// IsEDFField: Field name checker
bool FolderMessageItem::IsEDFField(char *szName)
{
   STACKTRACE
   bool bReturn = false;

   // printf("FolderMessageItem::IsEDFField %s\n", szName);

   // Fields that have member storage
   if(stricmp(szName, "subject") == 0)
   {
      return false;
   }

   // Fields that should not be stored
   if(stricmp(szName, "parentlink") == 0 ||
      stricmp(szName, "childlink") == 0)
   {
      return false;
   }

   // return TreeNodeItem<FolderMessageItem *>::IsEDFField(szName, true);

   bReturn = MessageItem::IsEDFField(szName);
   // printf("MessageItem::IsEDFField %s, %s\n", szName, BoolStr(bReturn));
   return bReturn;
}

// WriteFields: Output to EDF
bool FolderMessageItem::WriteFields(EDF *pEDF, int iLevel)
{
   STACKTRACE
   // bytes *pFromName = NULL;

   debug(DEBUGLEVEL_DEBUG, "FolderMessageItem::WriteFields %p %d, %p %p\n", pEDF, iLevel, GetParent(), GetTree());

   MessageItem::WriteFields(pEDF, iLevel);

   if(iLevel == -1)
   {
      iLevel = EDFITEMWRITE_HIDDEN + EDFITEMWRITE_ADMIN + MESSAGEITEMWRITE_DETAILS + MESSAGEITEMWRITE_TEXT + MESSAGEITEMWRITE_ATTDATA;
   }

   // printf("FolderMessageItem::WriteFields date %ld\n", m_lDate);

   /* if(mask(iLevel, MESSAGEWRITE_HIDDEN) == true)
   {
      pEDF->AddChild("topid", m_lTopID);
   } */

   debug(DEBUGLEVEL_DEBUG, "FolderMessageItem::WriteFields exit true\n");
   return true;
}

bool FolderMessageItem::WriteEDF(EDF *pEDF, int iLevel)
{
   int iVoteType = 0, iUserID = 0, iNumVotes = 0, iTotalVotes = 0, iVoteID = 0;
   bool bLoop = false;

   // EDFPrint("FolderMessageItem::WriteEDF", m_pEDF);

   MessageItem::WriteEDF(pEDF, iLevel);

   if(iLevel == -1)
   {
      iLevel = EDFITEMWRITE_HIDDEN + EDFITEMWRITE_ADMIN + MESSAGEITEMWRITE_DETAILS + MESSAGEITEMWRITE_TEXT + MESSAGEITEMWRITE_ATTDATA;
   }

   if(mask(iLevel, MESSAGEITEMWRITE_TEXT) == true)
   {
      // bytesprint("FolderMessageItem::WriteEDF toname", m_pToName);

      WriteDetail(pEDF, "delete", iLevel);
      WriteDetail(pEDF, "edit", iLevel);
      WriteDetail(pEDF, "move", iLevel);

      if(mask(GetType(), MSGTYPE_VOTE) == true && m_pEDF->Child("votes") == true)
      {
         EDFParser::debugPrint("FolderMessageItem::WriteEDF vote section", m_pEDF, EDFElement::PR_SPACE);

         m_pEDF->GetChild("votetype", &iVoteType);

         // printf("FolderMessageItem::WriteFields votes section %d:\n", iVoteType);
         // EDFPrint(m_pEDF, EDFElement::EL_CURR | EDFElement::PR_SPACE);

         pEDF->Add("votes");

         if(iVoteType > 0)
         {
            pEDF->AddChild("votetype", iVoteType);
         }

         pEDF->AddChild(m_pEDF, "minvalue");
         pEDF->AddChild(m_pEDF, "maxvalue");

         if(mask(iLevel, EDFITEMWRITE_ADMIN) == true || IS_VALUE_VOTE(iVoteType) == false || mask(iVoteType, VOTE_PUBLIC) == true)
         {
            bLoop = m_pEDF->Child("vote");
            while(bLoop == true)
            {
               m_pEDF->Get(NULL, &iVoteID);

               pEDF->Add("vote", iVoteID);
               pEDF->AddChild(m_pEDF, "text");

               if(mask(iLevel, EDFITEMWRITE_ADMIN) == true || mask(iVoteType, VOTE_PUBLIC) == true)
               {
                  iNumVotes = 0;

                  if(mask(iVoteType, VOTE_NAMED) == false)
                  {
                     m_pEDF->GetChild("numvotes", &iNumVotes);
                  }
                  else
                  {
                     iNumVotes = m_pEDF->Children("userid");

                     if(mask(iLevel, EDFITEMWRITE_ADMIN) == true || mask(iLevel, FOLDERMESSAGEITEMWRITE_SELF) == true)
                     {
                        bLoop = m_pEDF->Child("userid");
                        while(bLoop == true)
                        {
                           m_pEDF->Get(NULL, &iUserID);
                           pEDF->AddChild("userid", iUserID);

                           bLoop = m_pEDF->Next("userid");
                           if(bLoop == false)
                           {
                              m_pEDF->Parent();
                           }
                        }
                     }
                  }

                  if(iNumVotes > 0)
                  {
                     pEDF->AddChild("numvotes", iNumVotes);
                     iTotalVotes += iNumVotes;
                  }
               }

               pEDF->Parent();

               bLoop = m_pEDF->Next("vote");
               if(bLoop == false)
               {
                  m_pEDF->Parent();
               }
            }
         }

         if(mask(iLevel, EDFITEMWRITE_ADMIN) == true && mask(iVoteType, VOTE_NAMED) == false)
         {
            bLoop = m_pEDF->Child("userid");
            while(bLoop == true)
            {
               m_pEDF->Get(NULL, &iUserID);
               pEDF->AddChild("userid", iUserID);

               bLoop = m_pEDF->Next("userid");
               if(bLoop == false)
               {
                  m_pEDF->Parent();
               }
            }
         }

         if(iTotalVotes > 0)
         {
            pEDF->AddChild("numvotes", iTotalVotes);
         }

         pEDF->Parent();

         m_pEDF->Parent();
      }
   }

   return true;
}

bool FolderMessageItem::WriteFields(DBTable *pTable, EDF *pEDF)
{
   STACKTRACE

   // EDFPrint("FolderMessageItem::WriteFields", m_pEDF);

   MessageItem::WriteFields(pTable, pEDF);

   // pTable->SetField(m_lTopID);

   return true;
}

bool FolderMessageItem::ReadFields(DBTable *pTable)
{
   STACKTRACE

   MessageItem::ReadFields(pTable);

   // topid, subject
   // pTable->BindColumnInt();

   return true;
}

char *FolderMessageItem::TableName()
{
   return FMI_TABLENAME;
}

char *FolderMessageItem::TreeName()
{
   return "folder";
}

void FolderMessageItem::init(MessageTreeItem *pTree)
{
   // Top level fields

   // m_lTopID = -1;

   // m_pReply = NULL;
   m_lReplyID = -1;

   m_lExpire = -1;
}

void FolderMessageItem::setup()
{
   int iDate = -1, iFromID = -1, iTreeID = -1, iUserID = -1;
   bool bMove = false;
   // char *szFromName = NULL;
   bytes *pText = NULL;
   EDFElement *pVotes = NULL, *pElement = NULL;

   // debug("FolderMessageItem::setup, type %d\n", GetType());

   // Old style data fixing
   // bCopy = m_pEDF->GetCopy(false);

   while(m_pEDF->Child("note") == true)
   {
      EDFParser::debugPrint("FolderMessageItem::FolderMessageItem annotation", m_pEDF, EDFElement::EL_CURR | EDFElement::PR_SPACE);

      m_pEDF->Get(NULL, &iDate);
      m_pEDF->GetChild("fromid", &iFromID);
      m_pEDF->GetChild("text", &pText);
      m_pEDF->Delete();

      AddAnnotation(pText, iFromID, iDate);

      delete pText;

      SetChanged(true);
   }

   while(m_pEDF->Child("movelink") == true)
   {
      EDFParser::debugPrint("FolderMessageItem::FolderMessageItem move", m_pEDF, EDFElement::EL_CURR | EDFElement::PR_SPACE);

      m_pEDF->Get(NULL, &iTreeID);
      m_pEDF->GetChild("userid", &iUserID);
      m_pEDF->Delete();

      m_pEDF->Add("move");
      m_pEDF->AddChild("folderid", iTreeID);
      m_pEDF->AddChild("userid", iUserID);
      m_pEDF->Parent();

      SetChanged(true);
   }

   if(m_pEDF->IsChild("msgtype") == false && m_pEDF->Children("vote") > 0)
   {
      // Old style vote
      EDFParser::debugPrint("FolderMessageItem::setup votes section", m_pEDF, EDFElement::EL_CURR | EDFElement::PR_SPACE);

      SetType(GetType() | MSGTYPE_VOTE);

      m_pEDF->SetChild("votetype", VOTE_NAMED);

      EDFParser::debugPrint("FolderMessageItem::setup pre-edit voting", m_pEDF);

      m_pEDF->Add("votes");

      pVotes = m_pEDF->GetCurr();

      m_pEDF->Root();

      while(m_pEDF->Child("vote") == true)
      {
         pElement = m_pEDF->GetCurr();

         bMove = pVotes->moveFrom(pElement);
         debug("FolderMessageItem::setup move %s\n", BoolStr(bMove));

         m_pEDF->Root();
      }

      EDFParser::debugPrint("FolderMessageItem::setup post-edit voting", m_pEDF);

      SetChanged(true);
   }

   // m_pEDF->GetCopy(bCopy);
}

bool FolderMessageItem::AddMove(long lUserID, long lDate)
{
   AddDetail("move", lUserID, lDate, false);

   m_pEDF->AddChild("folderid", GetTreeID());

   m_pEDF->Parent();

   return true;
}

bool FolderMessageItem::AddAnnotation(bytes *pText, long lFromID, long lDate)
{
   if(AddAttachment(false, NULL, MSGATT_ANNOTATION, NULL, NULL, NULL, lFromID, lDate) == false)
   {
      return false;
   }

   EDFSetStr(m_pEDF, "text", pText, UA_SHORTMSG_LEN);

   m_pEDF->Parent();

   return true;
}

bool FolderMessageItem::IsAnnotation(int iFromID)
{
   STACKTRACE
   int iFromEDF = -1;
   bool bLoop = false, bReturn = false;
   char *szContentType = NULL;

   bLoop = m_pEDF->Child("attachment");
   while(bLoop == true && bReturn == false)
   {
      szContentType = NULL;

      if(m_pEDF->GetChild("content-type", &szContentType) == true && szContentType != NULL && stricmp(szContentType, MSGATT_ANNOTATION) == 0)
      {
         m_pEDF->GetChild("fromid", &iFromEDF);
         if(iFromID == iFromEDF)
         {
            bReturn = true;
            m_pEDF->Parent();
         }
      }

      delete[] szContentType;

      if(bReturn == false)
      {
         bLoop = m_pEDF->Next("attachment");
         if(bLoop == false)
         {
            m_pEDF->Parent();
         }
      }
   }

   return bReturn;
}

bool FolderMessageItem::AddLink(bytes *pText, long lMessageID, long lFromID, long lDate)
{
   if(AddAttachment(false, NULL, MSGATT_LINK, NULL, NULL, NULL, lFromID, lDate) == false)
   {
      return false;
   }

   EDFSetStr(m_pEDF, "text", pText, UA_SHORTMSG_LEN);
   m_pEDF->AddChild("messageid", lMessageID);

   m_pEDF->Parent();

   return true;
}

bool FolderMessageItem::SetVoteValue(const char *szField, int iVoteType, int iValue, double dValue)
{
   debug("FolderMessageItem::SetVoteValue entry %s %d %d %g\n", szField, iVoteType, iValue, dValue);

   if((iVoteType == VOTE_INTVALUES && mask(GetVoteType(), VOTE_INTVALUES) == false) ||
      (iVoteType == VOTE_FLOATVALUES && mask(GetVoteType(), VOTE_FLOATVALUES) == false))
   {
      debug("FolderMessageItem::SetVoteValue exit false, wrong vote type\n");
      return false;
   }

   m_pEDF->Root();

   if(m_pEDF->Child("votes") == false)
   {
      m_pEDF->Add("votes");
   }

   if(iVoteType == VOTE_INTVALUES)
   {
      m_pEDF->SetChild(szField, iValue);
   }
   else if(iVoteType == VOTE_FLOATVALUES)
   {
      m_pEDF->SetChild(szField, dValue);
   }

   SetChanged(true);

   m_pEDF->Parent();

   debug("FolderMessageItem::SetVoteValue exit true\n");
   return true;
}
