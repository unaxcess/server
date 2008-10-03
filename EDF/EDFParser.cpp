/*
** EDF - Encapsulated Data Format
** (c) 1998 Michael Wood (mike@compsoc.man.ac.uk)
**
** EDFParser.cpp: Read / write convenience functions for standard format EDF
*/

#include "stdafx.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "EDF.h"

#include "EDFParser.h"

long EDFParser::Read(EDF *pEDF, const char *szData, int iProgress, int iOptions)
{
	STACKTRACE
   return Read(pEDF, (const byte *)szData, strlen(szData));
}

long EDFParser::Read(EDF *pEDF, bytes *pData, int iProgress, int iOptions)
{
	STACKTRACE
   bytes *pTemp = (bytes *)pData;

   if(pTemp == NULL)
   {
      return 0;
   }

   return Read(pEDF, pTemp->Data(false), pTemp->Length());
}

#define VALID_CHAR \
(pPos < pStop)

#define IS_SPACE \
(*pPos == ' ' || *pPos == '\r' || *pPos == '\n' || *pPos == '\t' || *pPos == '\0')

#define PRINT_POS \
isprint(*pPos) || *pPos == ' ' ? *pPos : '\0'

#define WHITE_SPACE(szParse) \
/*printf("EDFParser::Read white space %s %d '%c' [%d]\n", szParse != NULL ? szParse : "", pPos - pData, PRINT_POS, *pPos);*/\
while(VALID_CHAR && IS_SPACE)\
{\
	if(*pPos == '\n')\
	{\
		pLine = pPos + 1;\
		lLineNum++;\
	}\
	pPos++;\
}

#define PARSE_SIZE 10

#define PARSE_ERROR(szError, lError)\
{\
   debug(DEBUGLEVEL_DEBUG, "EDFParser::Read (%d, %ld, %d) '%c' [%d] %s\n", pPos - pData, lLineNum, pPos - pLine, PRINT_POS, *pPos, szError);\
}\
lReturn = lError;\
bLoop = false;\
bValid = false;

#define BYTES_VALUE \
dBytesVal = gettick(); \
pPos++; \
pStart = pPos; \
bLiteral = false; \
lValueLiterals = 0; \
while(VALID_CHAR && !(bLiteral == false && *pPos == '"')) \
{ \
	if(bLiteral == true) \
	{ \
      if(*pPos == '\\' || *pPos == '"') \
		{ \
			lValueLiterals++; \
		} \
		bLiteral = false; \
	} \
	else if(*pPos == '\\') \
	{ \
		bLiteral = true; \
	} \
	if(*pPos == '\n') \
	{ \
		pLine = pPos + 1; \
		lLineNum++; \
	} \
	pPos++; \
} \
lBytesVal += tickdiff(dBytesVal);

