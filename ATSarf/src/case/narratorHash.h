#ifndef NARRATORHASH_H
#define NARRATORHASH_H

#include "hadithCommon.h"
#include <QtCore/qdatastream.h>
#include <QDataStream>
#include <QMultiHash>
#include <QVector>
#include "graph_nodes.h"


inline bool possessivesCompare(const NameConnectorPrim * n1,const NameConnectorPrim * n2) {
	return n1->getString()<n2->getString();
}

class NarratorHash {
private:
	typedef QVector< NarratorPrim *> NamePrimList;
	typedef QVector<NamePrimList> NamePrimHierarchy;
	typedef QVector<NameConnectorPrim *> PossessiveList;

	class HashValue {
	public:
		HashValue(ChainNarratorNode * node, int value, int total) {
			this->node=node;
			this->value=value;
			this->total=total;
		}
		ChainNarratorNode * node;
		int value,total;
	};

	typedef QMultiHash<QString,HashValue> HashTable;

	HashTable hashTable;

	QString getKey(const NamePrimHierarchy & hierarchy) {
		QString key="";
		int size=hierarchy.size();
		for (int i=0;i<size;i++) {
			int levelSize=hierarchy[i].size();
			for (int j=0; j<levelSize;j++) {
				key.append(hierarchy[i][j]->getString());
				if (j!=levelSize-1)
					key.append(" ");
			}
			if (i!=size-1)
				key.append("-");
		}
		return key;
	}
	inline QString getKey(const NamePrimHierarchy & hierarchy,const PossessiveList & possessives,int size, bool poss, bool skipFirst) {
		QString key="";
		for (int i=0;i<size;i++) {
			if (!skipFirst || i!=0) {
				int levelSize=hierarchy[i].size();
				for (int j=0; j<levelSize;j++) {
					key.append(hierarchy[i][j]->getString());
					if (j!=levelSize-1)
						key.append(" ");
				}
			}
			if (i==size-1) {
				if (poss) {
					key.append(":");
					int levelSize=possessives.size();
					for (int j=0; j<levelSize;j++) {
						key.append(possessives[j]->getString());
						if (j!=levelSize-1)
							key.append(" ");
					}
				}
			} else {
				key.append("-");
			}
		}
		return key;
	}
	class Visitor {
	public:
		virtual void visit(const QString & s, ChainNarratorNode * c, int value, int total)=0;
	};
	class InsertVisitor: public Visitor {
	private:
		NarratorHash * hash;
	public:
		InsertVisitor(NarratorHash * hash) {this->hash=hash;}
		void visit(const QString & s, ChainNarratorNode * c, int value, int total){
			hash->hashTable.insert(s,HashValue(c,value,total));
		}
	};
	class FindVisitor: public Visitor {
	private:
		NarratorHash * hash;
		double largestEquality;
		ChainNarratorNode * node;
	public:
		FindVisitor(NarratorHash * hash) {
			this->hash=hash;
			largestEquality=0;
			node=NULL;
		}
		void visit(const QString & s, ChainNarratorNode *, int value, int total){
			HashTable::iterator i = hash->hashTable.find(s);
			 while (i != hash->hashTable.end() && i.key() == s) {
				 HashValue v=*i;
				 double curr_equality=v.value/v.total*value/total;
				 if (curr_equality>largestEquality) {
					 largestEquality=curr_equality;
					 node=v.node;
				 }
				 ++i;
			 }
		}
		double getEqualityValue() { return largestEquality; }
		ChainNarratorNode * getCorrespondingNode() { return node; }
	};
	class DebuggingVisitor: public Visitor {
	public:
		virtual void visit(const QString & s, ChainNarratorNode * , int value, int total){
			qDebug()<<"\t"<<s<<"\t"<<value<<"/"<<total;
		}
	};

