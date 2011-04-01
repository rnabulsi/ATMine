#ifndef _NarrAbs_h
#define _NarrAbs_h

#include <QList>
#include <QTextStream>
#include <QFile>
#include <QDebug>
#include "logger.h"

#define SHOW_AS_TEXT


class ChainNarratorPrim;

//typedef QList <ChainNarratorPrim *>::iterator CNPIterator;

//#enumerate the types

class ChainNarratorPrim {
public://protected:
	QString * hadith_text;
public:
//    virtual CNPIterator first () const=0;
//    virtual CNPIterator end () const=0;
	virtual void serialize(QDataStream &chainOut) const=0;
	virtual void deserialize(QDataStream &chainOut)=0;
	virtual void serialize(QTextStream &chainOut) const=0;

    virtual QString getString() const {
		int length=getLength();
		if (length<0)
			return "";
		return hadith_text->mid(getStart(), length);
    }
    virtual int getStart() const = 0;
    virtual int getLength() const = 0;
    virtual int getEnd() const = 0;
 //   virtual int startPosition() const=0;
 //   virtual int lastPosition() const=0;

};

class NarratorPrim: public ChainNarratorPrim {
public:
    long long m_start;
    long long m_end;

	NarratorPrim(QString * hadith_text);
	NarratorPrim(QString * hadith_text,long long m_start);

 //   virtual CNPIterator first () const=0;
 //   virtual CNPIterator end () const=0;
	virtual bool isNamePrim() const =0;
	virtual void serialize(QDataStream &chainOut) const=0;
	virtual void deserialize(QDataStream &chainOut)=0;
	virtual void serialize(QTextStream &chainOut) const=0;
    virtual int getStart() const {
        return m_start;}
    virtual int getLength() const {
        return m_end - m_start + 1;}
    virtual int getEnd() const {
        return m_end;}

};
class ChainPrim :public ChainNarratorPrim {
public:
	ChainPrim(QString* hadith_text);
 //   virtual CNPIterator first () const=0;
//    virtual CNPIterator end () const=0;
	virtual bool isNarrator() const=0;
	virtual void serialize(QDataStream &chainOut) const=0;
	virtual void deserialize(QDataStream &chainOut)=0;
	virtual void serialize(QTextStream &chainOut) const=0;
    virtual int getStart() const = 0;
    virtual int getLength() const =0;
    virtual int getEnd() const = 0;
};
class NamePrim :public NarratorPrim {
public:
    virtual void f(){}
	NamePrim(QString * hadith_text);
	NamePrim(QString * hadith_text,long long m_start);

   // virtual CNPIterator first () const{
   // return QList<ChainNarratorPrim*> ().begin();
  //      }

//    virtual CNPIterator end () const{
//    return QList<ChainNarratorPrim*> ().end();
//    }
	virtual bool isNamePrim() const;
	virtual void serialize(QDataStream &chainOut) const;
	virtual void deserialize(QDataStream &chainOut);
	virtual void serialize(QTextStream &chainOut) const;
};
class NameConnectorPrim :public NarratorPrim {
private:
	enum Type {POS,IBN, OTHER};
	Type type;
public:
	void setPossessive(){type=POS;}
	bool isPossessive(){return type==POS;}
	void setIbn(){type=IBN;}
	bool isIbn(){return type==IBN;}
	void setOther(){type=OTHER;}
	bool isOther(){return type==OTHER;}
	NameConnectorPrim(QString * hadith_text);
	NameConnectorPrim(QString * hadith_text,long long m_start);

//    virtual CNPIterator first () const{
//    return QList<ChainNarratorPrim*> ().begin();
//        }
//
//    virtual CNPIterator end () const{
//    return QList<ChainNarratorPrim*> ().end();
//    }
	virtual bool isNamePrim() const;
	virtual void serialize(QDataStream &chainOut) const;
	virtual void deserialize(QDataStream &chainOut);
	virtual void serialize(QTextStream &chainOut) const;
};
class NarratorConnectorPrim :public ChainPrim {
public:
    long long m_start;
    long long m_end;
	NarratorConnectorPrim(QString * hadith_text);
	NarratorConnectorPrim(QString * hadith_text,long long m_start);

//    virtual CNPIterator first () const{
//    return QList<ChainNarratorPrim*> ().begin();
//        }
//
//    virtual CNPIterator end () const{
//    return QList<ChainNarratorPrim*> ().end();
//    }
	virtual bool isNarrator() const;
	virtual void serialize(QDataStream &chainOut) const;
	virtual void deserialize(QDataStream &chainOut);
	virtual void serialize(QTextStream &chainOut) const;

    virtual int getStart() const {
        return m_start;}
    virtual int getLength() const {
        return m_end - m_start + 1;}
    virtual int getEnd() const {
        return m_end;}
};



class Narrator : public ChainPrim{
public:
	Narrator(QString * hadith_text);
 //   QList <ChainNarratorPrim *> m_narrator; //modify back to here
   QList <NarratorPrim *> m_narrator;
//    virtual CNPIterator first () {
//    return m_narrator.begin();
//        }
//
//    virtual CNPIterator end () const{
//    //return m_narrator.end();
//    }
	virtual bool isNarrator() const;
	virtual void serialize(QDataStream &chainOut) const;
	virtual void deserialize(QDataStream &chainOut);
	virtual void serialize(QTextStream &chainOut) const;

    virtual int getStart() const {
        return m_narrator[0]->m_start;}

    virtual int getLength() const {
        int last = m_narrator.size() - 1;
		if (last==-1)
			return -1;
        int start = getStart();
        return m_narrator[last]->m_end - start + 1;}

    virtual int getEnd() const {
        int last = m_narrator.size() - 1;
        return m_narrator[last]->m_end;}

    virtual bool operator == (Narrator & rhs) const {
        return getString() == rhs.getString(); }

	virtual double equals(const Narrator & rhs) const;
};

class Chain: public ChainNarratorPrim {
public:
	Chain(QString * hadith_text);
	//QList <ChainNarratorPrim *> m_chain;//to be reverted to
    QList <ChainPrim *> m_chain;
	virtual void serialize(QDataStream &chainOut) const;
	virtual void deserialize(QDataStream &chainOut);
	virtual void serialize(QTextStream &chainOut) const;

    virtual int getStart() const {
        return m_chain[0]->getStart(); }

    virtual int getLength() const {
        int last = m_chain.size() - 1;
        int start = getStart();
        return m_chain[last]->getEnd() - start + 1;}

    virtual int getEnd() const {
        int last = m_chain.size() - 1;
        return m_chain[last]->getEnd();}

    virtual bool operator == (Chain & rhs) const {
        return getString() == rhs.getString(); }

//    virtual CNPIterator first () const{
//    //return m_chain.begin();
//        }
//
//    virtual CNPIterator end () const{
//    //return m_chain.end();
//    }

};

double equal(const Narrator & n1,const Narrator & n2);

QDataStream &operator>>(QDataStream &in, ChainNarratorPrim &p);
QDataStream &operator<<(QDataStream &out, const ChainNarratorPrim &p);
#endif