long EDFParser::Read(EDF *pEDF, const byte *pData, const long lDataLen, const int iProgress, const int iOptions)
{
	STACKTRACE
	long lReturn = 0, lLineNum = 1, lValueLiterals = 0;//, lValueLen = 0;
   long lNew = 0, lSpace = 0, lSet = 0, lTick = 0, lName = 0, lBytesVal = 0, lValid = 0;
	int iDepth = 0, iNumElements = 0;//, iType = 0;
	bool bLoop = true, bLiteral = false, bParent = false, bValid = true, bSingleton = false, bDepth = true, bRootSet = false, bFloat = false;
	double dTick = gettick(), dBytesVal = 0, dNew = 0, dSpace = 0, dSet = 0, dName = 0, dValid = 0;
	char *szName = NULL, *szParse = NULL;
	byte *pPos = (byte *)pData, *pStop = pPos + lDataLen, *pStart = pPos, *pLine = pPos, *pParse = NULL, *pProgress = pPos;//, *pValue = NULL;
	EDFElement *pElement = NULL, *pRoot = NULL, *pCurr = NULL;

	// if(m_bDebug == true)
	{
		debug(DEBUGLEVEL_DEBUG, "EDFParser::Read entry %p %ld %d\n", pData, lDataLen, iProgress);
	   // memprint("EDFParser::Read data", pData, lDataLen);
   }

   if(pData == NULL || lDataLen < 5)
   {
      // Has to be at least <></>
      return 0;
   }

	while(bLoop == true)
	{
      STACKTRACEUPDATE

      if(iProgress != -1 && pPos > pProgress + iProgress)
      {
         debug(DEBUGLEVEL_DEBUG, "EDFParser::Read progress point %d after %ld ms\n", pPos - pData, tickdiff(dTick));
         memprint(DEBUGLEVEL_DEBUG, debugfile(), NULL, pPos, 40, false);
         debug(DEBUGLEVEL_DEBUG, "\n");
         pProgress = pPos;
      }

      dSpace = gettick();

		while(VALID_CHAR && *pPos != '<')
      {
			if(*pPos == '\n')
			{
				pLine = pPos;
				lLineNum++;
			}
         pPos++;
      }

		if(!VALID_CHAR)
		{
         lReturn = -2;
			bLoop = false;
		}
		else
		{
         STACKTRACEUPDATE

			// Start of element
			pPos++;

			WHITE_SPACE("before name")

			if(!VALID_CHAR)
         {
            PARSE_ERROR("EOD before name", -1)
         }
         else if(!(isalpha(*pPos) || *pPos == '=' || *pPos == '>' || *pPos == '/'))
			{
				PARSE_ERROR("before name", 0)
			}
			else
			{
            STACKTRACEUPDATE

				// Element name
				if(*pPos == '/')
				{
					bParent = true;
					pPos++;
				}
				else
				{
					bParent = false;
				}

            lSpace += tickdiff(dSpace);

            dName = gettick();

				pStart = pPos;
            // pPos++; // Skip over first chacter
				while(VALID_CHAR && (isalnum(*pPos) || *pPos == '-'))
				{
					pPos++;
				}

				if(!VALID_CHAR)
				{
					PARSE_ERROR("EOD during name", -1)
				}
				else
				{
               STACKTRACEUPDATE

               lName += tickdiff(dName);

					// Create element
					// szName = (char *)memmk(pStart, pPos - pStart);
               dNew = gettick();
               NEWCOPY(szName, pStart, pPos - pStart, char, byte);
               lNew += tickdiff(dNew);
               // printf("EDFParser::Read name '%s'\n", szName);
               dValid = gettick();
					if(EDFElement::validName(szName) == false)
					{
						PARSE_ERROR("at invalid name", 0)
					}
					else
					{
                  STACKTRACEUPDATE

                  lValid += tickdiff(dValid);
						if(bParent == false)
						{
                     bRootSet = true;

                     iNumElements++;
                     dNew = gettick();
                     STACKTRACEUPDATE
                     // debug(DEBUGLEVEL_DEBUG, "EDFParser::Read new element %p '%s' '%s'\n", m_pCurr, m_pCurr != NULL ? m_pCurr->getName(false) : NULL, szName);
                     pElement = new EDFElement(pCurr, szName);
                     STACKTRACEUPDATE
                     lNew += tickdiff(dNew);
                     // printf("EDFParser::Read new element %p, parent %p\n", pElement, m_pCurr);
                     if(pRoot == NULL)
                     {
                        pRoot = pElement;
                     }
                     pCurr = pElement;
                     // m_pCurr->print("EDFParser::Read element");
						}

                  STACKTRACEUPDATE

                  dSpace = gettick();

						if(IS_SPACE)
						{
							// After name
							WHITE_SPACE("after name")
						}

						if(!VALID_CHAR)
						{
							PARSE_ERROR("EOD after name", -1)
						}
						else
						{
                     STACKTRACEUPDATE

                     lSpace += tickdiff(dSpace);
							if(bParent == false)
							{
								if(*pPos == '=')
								{
									// Value
									pPos++;

                           dSpace = gettick();
									WHITE_SPACE("before value")
                           lSpace += tickdiff(dSpace);

									if(!VALID_CHAR)
									{
										PARSE_ERROR("EOD before value", -1)
									}
									else
									{
                              STACKTRACEUPDATE

										if(*pPos == '"')
										{
											// Byte value
											BYTES_VALUE

											if(!VALID_CHAR)
                                 {
                                    PARSE_ERROR("EOD during byte value", -1)
                                 }
                                 /* else if(*pPos != '"')
											{
												PARSE_ERROR("during byte value", 0)
											} */
											else
											{
												// Set byte value
                                    // printf("EDFParser::Read set byte value\n");
                                    dSet = gettick();
												pCurr->setValue(pStart, pPos - pStart, true);
                                    lSet += tickdiff(dSet);

												pPos++;
											}

											szParse = "after string value";
										}
										else if(isdigit(*pPos) || *pPos == '-')
										{
                                 STACKTRACEUPDATE

											// Number value
                                 pStart = pPos;
                                 bFloat = false;

                                 if(*pPos == '-')
											{
												pPos++;
											}

											while(VALID_CHAR && isdigit(*pPos))
											{
												pPos++;
                                    if(VALID_CHAR)
                                    {
                                       if(bFloat == false && *pPos == '.')
                                       {
                                          // debug("EDFParser::Read float value (decimal point)\n");

                                          bFloat = true;
                                          pPos++;
                                       }
                                       else if(*pPos == 'e')
                                       {
                                          // debug("EDFParser::Read float value (exponent)\n");

                                          bFloat = true;
                                          pPos++;

                                          if(VALID_CHAR && (*pPos == '+' || *pPos == '-'))
                                          {
                                             // debug("EDFParser::Read float value (exponent sign)\n");

                                             pPos++;
                                          }
                                       }
                                    }
											}

											if(!VALID_CHAR)
											{
												PARSE_ERROR("EOD during number value", -1)
											}
											else
											{
                                    STACKTRACEUPDATE

                                    // memprint("EDFParser::Read number value", pStart, pPos - pStart);

                                    if(bFloat == true)
                                    {
                                       // memprint("EDFParser::Read parse float", pStart, pPos - pStart);

                                       pCurr->setValue(atof((char *)pStart));
                                    }
                                    else
                                    {
                                       pCurr->setValue(atol((char *)pStart));
                                    }
                                 }

											szParse = "EOD after number value";
										}
									}
								}
								else
								{
									szParse = "EOD after name";
								}

                        if(VALID_CHAR)
                        {
                           dSpace = gettick();
								   WHITE_SPACE("before end of element")
                           lSpace += tickdiff(dSpace);

								   if(!VALID_CHAR)
								   {
									   PARSE_ERROR(szParse, -1)
								   }
                        }
							}
						}

                  // pCurr->print("EDFParser::Read element");

						if(VALID_CHAR)
						{
							if(bParent == false)
							{
								if(*pPos == '/')
								{
									bSingleton = true;
									pPos++;
								}
                        else
                        {
                           bSingleton = false;
                        }

                        dSpace = gettick();
								WHITE_SPACE("before end of element")
                        lSpace += tickdiff(dSpace);
							}

							if(!VALID_CHAR)
                     {
                        PARSE_ERROR("EOD before end of element", -1)
                     }
                     else if(*pPos != '>')
							{
								PARSE_ERROR("before end of element", 0)
							}
							else
							{
                        // printf("EDFParser::Read move to parent %s && (%s || %s)\n", BoolStr(bRootSet), BoolStr(bSingleton), BoolStr(bParent));
								if(bRootSet == true && (bSingleton == true || bParent == true))
								{
                           if(pCurr->parent() != NULL)
                           {
                              bDepth = true;
                              pCurr = pCurr->parent();
                           }
                           else
                           {
                              bDepth = false;
                           }
                           bLoop = bDepth;
                           /* if(m_bDebug == true)
                           {
                              debug("EDFParser::Read parent l=%s d=%d(%s) n=%s\n", BoolStr(bLoop), m_iDepth, BoolStr(bDepth), m_pCurr != NULL ? m_pCurr->getName(false) : "");
                           } */
								}
                        else if(bParent == true)
                        {
                           bLoop = false;
                           bValid = false;
                        }
								pPos++;
							}
						}
					}
               delete[] szName;
				}
			}
		}

      /* if(m_bDebug == true)
      {
         debug("EDFParser::Read loop, s='%s' p=%d d=%d\n", pPos, pPos - pData, m_iDepth);
      } */
   }

   // printf("EDFParser::Read %d elements\n", iNumElements);

   /* if(lReturn < 0)
   {
      debug("EDFParser::Read exit %d\n", lReturn);
      return lReturn;
   } */

   // iDepth = Depth();
	if(lReturn < 0 || bValid == false || iDepth > 0 || (iDepth <= 0 && bDepth == true))
	{
      // printf(" EDFParser::Read use old root %p\n", pOldRoot);
      // if(m_bDebug == true)
      {
         int iParseLen = 0;

         if(pPos - pData > 200)
         {
            pParse = pPos - 200;
         }
         else
         {
            pParse = pPos;
         }
         if(pStop - pPos > 200)
         {
            iParseLen = 200;
         }
         else
         {
            iParseLen = pStop - pPos;
         }
         memprint(DEBUGLEVEL_DEBUG, debugfile(), "EDFParser::Read parse failed", pParse, pPos - pParse + iParseLen);
         // m_pRoot->print("EDFParser::Read parse failed");
      }

      // dValue = gettick();
		delete pRoot;
      // lDelete = tickdiff(dValue);

		// if(m_bDebug == true)
      lTick = tickdiff(dTick);
      /* if(lTick > 250)
		{
			debug("EDFParser::Read exit %ld, t=%ld ms\n", lReturn, lTick);
		} */
		return 0;
	}
	else
	{
      // printf(" EDFParser::Read use new root %p\n", m_pRoot);

      // dValue = gettick();
      // lDelete = tickdiff(dValue);

      // printf("EDFParser::Read delete %ld ms\n", lDelete);

		// Root();
	}

   // pRoot->print("EDFParser::Read");

	// if(m_bDebug == true)
   /* lTick = tickdiff(dTick);
   if(lTick > 250)
	{
		debug("EDFParser::Read exit %ld, t=%ld (new=%ld del=%ld set=%ld sp=%ld nm=%ld bv=%ld nv=%ld vld=%ld rem=%ld) ms\n", (long)(pPos - pData), lTick, lNew, lDelete, lSet, lSpace, lName, lBytesVal, lNumVal, lValid, lTick - lNew - lDelete - lSet - lSpace - lName - lBytesVal - lNumVal - lValid);
	} */

   // pRoot->print("EDFParser::Read root");

   pEDF->SetRoot(pRoot);

   /* debug("EDFParser::Read exit %p, %p\n", pReturn, pRoot);
   pRoot->print();
   EDFParser::Print(NULL, pReturn); */

	debug(DEBUGLEVEL_DEBUG, "EDFParser::Read exit %ld, %p parse OK t=%ld (new=%ld set=%ld sp=%ld nm=%ld bv=%ld vld=%ld rem=%ld) ms\n", (long)(pPos - pData), pEDF, lTick, lNew, lSet, lSpace, lName, lBytesVal, lValid, lTick - lNew - lSet - lSpace - lName - lBytesVal - lValid);
	return pPos - pData;
}