	void generateAllPosibilities(ChainNarratorNode * node, Visitor & v) {
	#if 0
	#ifdef NARRATORHASH_DEBUG
		qDebug()<<node->getNarrator().getString();
	#endif
		double max_equality=hadithParameters.equality_threshold*2,
			   delta=hadithParameters.equality_delta;
	#endif
		Narrator * n=&node->getNarrator();
		bool abihi=isRelativeNarrator(*n);
		if (abihi)
			return; //we dont know yet to what person is the ha2 in abihi a reference so they might not be equal.
		if (n->isRasoul) {
			v.visit(alrasoul,node,1,1);
			return;
		}
		NamePrimHierarchy names;
		PossessiveList possessives;
		int j=0; //index of names entry defined by bin
		names.append(NamePrimList());
		//qDebug()<< names.size();
		for (int i=0;i<n->m_narrator.count();i++) {
			if (n->m_narrator[i]->getString().isEmpty())
				continue;
			if (n->m_narrator[i]->isNamePrim())
				names[j].append(n->m_narrator[i]);
			else {
				NameConnectorPrim * c=(NameConnectorPrim*)n->m_narrator[i];
				if (c->isPossessive()) {
					possessives.append(c);
				} else if (c->isIbn()){
					names.append(NamePrimList());
					j++;
				} else if (c->isFamilyConnector()) {
					names[j].append(c);
				}
				//if (c->isOther()) do nothing
			}
		}
		//TODO: add sorting of names inside same level (pay attention to those after family connector) and possessives
		qSort(possessives.begin(),possessives.end(),possessivesCompare);
		//TODO: allow for skipping in the selection if total number is conserved

		int levelSize=names.size();
	#if 1
		assert(levelSize>0);
		bool possAvailable=possessives.count()>0,
			 levelOneEmpty=names.at(0).size()==0;
	#ifdef HASH_TOTAL_VALUES
		int total=(possAvailable?(levelSize-1)*2:levelSize-1)+(!levelOneEmpty ?(possAvailable?levelSize*2:levelSize):0);
	#else
		int total=(possAvailable?levelSize*2:levelSize)-(levelOneEmpty ?(possAvailable?2:1):0);
	#endif
		int minLevel=(levelOneEmpty?2:1);
		int value;
		for (int level=minLevel;level<=levelSize;level++) {
		#ifdef HASH_TOTAL_VALUES
			value=(level-minLevel+1)*(!levelOneEmpty?2:1)+(minLevel-2);
		#else
			value=level-minLevel+1;
		#endif
			v.visit(getKey(names,possessives,level,false,false),node,value,total);
			if (possAvailable) {
				value*=2;
				v.visit(getKey(names,possessives,level,true,false),node,value,total);
			}
			if (level>1 && !levelOneEmpty) {
				value=level-1;
				v.visit(getKey(names,possessives,level,false,true),node,value,total);
				if (possAvailable) {
					value*=2;
					v.visit(getKey(names,possessives,level,true,true),node,value,total);
				}
			}
		}
	#else
		int possSize=possessives.size();
		for (int poss_count=0;poss_count<=possSize;poss_count++) {
			for (int level_count=1;level_count<=levelSize+1;level_count++) {
				int level_min=(level_count<=levelSize?0:1);
				int level_max=(level_count<=levelSize?level_count:names.size());
				if (level_min==1) {
					if (0==levelSize || names[0].size()==0)
						continue;
				}
				for (int names_count=1;names_count<=1/*names[level_count].size()*/;names_count++) {
					NamePrimHierarchy result;
					int actualLevelCount=0;
					if (level_min==1)
						result.append(NamePrimList());
					for (int j=level_min;j<level_max;j++) {
						NamePrimList nameList;
						for (int k=0;k<names_count && k<names[j].count();k++) {
							nameList.append(names[j][k]);
						}
						result.append(nameList);
						if (nameList.size()>0)
							actualLevelCount++;
					}
					NamePrimList possessiveList;
					for (int i=0;i<poss_count;i++) {
						possessiveList.append(possessives[i]);
					}
					if (possessiveList.size()>0)
						result.append(possessiveList);
					double value=poss_count*delta/2+actualLevelCount*max_equality/levelSize/2;
					if (result.size()>0) {
						for (int f=0;f<result.size();f++) {
							if (result[f].size()>0) { //at least one level not empty
								QString key=getKey(result);
							#ifdef NARRATORHASH_DEBUG
								qDebug()<<"\t"<<key<<"\t"<<value;
							#endif
								v.visit(key,node,value);
								break;
							}
						}
					}
				}
			}
		}
	#endif
	}
public:
#if defined(REFINEMENTS) && defined(EQUALITY_REFINEMENTS) //does not work otherwise, instead of doing different versions for combinations of them on/off
	void addNode(ChainNarratorNode * node) {
	#ifdef NARRATORHASH_DEBUG
		qDebug()<<node->getNarrator().getString();
		DebuggingVisitor d;
		generateAllPosibilities(node,d);
	#endif
		InsertVisitor v(this);
		generateAllPosibilities(node,v);
	}
	ChainNarratorNode * findCorrespondingNode(Narrator * n) {
		ChainNarratorNode * node=new ChainNarratorNode(n,-1,-1);
		FindVisitor v(this);
		generateAllPosibilities(node,v);
		delete node;
		ChainNarratorNode * c=v.getCorrespondingNode();
	#ifdef NARRATORHASH_DEBUG
		qDebug()<<n->getString();
		DebuggingVisitor d;
		generateAllPosibilities(node,d);
	#endif
	#if 0
		double val=v.getEqualityValue();
		double maxEquality=hadithParameters.equality_threshold*hadithParameters.equality_threshold;

		if (val>=maxEquality) {
			return c;
		} else {
			return NULL;
		}
	#else
		return c;
	#endif
	}
	void clear() {
		hashTable.clear();
	}
#endif
};


#endif // NARRATORHASH_H
