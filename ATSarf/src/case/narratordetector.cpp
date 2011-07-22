#include "narratordetector.h"
#include "graph.h"
#include "narratorHash.h"

#ifdef NARRATORDEBUG
inline QString type_to_text(wordType t) {
	switch(t)
	{
		case NAME:
			return "NAME";
		case NRC:
			return "NRC";
		case NMC:
			return "NMC";
	#ifdef REFINEMENTS
		case STOP_WORD:
			return "STOP_WORD";
	#endif
		default:
			return "UNDEFINED-TYPE";
	}
}
inline QString type_to_text(stateType t) {
	switch(t)
	{
		case TEXT_S:
			return "TEXT_S";
		case NAME_S:
			return "NAME_S";
		case NMC_S:
			return "NMC_S";
		case NRC_S:
			return "NRC_S";
	#ifdef REFINEMENTS
		case STOP_WORD_S:
			return "STOP_WORD_S";
	#endif
		default:
			return "UNDEFINED-TYPE";
	}
}
inline void display(wordType t) {
	out<<type_to_text(t)<<" ";
	//qDebug() <<type_to_text(t)<<" ";
}
inline void display(stateType t) {
	out<<type_to_text(t)<<" ";
	//qDebug() <<type_to_text(t);
}
inline void display(QString t) {
	out<<t;
	//qDebug() <<t;
}
#else
	#define display(c)
#endif

class NarratorDetector
{
private:
	typedef struct stateData_ {
		long  biographyStartIndex, narratorCount,narratorStartIndex,narratorEndIndex;
		long  nmcCount, nrcCount,nameStartIndex,nmcStartIndex;
		bool nmcValid;
		bool ibn_or_3abid;

		void initialize() {
			nmcCount=0;
			narratorCount=0;
			nrcCount=0;
			narratorStartIndex=0;
			narratorEndIndex=0;
			nmcValid=false;
			ibn_or_3abid=false;
			nameStartIndex=0;
			nmcStartIndex=0;
			biographyStartIndex=0;
		}

	} stateData;
	class BiographyData {
	public:
		NamePrim *namePrim;
		NameConnectorPrim *nameConnectorPrim;
		TempConnectorPrimList temp_nameConnectors;
		Narrator *narrator;
		Biography *biography;

		void initialize(NarratorGraph *graph,QString * text) {
			if (namePrim!=NULL) {
				delete namePrim;
				namePrim=NULL;
			}
			if (nameConnectorPrim!=NULL) {
				delete nameConnectorPrim;
				nameConnectorPrim=NULL;
			}
			if (narrator!=NULL) {
				delete narrator;
				narrator=NULL;
			}
			int s=0;
			if (biography!=NULL) {
				s=biography->getStart();
				delete biography;
			}
			for (int i=0;i<temp_nameConnectors.size();i++)
				delete temp_nameConnectors[i];
			temp_nameConnectors.clear();
			biography=new Biography(graph,text,s);
		}
		BiographyData(){
			namePrim=NULL;
			nameConnectorPrim=NULL;
			narrator=NULL;
			biography=NULL;
		}
	};
	typedef QList<NarratorNodeIfc *> NarratorNodeList;

	stateData currentData;
	QString * text;
	long current_pos;

	NarratorGraph * graph;

public:
	BiographyList * biographies;

private:

	inline void fillStructure(StateInfo &  stateInfo,const Structure & currentStructure,BiographyData *currentBiography,bool punc=false,bool ending_punc=false) {
	#ifdef CHAIN_BUILDING
		assert(!ending_punc || (punc&& ending_punc));
		/*if (currentStructure==INITIALIZE && stateInfo.processedStructure!=RASOUL_WORD && !punc) {
			assert(currentChain->narrator==NULL);
			stateInfo.processedStructure=currentStructure;
			return;
		}*/

		if (punc) {
			switch(stateInfo.processedStructure) {
			case INITIALIZE: {
				assert(false);//must not happen probably
				break;
			}
			case NAME_PRIM: {
				assert(currentBiography->narrator!=NULL);
				assert(currentBiography->narrator->m_narrator.size()>0);
				currentBiography->biography->addNarrator(currentBiography->narrator);
				currentBiography->narrator=NULL;
				assert(currentStructure==NARRATOR_CONNECTOR);
				break;
			}
			case NARRATOR_CONNECTOR: {
				if (!ending_punc) {
					assert(currentStructure==NAME_PRIM);
				}
				break;
			}
			case NAME_CONNECTOR: {
				int size=currentBiography->temp_nameConnectors.size();
				if (!ending_punc) {//check if we should add these
					assert(currentBiography->narrator!=NULL);
					for (int i=0;i<size;i++)
						currentBiography->narrator->m_narrator.append(currentBiography->temp_nameConnectors[i]);
				} else {
					for (int i=0;i<size;i++)
						delete currentBiography->temp_nameConnectors[i];
				}
				currentBiography->temp_nameConnectors.clear();
				if (currentBiography->narrator!=NULL) {
					assert(currentBiography->narrator->m_narrator.size()>0);
					currentBiography->biography->addNarrator(currentBiography->narrator);
					currentBiography->narrator=NULL;
				}
				break;
			}
			default:
				assert(false);
			}
		}else {
			if (currentStructure!=INITIALIZE) {
				switch(stateInfo.processedStructure) {
				case INITIALIZE:
					currentBiography->initialize(graph,text);
					display(QString("\ninit%1\n").arg(currentBiography->biography->m_chain.size()));
					assert(currentStructure!=INITIALIZE); //must not happen
					break;
				case NARRATOR_CONNECTOR:
					break;
				case NAME_CONNECTOR:
					if (currentStructure!=NAME_CONNECTOR) {
						if (currentBiography->narrator==NULL)
							currentBiography->narrator=new Narrator(text);
						int size=currentBiography->temp_nameConnectors.size();
						for (int i=0;i<size;i++)
							currentBiography->narrator->m_narrator.append(currentBiography->temp_nameConnectors[i]);
						currentBiography->temp_nameConnectors.clear();
						if (currentStructure==NARRATOR_CONNECTOR) {
							currentBiography->biography->addNarrator(currentBiography->narrator);
							currentBiography->narrator=NULL;
						}
					}
					break;
				case RASOUL_WORD:
					if (currentStructure!=RASOUL_WORD) {
						if (currentStructure==NARRATOR_CONNECTOR) {
							fillStructure(stateInfo,INITIALIZE,currentBiography,punc,ending_punc);
							stateInfo.processedStructure=NARRATOR_CONNECTOR;
							return;
						} else
							assert(currentStructure==INITIALIZE);
					}
					break;
				case NAME_PRIM:
					if (currentStructure==NARRATOR_CONNECTOR) {
						currentBiography->biography->addNarrator(currentBiography->narrator);
						currentBiography->narrator=NULL;
					}
					break;
				default:
					assert(false);
				}
			}
			switch(currentStructure) {
			case INITIALIZE: {
				switch(stateInfo.processedStructure) {
				case RASOUL_WORD:
					if (currentBiography->narrator==NULL)
						currentBiography->narrator=new Narrator(text);
					assert(currentBiography->nameConnectorPrim!=NULL);
					currentBiography->narrator->m_narrator.append(currentBiography->nameConnectorPrim);
					currentBiography->nameConnectorPrim=NULL;
					currentBiography->narrator->isRasoul=true;
					currentBiography->biography->addNarrator(currentBiography->narrator);
					currentBiography->narrator=NULL;
					break;
				case NARRATOR_CONNECTOR:
					break;
				}

				//not known yet: maybe fill unfinished structures
				break;
			}
			case NAME_PRIM: {
				if (currentBiography->namePrim==NULL)
					currentBiography->namePrim=new NamePrim(text,stateInfo.startPos);
				currentBiography->namePrim->m_end=stateInfo.endPos;
			#ifdef REFINEMENTS
				currentBiography->namePrim->learnedName=stateInfo.learnedName;
			#endif
				if (currentBiography->narrator==NULL)
					currentBiography->narrator=new Narrator(text);
				currentBiography->narrator->m_narrator.append(currentBiography->namePrim);
				currentBiography->namePrim=NULL;
				break;
			}
			case NARRATOR_CONNECTOR: {
				break;
			}
			case NAME_CONNECTOR: {
				if (currentBiography->nameConnectorPrim==NULL)
					currentBiography->nameConnectorPrim=new NameConnectorPrim(text,stateInfo.startPos);
				currentBiography->nameConnectorPrim->m_end=stateInfo.endPos;
			#ifdef REFINEMENTS
				if (stateInfo.familyNMC){
					assert(currentBiography->nameConnectorPrim->isOther());
					currentBiography->nameConnectorPrim->setFamilyConnector();
					if (stateInfo.ibn)
						currentBiography->nameConnectorPrim->setIbn();
					else if (stateInfo._2ab)
						currentBiography->nameConnectorPrim->setAB();
					else if (stateInfo._2om)
						currentBiography->nameConnectorPrim->setOM();
					//TODO: learn that word that comes after is a name
				} else if (stateInfo.possessivePlace) {
					assert(currentBiography->nameConnectorPrim->isOther());
					currentBiography->nameConnectorPrim->setPossessive();
				}
			#endif
				if (stateInfo.isFamilyConnectorOrPossessivePlace()) {
					if (currentBiography->narrator==NULL)
						currentBiography->narrator=new Narrator(text);
					int size=currentBiography->temp_nameConnectors.size();
					for (int i=0;i<size;i++)
						currentBiography->narrator->m_narrator.append(currentBiography->temp_nameConnectors[i]);
					currentBiography->narrator->m_narrator.append(currentBiography->nameConnectorPrim);
					currentBiography->nameConnectorPrim=NULL;
					currentBiography->temp_nameConnectors.clear();
				} else {
					currentBiography->temp_nameConnectors.append(currentBiography->nameConnectorPrim);
					currentBiography->nameConnectorPrim=NULL;
				}
				break;
			}
			case RASOUL_WORD: {
				switch(stateInfo.processedStructure) {
				case NAME_CONNECTOR: {
					//1-finish old narrator and use last nmc as nrc
					if (currentBiography->narrator==NULL)
						currentBiography->narrator=new Narrator(text);
					int size=currentBiography->temp_nameConnectors.size();
					for (int i=0;i<size-1;i++)
						currentBiography->narrator->m_narrator.append(currentBiography->temp_nameConnectors[i]);
					if (size>1) {
						if (currentBiography->narrator->m_narrator.size()>0) {
							currentBiography->biography->addNarrator(currentBiography->narrator);
							currentBiography->narrator=NULL;
							NameConnectorPrim *n=currentBiography->temp_nameConnectors[size-1];
						#if 0
							currentBiography->narratorConnectorPrim=new NarratorConnectorPrim(text,n->getStart());
							currentBiography->narratorConnectorPrim->m_end=n->getEnd();
							currentBiography->biography->addNarrator(currentBiography->narratorConnectorPrim);
							currentBiography->narratorConnectorPrim=NULL;
						#endif
							delete n;
						}
					} else {
						if (currentBiography->narrator->m_narrator.size()>0) {
							currentBiography->biography->addNarrator(currentBiography->narrator);
							currentBiography->narrator=NULL;
						}
					}
					currentBiography->temp_nameConnectors.clear();
					//display(currentChain->narratorConnectorPrim->getString()+"\n");
				}
					//2-create a new narrator of just this stop word as name connector, so we dont insert "break;"
				case NARRATOR_CONNECTOR:
				case NAME_PRIM: {
					assert(currentBiography->nameConnectorPrim==NULL);
					currentBiography->nameConnectorPrim=new NameConnectorPrim(text,stateInfo.startPos); //we added this to previous name bc assumed this will only happen if it is muhamad and "sal3am"
					currentBiography->nameConnectorPrim->m_end=stateInfo.endPos;
					if (currentBiography->narrator!=NULL) {
						if (!currentBiography->narrator->m_narrator.isEmpty()) {
							NarratorPrim * n=currentBiography->narrator->m_narrator.last();
							if (n->isNamePrim() && !((NamePrim *)n)->learnedName) {//if last has not a learned name, we split both narrators. maybe this must happen to all narrators
								currentBiography->biography->addNarrator(currentBiography->narrator);
								currentBiography->narrator=NULL;
							}
						}
					}
					break;
				}
				case RASOUL_WORD: {
					assert(currentBiography->nameConnectorPrim!=NULL);
					currentBiography->nameConnectorPrim->m_end=stateInfo.endPos;
					break;
				}
				default:
					assert(false);
				}
				break;
			}
			default:
				assert(false);
			}
		}
		stateInfo.processedStructure=currentStructure;
	#endif
	}
	inline void assertStructure(StateInfo & stateInfo,const Structure s) {
	#ifdef CHAIN_BUILDING
		assert(stateInfo.processedStructure==s);
	#endif
	}