bytes *EDFParser::Write(EDF *pEDF, const bool bRoot, const bool bCurr, const bool bPretty, const bool bCRLF)
{
	STACKTRACE
	int iOptions = 0;

   if(bRoot == true)
   {
      iOptions |= EDFElement::EL_ROOT;
   }
	if(bCurr == true)
	{
		iOptions |= EDFElement::EL_CURR;
	}
   if(bPretty == true)
   {
      iOptions |= EDFElement::PR_SPACE;
   }
   if(bCRLF == true)
   {
      iOptions |= EDFElement::PR_CRLF;
   }

	return Write(pEDF, iOptions);
}

bytes *EDFParser::Write(EDF *pEDF, const int iOptions)
{
	STACKTRACE
   EDFElement *pElement = NULL;
   bytes *pReturn = NULL;

   debug(DEBUGLEVEL_DEBUG, "EDFParser::Write %p %d\n", pEDF, iOptions);

   if(mask(iOptions, EDFElement::EL_ROOT) == true)
   {
      pElement = pEDF->GetCurr();
      debug(DEBUGLEVEL_DEBUG, "EDFParser::Write curr %p\n", pElement);
      while(pElement->parent() != NULL)
      {
         pElement = pElement->parent();
         debug(DEBUGLEVEL_DEBUG, "EDFParser::Write parent %p\n", pElement);
      }
   }
   else
   {
      pElement = pEDF->GetCurr();
   }

   debug(DEBUGLEVEL_DEBUG, "EDFParser::Write element %p\n", pElement);

   pReturn = pElement->write(iOptions);

   return pReturn;
}

