/*
** EDFItem library
** (c) 2000 Michael Wood (mike@compsoc.man.ac.uk)
**
** TreeNodeItem.h: Definition of TreeNodeItem class
**
** An EDFItem with a pointer to it's parent and children
*/

#ifndef _TREENODEITEM_H_
#define _TREENODEITEM_H_

#include "EDFItem.h"
#include "EDFItemList.h"

template <class T> class TreeNodeItem : public EDFItem
{
public:
	TreeNodeItem(long lID, T pParent);
	TreeNodeItem(EDF *pEDF, T pParent);
	TreeNodeItem(DBTable *pTable, T pParent, int iEDFField);
	virtual ~TreeNodeItem();

	T GetParent();
	bool SetParent(T pParent);

   long GetParentID();
   bool SetParentID(long lParentID);

   bool Add(T pItem);
   bool Delete(long lID);
   int Count(bool bRecurse);
   T Find(long lID, bool bRecurse = true);
   T Child(int iChildNum);
   // int Pos(long lID, bool bRecurse = true);

   bool ItemDelete(int iItemNum);

   bool Delete(char *szTable = NULL);

protected:
   virtual bool WriteFields(DBTable *pTable, EDF *pEDF);
   virtual bool ReadFields(DBTable *pTable);
   T find(long lID, bool bRecurse, int *iPos);

private:
	T m_pParent;
   long m_lParentID;

	EDFItemList *m_pChildren;

	void init(T pParent);
};

#include "TreeNodeItem.cpp"

#endif