	bool getNextState(StateInfo &  stateInfo,BiographyData *currentChain) {
		display(QString(" nmcsize: %1 ").arg(currentData.nmcCount));
		display(QString(" nrcsize: %1 ").arg(currentData.nrcCount));
		display(stateInfo.currentState);
		bool ending_punc=false;
	#ifdef PUNCTUATION
		if (stateInfo.currentPunctuationInfo.has_punctuation) {
			display("<has punctuation>");
			if (stateInfo.currentPunctuationInfo.fullstop && stateInfo.currentPunctuationInfo.newLine) {
				ending_punc=true;
				display("<ending Punctuation>");
			}
		}
		if (stateInfo.number) {
			stateInfo.currentPunctuationInfo.has_punctuation=true;
			stateInfo.currentPunctuationInfo.fullstop=true;
			stateInfo.currentPunctuationInfo.newLine=true;
		}
	#endif
	#ifdef REFINEMENTS
		bool reachedRasoul= (stateInfo.currentType== STOP_WORD && !stateInfo.familyConnectorOr3abid());//stop_word not preceeded by 3abid or ibn
		if (stateInfo.currentType== STOP_WORD && !reachedRasoul)
			stateInfo.currentType=NAME;
	#endif
		bool return_value=true;
		switch(stateInfo.currentState)
		{
		case TEXT_S:
			assertStructure(stateInfo,INITIALIZE);
			if(stateInfo.currentType==NAME) {
				currentData.initialize();
				stateInfo.nextState=NAME_S;
				currentData.biographyStartIndex=stateInfo.startPos;
				currentData.narratorStartIndex=stateInfo.startPos;

				fillStructure(stateInfo,NAME_PRIM,currentChain);

			#ifdef STATS
				temp_names_per_narrator=1;
			#endif
			#ifdef PUNCTUATION
				if (stateInfo.currentPunctuationInfo.has_punctuation) {
					display("<punc1>");
					currentData.narratorCount++;
					stateInfo.nextState=NRC_S;
					currentData.nrcCount=0;//punctuation is zero
					currentData.narratorEndIndex=stateInfo.endPos;
					//currentData.nrcStartIndex=stateInfo.nextPos;//next_positon(stateInfo.endPos,stateInfo.followedByPunctuation);

					fillStructure(stateInfo,NAME_CONNECTOR,currentChain,true);

					if (ending_punc) {
						stateInfo.nextState=TEXT_S;
						currentData.narratorEndIndex=stateInfo.endPos;
						currentData.narratorCount++;
						return_value=false;
					}
				}
			#endif
			}
			else if (stateInfo.currentType==NRC) {
				currentData.initialize();
				currentData.biographyStartIndex=stateInfo.startPos;
				//currentData.nrcStartIndex=stateInfo.startPos;
				stateInfo.nextState=NRC_S;
				currentData.nrcCount=1;

				fillStructure(stateInfo,NARRATOR_CONNECTOR,currentChain);

			#ifdef STATS
				temp_nrc_s.clear();
				map_entry * entry=new map_entry;
				entry->exact=current_exact;
				entry->stem=current_stem;
				entry->frequency=1;
				temp_nrc_s.append(entry);
				temp_nrc_count=1;
			#endif
			#ifdef REFINEMENTS
				if (stateInfo._3an) {
					currentData.nrcCount=1;
					//currentData.nrcEndIndex=stateInfo.endPos;

					assertStructure(stateInfo,NARRATOR_CONNECTOR);
					fillStructure(stateInfo,NAME_PRIM,currentChain,true);

					stateInfo.nextState=NAME_S;
				}
			#endif
			#ifdef PUNCTUATION
				if (ending_punc) {
					currentData.narratorEndIndex=stateInfo.endPos;
					stateInfo.nextState=TEXT_S;
					return_value=false;
				}
			#endif
			}
	#ifdef IBN_START//needed in case a hadith starts by familyConnector such as "ibn yousef qal..."
			else if (stateInfo.currentType==NMC && stateInfo.familyNMC) {
			#ifdef PUNCTUATION
				if (!stateInfo.previousPunctuationInfo.fullstop) {
					stateInfo.nextState=TEXT_S;
					break;
				}
			#endif
				display("<Family1>");
				currentData.initialize();
				currentData.biographyStartIndex=stateInfo.startPos;
				currentData.nmcStartIndex=stateInfo.startPos;
				currentData.narratorStartIndex=stateInfo.startPos;
				currentData.nmcCount=1;
				stateInfo.nextState=NMC_S;
				currentData.nmcValid=true;

				fillStructure(stateInfo,NAME_CONNECTOR,currentChain);

			#ifdef STATS
				temp_nmc_s.clear();
				map_entry * entry=new map_entry;
				entry->exact=current_exact;
				entry->stem=current_stem;
				entry->frequency=1;
				temp_nmc_s.append(entry);
				temp_nmc_count=1;
			#endif
			#ifdef PUNCTUATION
				if (ending_punc) {
					stateInfo.nextState=TEXT_S;

					fillStructure(stateInfo,INITIALIZE,currentChain,true,true); //futureStructure=INITIALIZE will reset the structure for next time and will not hurt flow since anyways resetting here

					currentData.narratorCount++;
					currentData.narratorEndIndex=stateInfo.endPos;
					return_value=false;
				}
			#endif
			}
	#endif
			else {
				stateInfo.nextState=TEXT_S;
			}
			break;

		case NAME_S:
			assertStructure(stateInfo,NAME_PRIM);
		#ifdef REFINEMENTS
			if(reachedRasoul)
			{
				display("<STOP1>");
				stateInfo.nextState=STOP_WORD_S;
				currentData.narratorCount++;

				fillStructure(stateInfo,RASOUL_WORD,currentChain);

				currentData.narratorEndIndex=stateInfo.endPos;
			#ifdef STATS
				for (int i=temp_nmc_s.count()-temp_nmc_count;i<temp_nmc_s.count();i++)
				{
					delete temp_nmc_s[i];
					temp_nmc_s.remove(i);
				}
				for (int i=temp_nrc_s.count()-temp_nrc_count;i<temp_nrc_s.count();i++)
				{
					delete temp_nrc_s[i];
					temp_nrc_s.remove(i);
				}
				temp_nmc_count=0;
				temp_nrc_count=0;
			#endif

				//return_value= false;
				break;
			}
		#endif
			if(stateInfo.currentType==NMC) {
				stateInfo.nextState=NMC_S;
				currentData.nmcValid=stateInfo.isFamilyConnectorOrPossessivePlace();
				currentData.nmcCount=1;
				currentData.nmcStartIndex=stateInfo.startPos;

				//check why in previous implementation, were clearing currentChain->temp_nameConnectors without adding them in even if NMC was ibn or possessive
				fillStructure(stateInfo,NAME_CONNECTOR,currentChain);

			#ifdef STATS
				map_entry * entry=new map_entry;
				entry->exact=current_exact;
				entry->stem=current_stem;
				entry->frequency=1;
				temp_nmc_s.append(entry);
				temp_nmc_count++;
			#endif
			#ifdef PUNCTUATION
				if (stateInfo.currentPunctuationInfo.has_punctuation) {
					currentData.nmcCount=hadithParameters.nmc_max+1;
					if (ending_punc) {
						stateInfo.nextState=TEXT_S;

						fillStructure(stateInfo,INITIALIZE,currentChain,true,true);

						currentData.narratorEndIndex=stateInfo.endPos;
						currentData.narratorCount++;
						return_value=false;
					}
				}
			#endif
			}
			else if (stateInfo.currentType==NRC) {
				stateInfo.nextState=NRC_S;
			#ifdef GET_WAW
				if (!stateInfo.isWaw) //so as not to affect the count for tolerance and lead to false positives, just used for accuracy
			#endif
					currentData.narratorCount++;
			#ifdef STATS
				stat.name_per_narrator.append(temp_names_per_narrator);//found 1 name
				temp_names_per_narrator=0;

				map_entry * entry=new map_entry;
				entry->exact=current_exact;
				entry->stem=current_stem;
				entry->frequency=1;
				temp_nrc_s.append(entry);
				temp_nrc_count=1;
				temp_nmc_count=0;
			#endif
				display(QString("counter%1\n").arg(currentData.narratorCount));
				currentData.nrcCount=1;
				currentData.narratorEndIndex=stateInfo.lastEndPos;//getLastLetter_IN_previousWord(stateInfo.startPos);
				//currentData.nrcStartIndex=stateInfo.startPos;

				fillStructure(stateInfo,NARRATOR_CONNECTOR,currentChain);

			#ifdef REFINEMENTS
				if (stateInfo._3an) {
					currentData.nrcCount=1;
					//currentData.nrcEndIndex=stateInfo.endPos;

					assertStructure(stateInfo,NARRATOR_CONNECTOR);
					fillStructure(stateInfo,NAME_PRIM,currentChain,true);

					stateInfo.nextState=NAME_S;
				}
			#endif
			#ifdef PUNCTUATION
			#if 0
				if (stateInfo.punctuationInfo.has_punctuation)
					currentData.nrcCount=parameters.nrc_max;
			#endif
				if (ending_punc) {
					currentData.narratorEndIndex=stateInfo.endPos;
					stateInfo.nextState=TEXT_S;
					return_value=false;
				}
			#endif
			}
			else {
			#ifdef STATS
				if (currentType==NAME) {
					temp_names_per_narrator++;//found another name name
				}
			#endif
				stateInfo.nextState=NAME_S;

				fillStructure(stateInfo,NAME_PRIM,currentChain);

			#ifdef PUNCTUATION
				if (stateInfo.currentPunctuationInfo.has_punctuation) {
					display("<punc2>");
					currentData.narratorCount++;
					stateInfo.nextState=NRC_S;
					currentData.nrcCount=0; //punctuation not counted
					currentData.narratorEndIndex=stateInfo.endPos;
					//currentData.nrcStartIndex=stateInfo.nextPos;//next_positon(stateInfo.endPos,stateInfo.followedByPunctuation);

					fillStructure(stateInfo,NARRATOR_CONNECTOR,currentChain,true);

					if (ending_punc) {
						currentData.narratorEndIndex=stateInfo.endPos;
						stateInfo.nextState=TEXT_S;
						return_value=false;
					}
				}
			#endif
			}
			break;

		case NMC_S:
			assertStructure(stateInfo,NAME_CONNECTOR);
		#ifdef REFINEMENTS
			if(reachedRasoul) {
				display("<STOP2>");
				//1-finish old narrator and use last nmc as nrc
				currentData.narratorCount++;
			#ifdef STATS
				stat.name_per_narrator.append(temp_names_per_narrator);//found 1 name
				temp_names_per_narrator=0;

				map_entry * entry=new map_entry;
				entry->exact=current_exact;
				entry->stem=current_stem;
				entry->frequency=1;
				temp_nrc_s.append(entry);
				temp_nrc_count=0;
				temp_nmc_count=0;
			#endif
				display(QString("counter%1\n").arg(currentData.narratorCount));
				currentData.nrcCount=0;
				stateInfo.nextState=NRC_S;

				currentData.narratorEndIndex=stateInfo.lastEndPos;//getLastLetter_IN_previousWord(currentData.nmcStartIndex);
				//currentData.nrcStartIndex=currentData.nmcStartIndex;
				//currentData.nrcEndIndex=stateInfo.lastEndPos;//getLastLetter_IN_previousWord(stateInfo.startPos);

				//2-create a new narrator of just this stop word as name connector
				currentData.narratorEndIndex=stateInfo.endPos;
				stateInfo.nextState=STOP_WORD_S;
				currentData.narratorCount++;

				fillStructure(stateInfo,RASOUL_WORD,currentChain);

			#ifdef STATS
				for (int i=temp_nmc_s.count()-temp_nmc_count;i<temp_nmc_s.count();i++)
				{
					delete temp_nmc_s[i];
					temp_nmc_s.remove(i);
				}
				for (int i=temp_nrc_s.count()-temp_nrc_count;i<temp_nrc_s.count();i++)
				{
					delete temp_nrc_s[i];
					temp_nrc_s.remove(i);
				}
				temp_nmc_count=0;
				temp_nrc_count=0;
			#endif
				//return_value= false;
				break;
			}
		#endif
			if (stateInfo.currentType==NRC) {
				currentData.narratorCount++;
			#ifdef STATS
				stat.name_per_narrator.append(temp_names_per_narrator);//found 1 name
				temp_names_per_narrator=0;

				map_entry * entry=new map_entry;
				entry->exact=current_exact;
				entry->stem=current_stem;
				entry->frequency=1;
				temp_nrc_s.append(entry);
				temp_nrc_count=1;
				temp_nmc_count=0;
			#endif
				display(QString("counter%1\n").arg(currentData.narratorCount));
				currentData.nmcCount=0;
				currentData.nrcCount=1;
				stateInfo.nextState=NRC_S;

				currentData.narratorEndIndex=stateInfo.lastEndPos;//getLastLetter_IN_previousWord(stateInfo.startPos);
				//currentData.nrcStartIndex=stateInfo.startPos;

				fillStructure(stateInfo,NARRATOR_CONNECTOR,currentChain);

			#ifdef REFINEMENTS
				if (stateInfo._3an) {
					currentData.nrcCount=1;
					//currentData.nrcEndIndex=stateInfo.endPos;

					assertStructure(stateInfo,NARRATOR_CONNECTOR);
					fillStructure(stateInfo,NAME_PRIM,currentChain,true);

					stateInfo.nextState=NAME_S;
				}
			#endif
			#ifdef PUNCTUATION
				if (ending_punc) {
					currentData.narratorEndIndex=stateInfo.endPos;
					stateInfo.nextState=TEXT_S;
					return_value=false;
				}
			#endif
			}
			else if(stateInfo.currentType==NAME) {
				currentData.nmcCount=0;
				stateInfo.nextState=NAME_S;

				fillStructure(stateInfo,NAME_PRIM,currentChain);

			#ifdef STATS
				temp_names_per_narrator++;//found another name name
			#endif
			#ifdef PUNCTUATION
				if (stateInfo.currentPunctuationInfo.has_punctuation) {
					display("<punc3>");
					currentData.narratorCount++;
					stateInfo.nextState=NRC_S;
					currentData.nrcCount=0;
					currentData.narratorEndIndex=stateInfo.endPos;
					//currentData.nrcStartIndex=stateInfo.nextPos;//next_positon(stateInfo.endPos,stateInfo.followedByPunctuation);
				/*#ifdef TRYTOLEARN
					stateInfo.nrcIsPunctuation=true;
				#endif*/
					fillStructure(stateInfo,NARRATOR_CONNECTOR,currentChain,true);

					if (ending_punc) {
						currentData.narratorEndIndex=stateInfo.endPos;
						stateInfo.nextState=TEXT_S;
						return_value=false;
						break;
					}
				}
			#endif
			}
		#ifdef PUNCTUATION
			else if (stateInfo.currentPunctuationInfo.has_punctuation) { //TODO: if punctuation check is all what is required
				stateInfo.nextState=NMC_S;
				currentData.nmcCount=hadithParameters.nmc_max+1;
				currentData.nmcValid=false;

				fillStructure(stateInfo,NAME_CONNECTOR,currentChain);

				if (ending_punc) {
					stateInfo.nextState=TEXT_S;

					fillStructure(stateInfo,INITIALIZE,currentChain,true,true);

					currentData.narratorEndIndex=stateInfo.lastEndPos;//TODO: find a better representation
					currentData.narratorCount++;
					return_value=false;
					break;
				}
			}
		#endif
			else if (currentData.nmcCount>hadithParameters.bio_nmc_max
					#ifdef PUNCTUATION
						 || (ending_punc)
					#endif
				) {
				if (currentData.nmcCount>hadithParameters.nmc_max && currentData.nmcValid) {
					currentData.nmcValid=false;
					stateInfo.nextState=NMC_S;
					currentData.nmcCount=0;

					fillStructure(stateInfo,NAME_CONNECTOR,currentChain);

				} else {
					stateInfo.nextState=NRC_S; //was TEXT_S;

					fillStructure(stateInfo,NARRATOR_CONNECTOR,currentChain,true,true);

					// TODO: added this later to the code, check if really is in correct place, but seemed necessary
					currentData.narratorCount++;
				#ifdef STATS
					for (int i=temp_nmc_s.count()-temp_nmc_count;i<temp_nmc_s.count();i++)
					{
						delete temp_nmc_s[i];
						temp_nmc_s.remove(i);
					}
					for (int i=temp_nrc_s.count()-temp_nrc_count;i<temp_nrc_s.count();i++)
					{
						delete temp_nrc_s[i];
						temp_nrc_s.remove(i);
					}
					temp_nmc_count=0;
					temp_nrc_count=0;
				#endif
					//till here was added later

					display("{check}");
					currentData.narratorEndIndex=stateInfo.lastEndPos;//TODO: find a better representation
					//return_value= false;
					break;
				}
				//currentData.narratorEndIndex=stateInfo.lastEndPos;//getLastLetter_IN_previousWord(start_index); check this case

			} else  { //NMC

				fillStructure(stateInfo,NAME_CONNECTOR,currentChain);

				currentData.nmcCount++;
				if (stateInfo.isFamilyConnectorOrPossessivePlace())
					currentData.nmcValid=true;
				stateInfo.nextState=NMC_S;
			#ifdef STATS
				map_entry * entry=new map_entry;
				entry->exact=current_exact;
				entry->stem=current_stem;
				entry->frequency=1;
				temp_nmc_s.append(entry);
				temp_nmc_count++;
			#endif
			#ifdef PUNCTUATION
				if (ending_punc) {
					stateInfo.nextState=TEXT_S;

					//TODO: check why in previous implementation we added the temp_nameConnectors here but not in previous ending_punc
					fillStructure(stateInfo,INITIALIZE,currentChain,true,true);

					currentData.narratorEndIndex=stateInfo.endPos;
					currentData.narratorCount++;
					return_value=false;
					break;
				}
			#endif
			}
			break;

		case NRC_S:
			assertStructure(stateInfo,NARRATOR_CONNECTOR);
		#ifdef REFINEMENTS
			if(reachedRasoul) {
				display("<STOP3>");

				fillStructure(stateInfo,RASOUL_WORD,currentChain);

				//currentData.nrcEndIndex=stateInfo.lastEndPos;//getLastLetter_IN_previousWord(stateInfo.startPos);
				currentData.nmcCount=1;
			#ifdef STATS
				map_entry * entry=new map_entry;
				entry->exact=current_exact;
				entry->stem=current_stem;
				entry->frequency=1;
				temp_nmc_s.append(entry);
				temp_nmc_count++;
			#endif
				currentData.narratorStartIndex=stateInfo.startPos;
				currentData.nmcStartIndex=stateInfo.startPos;

				stateInfo.nextState=STOP_WORD_S;
				currentData.narratorCount++;

				currentData.narratorEndIndex=stateInfo.endPos;
			#ifdef STATS
				for (int i=temp_nmc_s.count()-temp_nmc_count;i<temp_nmc_s.count();i++)
				{
					delete temp_nmc_s[i];
					temp_nmc_s.remove(i);
				}
				for (int i=temp_nrc_s.count()-temp_nrc_count;i<temp_nrc_s.count();i++)
				{
					delete temp_nrc_s[i];
					temp_nrc_s.remove(i);
				}
				temp_nmc_count=0;
				temp_nrc_count=0;
			#endif
				//return_value= false;
				break;
			}
		#endif
		#ifdef REFINEMENTS
			if (stateInfo.currentType==NAME || stateInfo.possessivePlace) {
		#else
			if (stateInfo.currentType==NAME) {
		#endif
				stateInfo.nextState=NAME_S;
				currentData.nrcCount=1;

				currentData.narratorStartIndex=stateInfo.startPos;
				if (stateInfo.currentType==NAME)
					currentData.nameStartIndex=stateInfo.startPos;
				else
					currentData.nmcStartIndex=stateInfo.startPos;

				fillStructure(stateInfo,(stateInfo.currentType==NAME?NAME_PRIM:NAME_CONNECTOR),currentChain);
				stateInfo.processedStructure=NAME_PRIM; //to have consistency with nextState in case it was NAME_CONNECTOR

				//currentData.nrcEndIndex=stateInfo.lastEndPos;//getLastLetter_IN_previousWord(stateInfo.startPos);
			#ifdef STATS
				temp_names_per_narrator++;//found another name
			#endif
			#ifdef PUNCTUATION
				if (stateInfo.currentPunctuationInfo.has_punctuation)
				{
					display("<punc4>");
					currentData.narratorCount++;
					stateInfo.nextState=NRC_S;
					currentData.nrcCount=0;
					currentData.narratorEndIndex=stateInfo.endPos;
					//currentData.nrcStartIndex=stateInfo.nextPos;//next_positon(stateInfo.endPos,stateInfo.followedByPunctuation);
				/*#ifdef TRYTOLEARN
					stateInfo.nrcIsPunctuation=true;
				#endif*/
					fillStructure(stateInfo,NARRATOR_CONNECTOR,currentChain,true);

					if (ending_punc) {
						currentData.narratorEndIndex=stateInfo.endPos;
						stateInfo.nextState=TEXT_S;
						return_value=false;
						break;
					}
					break;
				}
			#endif
			}
		#ifdef PUNCTUATION
			else if (currentData.nrcCount>=hadithParameters.bio_nrc_max ||stateInfo.number) { //if not in refinements mode stateInfo.number will always remain false
		#else
			else if (currentData.nrcCount>=hadithParameters.bio_nrc_max) {
		#endif
				stateInfo.nextState=TEXT_S;
			#ifdef STATS
				for (int i=temp_nmc_s.count()-temp_nmc_count;i<temp_nmc_s.count();i++)
				{
					delete temp_nmc_s[i];
					temp_nmc_s.remove(i);
				}
				for (int i=temp_nrc_s.count()-temp_nrc_count;i<temp_nrc_s.count();i++)
				{
					delete temp_nrc_s[i];
					temp_nrc_s.remove(i);
				}
				temp_nmc_count=0;
				temp_nrc_count=0;
			#endif
				fillStructure(stateInfo,INITIALIZE,currentChain,true,true); //just to delete dangling NARRATOR_CONNECTOR

				return_value= false;
				break;
			}
		#ifdef IBN_START
			else if (stateInfo.currentType==NMC && stateInfo.familyNMC) {
				display("<Family3>");

				fillStructure(stateInfo,NAME_CONNECTOR,currentChain);

				//currentData.nrcEndIndex=stateInfo.lastEndPos;//getLastLetter_IN_previousWord(stateInfo.startPos);
				currentData.nmcValid=true;
				currentData.nmcCount=1;
			#ifdef STATS
				map_entry * entry=new map_entry;
				entry->exact=current_exact;
				entry->stem=current_stem;
				entry->frequency=1;
				temp_nmc_s.append(entry);
				temp_nmc_count++;
			#endif
				currentData.narratorStartIndex=stateInfo.startPos;
				currentData.nmcStartIndex=stateInfo.startPos;
				stateInfo.nextState=NMC_S;
			#ifdef PUNCTUATION
				if (stateInfo.currentPunctuationInfo.has_punctuation)
				{
					display("<punc5>");
					currentData.narratorCount++;
					stateInfo.nextState=NRC_S;
					currentData.nrcCount=0;
					currentData.narratorEndIndex=stateInfo.endPos;
					//currentData.nrcStartIndex=stateInfo.nextPos;//next_positon(stateInfo.endPos,stateInfo.followedByPunctuation);
				#ifdef TRYTOLEARN
					stateInfo.nrcIsPunctuation=true;
				#endif

					fillStructure(stateInfo,NARRATOR_CONNECTOR,currentChain,true);

					if (ending_punc) {
						currentData.narratorEndIndex=stateInfo.endPos;
						stateInfo.nextState=TEXT_S;
						return_value=false;
						break;
					}
					break;
				}
			#endif
			}
		#endif
			else {
				stateInfo.nextState=NRC_S;

				fillStructure(stateInfo,NARRATOR_CONNECTOR,currentChain);

				currentData.nrcCount++;
			#ifdef STATS
				map_entry * entry=new map_entry;
				entry->exact=current_exact;
				entry->stem=current_stem;
				entry->frequency=1;
				temp_nrc_s.append(entry);
				temp_nrc_count++;
			#endif
			#ifdef PUNCTUATION
				if (ending_punc) {
					stateInfo.nextState=TEXT_S;

					fillStructure(stateInfo,NARRATOR_CONNECTOR,currentChain,true,true);

					currentData.narratorEndIndex=stateInfo.endPos;
					return_value=false;
					break;
				}
			#endif
			#ifdef REFINEMENTS
				if (stateInfo._3an) {
					currentData.nrcCount=1;
					//currentData.nrcEndIndex=stateInfo.endPos;

					assertStructure(stateInfo,NARRATOR_CONNECTOR);
					fillStructure(stateInfo,NAME_PRIM,currentChain,true);

					stateInfo.nextState=NAME_S;
					break;
				}
			#endif
			}
			break;
	#ifdef REFINEMENTS
		case STOP_WORD_S:
			assertStructure(stateInfo,RASOUL_WORD);
			if (stateInfo.currentType==STOP_WORD) {
				stateInfo.nextState=STOP_WORD_S;

				fillStructure(stateInfo,RASOUL_WORD,currentChain);

				currentData.narratorEndIndex=stateInfo.endPos;
			#ifdef PUNCTUATION
				if (ending_punc) {

					fillStructure(stateInfo,INITIALIZE,currentChain);
					stateInfo.nextState=TEXT_S;
					return_value=false;
					break;
				}
			#endif
			} else {
				fillStructure(stateInfo,NARRATOR_CONNECTOR,currentChain);

				stateInfo.nextState=NRC_S; //was TEXT_S
				//return_value=false;
			}
			break;
	#endif
		default:
			break;
		}
		display("\n");
	#ifdef REFINEMENTS
		currentData.ibn_or_3abid=stateInfo.familyConnectorOr3abid(); //for it to be saved for next time use
	#endif
		if (!return_value /*&& stateInfo.processedStructure!=INITIALIZE*/)
			fillStructure(stateInfo,INITIALIZE,currentChain);
		else if (return_value==false)
			assert (currentChain->narrator==NULL);
		return return_value;
	}

	inline bool result(WordType t, StateInfo &  stateInfo,BiographyData *currentBiography){display(t); stateInfo.currentType=t; return getNextState(stateInfo,currentBiography);}
	bool proceedInStateMachine(StateInfo &  stateInfo,BiographyData *currentBiography) { //does not fill stateInfo.currType
		hadith_stemmer s(text,stateInfo.startPos);
		if (stateInfo.familyNMC)
			s.tryToLearnNames=true;
		stateInfo.resetCurrentWordInfo();
		long  finish;
		stateInfo.possessivePlace=false;
		stateInfo.resetCurrentWordInfo();
	#if 0
		static hadith_stemmer * s_p=NULL;
		if (s_p==NULL)
			s_p=new hadith_stemmer(text,stateInfo.startPos);
		else
			s_p->init(stateInfo.startPos);
		hadith_stemmer & s=*s_p;
	#endif
	#ifdef REFINEMENTS
	#ifdef TRYTOLEARN
		if (stateInfo.currentState==NRC_S && currentData.nrcCount<=1
			#ifdef PUNCTUATION
				&& !stateInfo.nrcIsPunctuation
			#endif
			)
			s.tryToLearnNames=true;
	#endif
	#ifdef PUNCTUATION
		if (isNumber(text,stateInfo.startPos,finish)) {
			display("Number ");
			stateInfo.number=true;
			stateInfo.endPos=finish;
			stateInfo.nextPos=next_positon(text,finish+1,stateInfo.currentPunctuationInfo);
			display(text->mid(stateInfo.startPos,finish-stateInfo.startPos+1)+":");
			return result(NMC,stateInfo,currentBiography);
		}
	#endif

		QString c;
		bool found,phrase=false,stop_word=false;
		foreach (c, rasoul_words) {
			int pos;
			if (startsWith(text->midRef(stateInfo.startPos),c,pos))	{
				stop_word=true;
				found=true;
				finish=pos+stateInfo.startPos;
				break;
			}
		}
		if (!stop_word) {//TODO: maybe modified to be set as a utility function, and just called from here
			foreach (c, compound_words)	{
				int pos;
				if (startsWith(text->midRef(stateInfo.startPos),c,pos))	{
					phrase=true;
					found=true;
					finish=pos+stateInfo.startPos;
					break;
				}
			}
		}
		if (!stop_word && !phrase)	{
			s();
			finish=max(s.info.finish,s.finish_pos);
			if (finish==stateInfo.startPos) {
				finish=getLastLetter_IN_currentWord(text,stateInfo.startPos);
			#ifdef REFINEMENTS
				if (s.tryToLearnNames && removeDiacritics(text->mid(stateInfo.startPos,finish-stateInfo.startPos+1)).count()>=3) {
					s.name=true;
					s.finishStem=finish;
					s.startStem=stateInfo.startPos;
					s.learnedName=true;
				}
			#endif
			}
	#ifdef REFINEMENTS
		}
	#endif
		stateInfo.endPos=finish;
		stateInfo.nextPos=next_positon(text,finish,stateInfo.currentPunctuationInfo);
		display(text->mid(stateInfo.startPos,finish-stateInfo.startPos+1)+":");
	#ifdef REFINEMENTS
		if (stop_word || s.stopword) {
			return result(STOP_WORD,stateInfo,currentBiography);
		}
		if (phrase)
		{
			display("PHRASE ");
			//isBinOrPossessive=true; //same behaviour as Bin
			return result(NMC,stateInfo,currentBiography);
		}
		stateInfo._3abid=s._3abid;
	#endif
	#ifdef TRYTOLEARN
		if (!s.name && !s.nmc && !s.nrc &&!s.stopword) {
			QString word=s.getString().toString();
			if (stateInfo.currentPunctuationInfo.has_punctuation && s.tryToLearnNames && removeDiacritics(word).count()>=3) {
				display("{learned}");
				s.name=true;
				s.nmc=false;
				s.learnedName=true;
				s.startStem=s.info.start;
				s.finishStem=s.info.finish;
			}
		}
	#endif
		stateInfo.possessivePlace=s.possessive;
		if (s.nrc ) {
		#ifdef REFINEMENTS
			if (s.is3an) {
			#if 0
				PunctuationInfo copyPunc=stateInfo.punctuationInfo;
				display(" [An] ");
				stateInfo._3an=true;
				stateInfo.startPos=s.startStem;
				stateInfo.endPos=s.finishStem;
				stateInfo.punctuationInfo.reset();
				if (s.finishStem<finish)
					stateInfo.nextPos=s.finishStem+1;
				else
					stateInfo.punctuationInfo=copyPunc;
				if (!result(NRC,stateInfo,currentChain))
					return false;
				stateInfo.currentState=stateInfo.nextState;
				stateInfo.lastEndPos=stateInfo.endPos;
				if (s.finishStem<finish) {
					stateInfo._3an=false;
					stateInfo.startPos=s.finishStem+1;
					stateInfo.endPos=finish;
					return result(NAME,stateInfo,currentChain);
				}
				return true;
			#else
				if (s.finishStem==finish) {
					display(" [An] ");
					stateInfo._3an=true;
				}
			#endif
			}
		#endif
			return result(NRC,stateInfo,currentBiography);
		}
		else if (s.nmc)
		{
			if (s.familyNMC) {
			#if defined(GET_WAW) || defined(REFINEMENTS)
				PunctuationInfo copyPunc=stateInfo.currentPunctuationInfo;
			#endif
			#ifdef GET_WAW
				long nextpos=stateInfo.nextPos;
				if (s.has_waw && (stateInfo.currentState==NAME_S ||stateInfo.currentState==NRC_S)) {
					display("waw ");
					stateInfo.isWaw=true;
					stateInfo.startPos=s.wawStart;
					stateInfo.endPos=s.wawEnd;
					stateInfo.nextPos=s.wawEnd+1;
					stateInfo.currentPunctuationInfo.reset();
					if (!result(NRC,stateInfo,currentBiography))
						return false;
					stateInfo.isWaw=false;
					stateInfo.currentState=stateInfo.nextState;
					stateInfo.lastEndPos=stateInfo.endPos;
				}
			#endif
			#ifdef REFINEMENTS
				display("FamilyNMC ");
				stateInfo.familyNMC=true;
				if (s.ibn)
					stateInfo.ibn=true;
				if (s._2ab)
					stateInfo._2ab=true;
				if (s._2om)
					stateInfo._2om=true;
				stateInfo.startPos=s.startStem;
				stateInfo.endPos=s.finishStem;
				stateInfo.currentPunctuationInfo.reset();
				long startSuffix=s.finishStem+1;
				bool isRelativeReference=s.finishStem<finish && suffixNames.contains(text->mid(startSuffix,finish-startSuffix+1));
				if (isRelativeReference)
					stateInfo.nextPos=startSuffix;
				else {
					stateInfo.currentPunctuationInfo=copyPunc;
					stateInfo.nextPos=nextpos;
				}
				if (!result(NMC,stateInfo,currentBiography))
					return false;
				stateInfo.currentState=stateInfo.nextState;
				stateInfo.lastEndPos=stateInfo.endPos;
				if (isRelativeReference) {
					stateInfo.familyNMC=false;
					stateInfo.ibn=false;
					stateInfo.startPos=startSuffix;
					stateInfo.endPos=finish;
					stateInfo.nextPos=nextpos;
					stateInfo.currentPunctuationInfo=copyPunc;
					return result(NAME,stateInfo,currentBiography);
				}
				return true;
			#else
				return result(NMC,stateInfo,currentBiography);
			#endif
			}
			if (s.possessive) {
				display("Possessive ");
				return result(NMC,stateInfo,currentBiography);
			}
			return result(NMC,stateInfo,currentBiography);
		}
		else if (s.name){
		#ifdef GET_WAW
			long nextpos=stateInfo.nextPos;
			PunctuationInfo copyPunc=stateInfo.currentPunctuationInfo;
			if (s.has_waw && (stateInfo.currentState==NAME_S ||stateInfo.currentState==NRC_S) ) {
				display("waw ");
				stateInfo.isWaw=true;
				stateInfo.startPos=s.wawStart;
				stateInfo.endPos=s.wawEnd;
				stateInfo.nextPos=s.wawEnd+1;
				stateInfo.currentPunctuationInfo.reset();
				if (!result(NRC,stateInfo,currentBiography))
					return false;
				stateInfo.isWaw=false;
				stateInfo.currentState=stateInfo.nextState;
				stateInfo.lastEndPos=stateInfo.endPos;
			}
			stateInfo.currentPunctuationInfo=copyPunc;
		#endif
		#ifdef REFINEMENTS
			stateInfo.learnedName=s.learnedName;
			stateInfo.startPos=s.startStem;
			stateInfo.endPos=s.finishStem;
			stateInfo.nextPos=nextpos;
		#endif
			return result(NAME,stateInfo,currentBiography);
		}
		else
			return result(NMC,stateInfo,currentBiography);
	}
#ifdef SEGMENT_BIOGRAPHY_USING_POR
	class ReachableVisitor:public NodeVisitor {
		NarratorNodeIfc * target;
		ColorIndices & colorGuard;
		bool found;
	public:
		ReachableVisitor(NarratorNodeIfc * aTarget,ColorIndices & guard):colorGuard(guard) {target=aTarget;}
		void initialize(){
			found=false;
		}
		virtual void visit(NarratorNodeIfc & ,NarratorNodeIfc & , int) {	}
		virtual void visit(NarratorNodeIfc & n) {
			if (&n==target) {
				found=true;
				colorGuard.setAllNodesVisited(controller->getVisitColorIndex()); //to stop further traversal
				colorGuard.setAllNodesVisited(controller->getFinishColorIndex());
			}
		}
		virtual void finishVisit(NarratorNodeIfc & ){ }
		virtual void detectedCycle(NarratorNodeIfc & ){ }
		virtual void finish(){	}
		bool isFound() {return found;}
	};
	class Cluster;
	class NodeItem {
	public:
		NarratorNodeIfc * node;
		Cluster* cluster;
		int inDegree;
		int outDegree;
		NodeItem(NarratorNodeIfc * n) {
			node=n;
			cluster=NULL;
			inDegree=0;
			outDegree=0;
		}
	};
	typedef QList<NodeItem *> NodeItemList;
	typedef QList<NodeItemList> NodeItemGroup;
	class Cluster {
	public:
		int id;
		NodeItemList list;
		Cluster(NodeItem * n1,NodeItem * n2,int cluster_id){
			list.append(n1);
			list.append(n2);
			n1->cluster=this;
			n2->cluster=this;
			id=cluster_id;
		}
		void addNodeItem(NodeItem * n) {
			list.append(n);
			n->cluster=this;
		}
		int size() {
			return list.size();
		}
		NodeItem * operator[](int i){
			return list[i];
		}
		~Cluster() {
			for (int i=0;i<list.size();i++)
				delete list[i];
		}
	};
	class ClusterList {
	private:
		QList<Cluster *> list;
	public:
		void addCluster(Cluster * c,int id=-1){
			if (id<0 && c->id>=0)
				id=c->id;
			if (id>=0 && c->id<0)
				c->id=id;
			if (id<0 && c->id<0) {
				id=list.size();
				c->id=id;
			}
			assert(c->id==id);
			assert(list.size()<=id || list[id]==NULL);
			for (int i=list.size();i<=id;i++)
				list.append(NULL);
			list[id]=c;
		}
		int size() {
			return list.size();
		}
		Cluster * operator[](int i){
			return list[i];
		}
		~ClusterList() {
			for (int i=0;i<list.size();i++)
				if (list[i]!=NULL)
					delete list[i];
		}
		void mergeNodeItems(NodeItem * n1,NodeItem * n2) {
			if (n1->cluster==NULL && n2->cluster==NULL){
				Cluster * c=new Cluster(n1,n2,size());
				addCluster(c);
			} else if (n1->cluster!=NULL && n2->cluster==NULL) {
				n1->cluster->addNodeItem(n2);
				assert(n2->cluster==n1->cluster);
			} else if (n2->cluster!=NULL && n1->cluster==NULL) {
				n2->cluster->addNodeItem(n1);
				assert(n2->cluster==n1->cluster);
			} else {//in both cases:n1!=n2 or n1==n2
				return;
			}
		}
	};


	bool near(NarratorNodeIfc *n1, NarratorNodeIfc *n2) {
	#if 0
		ChainNodeIterator itr=n1.begin();
		for (;!itr.isFinished();++itr) {
			if (&itr.getChild()==&n2)
				return true;
			if (&itr.getParent()==&n2)
				return true;
		}
		return false;
	#else
		//qDebug()<<"--test if near--("<<n1->CanonicalName()<<","<<n2->CanonicalName()<<")";
		if (n1==n2)
			return false;
		ReachableVisitor v(n2,graph->colorGuard);
		GraphVisitorController controller(&v,graph);
		graph->BFS_traverse(controller,hadithParameters.bio_max_reachability,n1,1);
		bool found=v.isFound();
		if (!found) {
			graph->BFS_traverse(controller,hadithParameters.bio_max_reachability,n1,-1);
			found=v.isFound();
		}
		return found;
	#endif
	}
	bool near(NarratorNodeIfc * n, const NarratorNodeList & list) {
		for (int i=0;i<list.size();i++) {
			if (near(n,list[i]))
				return true;
		}
		return false;
	}
	int getLargestClusterSize(Biography::NarratorNodeGroups & list) {
		NodeItemGroup nodeItemgroups;
		//1-transform to nodeItems:
		for (int i=0;i<list.size();i++) {
			int size=list[i].size();
			if (size>0) {
				nodeItemgroups.append(NodeItemList());
				for (int j=0;j<size;j++) {
					NodeItem * node=new NodeItem(list[i][j]);
					nodeItemgroups[i].append(node);
				}
			}
		}
		ClusterList clusters;
		//1-each node put it in its own list or merge it with a group of already found if it is near them
		int size=nodeItemgroups.size();
		for (int i=0;i<size;i++) {
			for (int j=i+1;j<size;j++) {
				int size1=nodeItemgroups[i].size();
				int size2=nodeItemgroups[j].size();
				for (int i2=0;i2<size1;i2++) {
					for (int j2=0;j2<size2;j2++) {
						NodeItem * item1=nodeItemgroups[i][i2],
								 * item2=nodeItemgroups[j][j2];
						if (near(item1->node,item2->node)) {
							clusters.mergeNodeItems(item1,item2);
						}
					}
				}
			}
		}
		//3-return largest list size
		int largest=1;
		int index=-1;
		for (int i=0;i<clusters.size();i++) {
			if (clusters[i]->size()>largest) {
				largest=clusters[i]->size();
				index=i;
			}
		}
	#if 1
		if (index>=0) {
			qDebug()<<largest<<"\n";
			for (int i=0;i<clusters[index]->size();i++){
				qDebug()<<(*clusters[index])[i]->node->CanonicalName()<<"\n";
			}
			qDebug()<<"\n";
		}
	#endif
		return largest;
	}

#endif

public:
#ifdef SEGMENT_BIOGRAPHY_USING_POR
	NarratorDetector(NarratorGraph * graph) {this->graph=graph; }
	NarratorDetector() {}
#else
	NarratorDetector() {graph=NULL;}
#endif
	int segment(QString input_str,ATMProgressIFC *prg)  {
		QFile chainOutput(chainDataStreamFileName);

		chainOutput.remove();
		if (!chainOutput.open(QIODevice::ReadWrite))
			return 1;
		QDataStream chainOut(&chainOutput);
		QFile input(input_str);
		if (!input.open(QIODevice::ReadOnly)) {
			out << "File not found\n";
			return 1;
		}
		QTextStream file(&input);
		file.setCodec("utf-8");
		text=new QString(file.readAll());
		if (text==NULL)	{
			out<<"file error:"<<input.errorString()<<"\n";
			return 1;
		}
		if (text->isEmpty()) { //ignore empty files
			out<<"empty file\n";
			return 0;
		}
		long text_size=text->size();
		long  biographyStart=-1;
		currentData.initialize();

	#ifdef CHAIN_BUILDING
		BiographyData *currentBiography=new BiographyData();
		currentBiography->initialize(graph,text);
		display(QString("\ninit%1\n").arg(currentBiography->narrator->m_narrator.size()));
	#else
		chainData *currentBiography=NULL;
	#endif
		long  biographyEnd;
		int biography_Counter=1;
	#endif
		StateInfo stateInfo;
		stateInfo.resetCurrentWordInfo();
		stateInfo.currentState=TEXT_S;
		stateInfo.nextState=TEXT_S;
		stateInfo.lastEndPos=0;
		stateInfo.startPos=0;
	#ifdef PUNCTUATION
		stateInfo.previousPunctuationInfo.fullstop=true;
	#endif
		while(stateInfo.startPos<text->length() && isDelimiter(text->at(stateInfo.startPos)))
			stateInfo.startPos++;
	#ifdef PROGRESSBAR
		prg->setCurrentAction("Parsing Biography");
	#endif
		for (;stateInfo.startPos<text_size;) {
			if((proceedInStateMachine(stateInfo,currentBiography)==false)) {
			#ifdef SEGMENT_BIOGRAPHY_USING_POR
				Biography::NarratorNodeGroups & realNarrators=currentBiography->biography->nodeGroups;
				int num=getLargestClusterSize(realNarrators);
				if (num>=hadithParameters.bio_narr_min) {
			#else
				if (currentData.narratorCount>=hadithParameters.bio_narr_min) {
			#endif
					//biographyEnd=currentData.narratorEndIndex;
					biographyEnd=stateInfo.endPos;
					currentBiography->biography->setEnd(biographyEnd);
				#ifdef DISPLAY_HADITH_OVERVIEW
					//biographyStart=currentData.biographyStartIndex;
					biographyStart=currentBiography->biography->getStart();
					//long end=text->indexOf(QRegExp(delimiters),sanadEnd);//sanadEnd is first letter of last word in sanad
					//long end=stateInfo.endPos;
					out<<"\n"<<biography_Counter<<" new biography start: "<<text->mid(biographyStart,display_letters)<<endl;
					out<<"sanad end: "<<text->mid(biographyEnd-display_letters+1,display_letters)<<endl<<endl;
				#ifdef CHAIN_BUILDING
					currentBiography->biography->serialize(chainOut);
					currentBiography->biography->setStart(stateInfo.nextPos);
					//currentChain->chain->serialize(displayed_error);
				#endif
				#endif
					biography_Counter++;
				}
			}
			stateInfo.currentState=stateInfo.nextState;
			stateInfo.startPos=stateInfo.nextPos;
			stateInfo.lastEndPos=stateInfo.endPos;
		#ifdef PUNCTUATION
			stateInfo.previousPunctuationInfo=stateInfo.currentPunctuationInfo;
			if (stateInfo.number) {
				stateInfo.previousPunctuationInfo.fullstop=true;
				stateInfo.previousPunctuationInfo.has_punctuation=true;
			}
			/*if (stateInfo.previousPunctuationInfo.has_punctuation)
				stateInfo.previousPunctuationInfo.fullstop=true;*/
			if (stateInfo.previousPunctuationInfo.fullstop) {
				if (currentBiography->biography!=NULL)
					delete currentBiography->biography;
				currentBiography->biography=new Biography(graph,text,stateInfo.startPos);
			}


		#endif
	#ifdef PROGRESSBAR
			prg->report((double)stateInfo.startPos/text_size*100+0.5);
			if (stateInfo.startPos==text_size-1)
				break;
	#endif
		}
	#if defined(DISPLAY_HADITH_OVERVIEW)
		if (biographyStart<0)
		{
			out<<"no biography found\n";
			chainOutput.close();
			return 2;
		}
		chainOutput.close();
	#endif
	#ifdef CHAIN_BUILDING //just for testing deserialize
		QFile f("biography_chains.txt");
		if (!f.open(QIODevice::WriteOnly))
			return 1;
		QTextStream file_biography(&f);
			file_biography.setCodec("utf-8");

		if (!chainOutput.open(QIODevice::ReadWrite))
			return 1;
		QDataStream tester(&chainOutput);
		int tester_Counter=1;
	#ifdef TEST_BIOGRAPHIES
		biographies=new BiographyList;
		biographies->clear();
	#endif
	#if defined(TAG_HADITH)
		prg->startTaggingText(*text);
	#endif
		while (!tester.atEnd())
		{
			Biography * s=new Biography(graph,text);
			s->deserialize(tester);
		#ifdef TEST_BIOGRAPHIES
			biographies->append(s);
		#endif
		#if defined(TAG_HADITH)
			for (int j=0;j<s->size();j++)
			{
				const Narrator * n=(*s)[j];
				if (n->m_narrator.size()==0) {
					out<<"found a problem an empty narrator in ("<<tester_Counter<<","<<j<<")\n";
					continue;
				}
				if (s->isReal(j))
					prg->tag(n->getStart(),n->getLength(),Qt::darkYellow,false);
				else
					prg->tag(n->getStart(),n->getLength(),Qt::darkGray,false);
				for (int i=0;i<n->m_narrator.size();i++)
				{
					NarratorPrim * nar_struct=n->m_narrator[i];
					if (nar_struct->isNamePrim()) {
						if (((NamePrim*)nar_struct)->learnedName) {
							prg->tag(nar_struct->getStart(),nar_struct->getLength(),Qt::blue,true);
							//error<<nar_struct->getString()<<"\n";
						}
						else
							prg->tag(nar_struct->getStart(),nar_struct->getLength(),Qt::white,true);
					}
					else if (((NameConnectorPrim *)nar_struct)->isFamilyConnector())
						prg->tag(nar_struct->getStart(),nar_struct->getLength(),Qt::darkRed,true);
					else if (((NameConnectorPrim *)nar_struct)->isPossessive())
						prg->tag(nar_struct->getStart(),nar_struct->getLength(),Qt::darkMagenta,true);
				}
			}
		#else
			hadith_out<<tester_Counter<<" ";
			s->serialize(hadith_out);
		#endif
			tester_Counter++;
			s->serialize(file_biography);
		}
		chainOutput.close();
		f.close();
	#endif
	#ifndef TAG_HADITH
		prg->startTaggingText(*hadith_out.string()); //we will not tag but this will force a text to be written there
	#else
		prg->finishTaggingText();
	#endif
		if (currentBiography!=NULL)
			delete currentBiography;
		return 0;
	}

	void freeMemory() { //called if we no longer need stuctures of this class
		for (int i=0;i<biographies->size();i++)
			delete (*biographies)[i];
		delete text;
	}
};

int biographyHelper(QString input_str,ATMProgressIFC *prg) {
	input_str=input_str.split("\n")[0];
	NarratorDetector s;
	s.segment(input_str,prg);
#ifdef TEST_BIOGRAPHIES
	s.freeMemory();
#endif
	return 0;
}

#ifdef TEST_BIOGRAPHIES
BiographyList * getBiographies(QString input_str,NarratorGraph* graph,ATMProgressIFC *prg) {
	input_str=input_str.split("\n")[0];
#ifdef SEGMENT_BIOGRAPHY_USING_POR
	NarratorDetector s(graph);
#else
	NarratorDetector s;
#endif
	s.segment(input_str,prg);
	return s.biographies;
}
#endif