EDF *EDFParser::FromFile(const char *szFilename, size_t *lReadLen, const int iProgress, const int iOptions)
{
	STACKTRACE
   long lEDF = 0;
	size_t lDataLen = 0;
	byte *pData = NULL;
	EDF *pEDF = NULL;

   // printf(" FileToEDF entry %s\n", szFilename);

   pData = FileRead(szFilename, &lDataLen);
   if(pData == NULL)
   {
      return NULL;
   }

   pEDF = new EDF();
   lEDF = EDFParser::Read(pEDF, pData, lDataLen, iProgress, iOptions);
   delete[] pData;

   if(lEDF <= 0)
   {
      delete pEDF;
      return NULL;
   }

   // Print("EDFParser::FromFile", pEDF);

   // printf(" FileToEDF exit %p\n", pEDF);
   return pEDF;
}

bool EDFParser::ToFile(EDF *pEDF, const char *szFilename, int iOptions)
{
	STACKTRACE
   // long lWriteLen = 0;
   bytes *pWrite = NULL;

	if(iOptions == -1)
	{
		iOptions = EDFElement::EL_ROOT | EDFElement::EL_CURR | EDFElement::PR_SPACE;
	}

   debug("EDFParser::ToFile options %d\n", iOptions);
   // EDFParser::Print(pEDF, true, true);
   pWrite = EDFParser::Write(pEDF, iOptions);
   // memprint("EDFParser::ToFile write", pWrite, lWriteLen);
   if(FileWrite(szFilename, pWrite->Data(false), pWrite->Length()) == -1)
   {
      delete pWrite;
      return false;
   }

   delete pWrite;

   return true;
}

