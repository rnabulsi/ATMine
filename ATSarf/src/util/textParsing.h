#ifndef TEXTPARSING_H
#define TEXTPARSING_H
#include "hadith.h"
#include "letters.h"
#include "diacritics.h"

class PunctuationInfo {
public:
	bool has_punctuation:1;
	bool comma:1;
	bool semicolon:1;
	bool fullstop:1;
	bool newLine:1;
	bool colon:1;
	bool dash:1;
	PunctuationInfo() {
		reset();
	}
	void reset() {
		has_punctuation=false;
		comma=false;
		colon=false;
		semicolon=false;
		fullstop=false;
		newLine=false;
		dash=false;
	}
	bool hasEndingPunctuation() { return fullstop || newLine;}
	bool hasParagraphPunctuation() { return fullstop && newLine;}
	bool update(const QChar & letter) { //returns true if this letter is a delimiter
		if (non_punctuation_delimiters.contains(letter))
			return true;
		else if (punctuation.contains(letter)) {
			has_punctuation=true;
			if (letter==',' || letter ==fasila)
				comma=true;
			else if (letter=='\n' || letter =='\r' || letter==paragraph_seperator)
				newLine=true;
			else if (letter==semicolon_ar || letter ==';')
				semicolon=true;
			else if (letter==full_stop1 || letter==full_stop2 || letter==full_stop3 || letter==question_mark || letter=='.' || letter=='?')
				fullstop=true;
			else if (letter==':' || letter==colon_raised || letter==colon_modifier)
				colon=true;
			else if (letter=='-')
				dash=true;
			return true;
		} else
			return false;
	}
};

inline long next_positon(QString * text,long finish,PunctuationInfo & punctuationInfo) {
	punctuationInfo.reset();
	int size=text->length();
	if (finish>=size)
		return finish+1;//check this
	QChar letter;
	if (finish>=0) {
		letter=text->at(finish);
	#ifdef PUNCTUATION
		punctuationInfo.update(letter);
	#endif
	} else {
		letter='\0';
	}
	finish++;
	while(finish<size) {
		letter=text->at(finish);
	#ifdef PUNCTUATION
		if (!punctuationInfo.update(letter)) // update returns true if letter is a delimiter
			break;
	#else
		if (!delimiters.contains(letter))
			break;
	#endif
		finish++;
	}
	return finish;
}

inline long getLastLetter_IN_previousWord(QString * text,long start_letter_current_word) {
	start_letter_current_word--;
	while(start_letter_current_word>=0 && isDelimiter(text->at(start_letter_current_word)))
		start_letter_current_word--;
	return start_letter_current_word;
}

inline long getLastLetter_IN_currentWord(QString * text,long start_letter_current_word) {
	int size=text->length();
#if 0
	if (!isDelimiter(text->at(start_letter_current_word)))
		start_letter_current_word++;
#endif
	bool first=true;
	while(start_letter_current_word<size) {
		if(!isDelimiter(text->at(start_letter_current_word)))
			start_letter_current_word++;
		else {
			if (!first)
				start_letter_current_word--;
			break;
		}
		first=false;
	}
	return start_letter_current_word;
}

inline bool isNumber(QString * text,long & currentPos,long & finish) {
	bool ret_val=false;
	long size=text->size();
	long i;
	for (i=currentPos;i<size;i++) {
		if (isNumber(text->at(i)))
			ret_val=true;
		else {
			finish=max(i-1,currentPos);
			return ret_val;
		}
	}
	finish=i; //means (still number and text finished) or did not enter the loop at all
	return ret_val;
}

inline QString getLastNWords(QString s, int N) {
	int l=s.size();
	int count=0;
	do {
		l=s.lastIndexOf(' ',l-1);
		if (l<0) {
			l=0;
			break;
		}
		count++;
	} while (count<N);
	return s.mid(l-1);
}

#endif // TEXTPARSING_H