bool EDFParser::Print(EDF *pEDF, int iOptions)
{
	return EDFParser::Print(NULL, NULL, pEDF, iOptions);
}

bool EDFParser::Print(EDF *pEDF, const bool bRoot, const bool bCurr)
{
	int iOptions = EDFElement::PR_SPACE;

	if(bRoot == true)
	{
		iOptions |= EDFElement::EL_ROOT;
	}
	if(bCurr == true)
	{
		iOptions |= EDFElement::EL_CURR;
	}

	return EDFParser::Print(NULL, NULL, pEDF, iOptions);
}

bool EDFParser::Print(const char *szTitle, EDF *pEDF, const int iOptions)
{
	return EDFParser::Print(NULL, szTitle, pEDF, iOptions);
}

/* void EDFParser::Print(const char *szTitle, EDF *pEDF, const bool bRoot, const bool bCurr)
{
	int iOptions = EDFElement::PR_SPACE;

	if(bRoot == true)
	{
		iOptions |= EDFElement::EL_ROOT;
	}
	if(bCurr == true)
	{
		iOptions |= EDFElement::EL_CURR;
	}

	EDFParser::Print(NULL, szTitle, pEDF, iOptions);
} */

bool EDFParser::Print(FILE *fOutput, const char *szTitle, EDF *pEDF, int iOptions)
{
	STACKTRACE
   // long lWriteLen = 0;
   bytes *pWrite = NULL;

   debug(DEBUGLEVEL_DEBUG, "EDFParser::Print %p '%s' %p %d\n", fOutput, szTitle, pEDF, iOptions);

   /* if(fOutput == NULL)
   {
      fOutput = stdout;
   } */

	if(iOptions == -1)
	{
		iOptions = EDFElement::EL_ROOT | EDFElement::EL_CURR | EDFElement::PR_SPACE;
	}

   // printf("EDFParser::Print options %d\n", iOptions);

   if(pEDF != NULL)
   {
      pWrite = EDFParser::Write(pEDF, iOptions);
   }
   if(szTitle != NULL)
   {
      if(fOutput == NULL)
      {
         debug("%s:\n", szTitle);
      }
      else
      {
         fprintf(fOutput, "%s:\n", szTitle);
      }
   }
   if(pWrite != NULL)
   {
      if(fOutput == NULL)
      {
         debug((char *)pWrite->Data(false));
      }
      else
      {
         fwrite(pWrite->Data(false), 1, pWrite->Length(), fOutput);
      }
      if(pWrite->Length() > 0)
      {
         if(fOutput == NULL)
         {
            debug("\n");
         }
         else
         {
            fprintf(fOutput, "\n");
         }
      }
   }
   else
   {
      if(fOutput == NULL)
      {
         debug("NULL\n");
      }
      else
      {
         fwrite("NULL\n", 1, 5, fOutput);
      }
   }

   delete pWrite;

   return true;
}

bool EDFParser::Print(FILE *fOutput, const char *szTitle, EDF *pEDF, const bool bRoot, const bool bCurr)
{
	int iOptions = EDFElement::PR_SPACE;

	if(bRoot == true)
	{
		iOptions |= EDFElement::EL_ROOT;
	}
	if(bCurr == true)
	{
		iOptions |= EDFElement::EL_CURR;
	}

	return EDFParser::Print(fOutput, szTitle, pEDF, iOptions);
}

bool EDFParser::debugPrint(EDF *pEDF, const int iOptions)
{
	return EDFParser::debugPrint(0, NULL, pEDF, iOptions);
}

bool EDFParser::debugPrint(EDF *pEDF, const bool bRoot, const bool bCurr)
{
	int iOptions = EDFElement::PR_SPACE;

	if(bRoot == true)
	{
		iOptions |= EDFElement::EL_ROOT;
	}
	if(bCurr == true)
	{
		iOptions |= EDFElement::EL_CURR;
	}

	return EDFParser::debugPrint(0, NULL, pEDF, iOptions);
}

bool EDFParser::debugPrint(int iLevel, EDF *pEDF, const int iOptions)
{
	return EDFParser::debugPrint(iLevel, NULL, pEDF, iOptions);
}

bool EDFParser::debugPrint(const char *szTitle, EDF *pEDF, const int iOptions)
{
	return EDFParser::debugPrint(0, szTitle, pEDF, iOptions);
}

bool EDFParser::debugPrint(int iLevel, const char *szTitle, EDF *pEDF, const int iOptions)
{
   if(iLevel > debuglevel())
   {
      return false;
   }

	return EDFParser::Print(NULL, szTitle, pEDF, iOptions);
}

/* void EDFParser::debugPrint(const char *szTitle, EDF *pEDF, const bool bRoot, const bool bCurr)
{
	int iOptions = EDFElement::PR_SPACE;

	if(bRoot == true)
	{
		iOptions |= EDFElement::EL_ROOT;
	}
	if(bCurr == true)
	{
		iOptions |= EDFElement::EL_CURR;
	}

	EDFParser::Print(debugfile(), szTitle, pEDF, iOptions);
} */
