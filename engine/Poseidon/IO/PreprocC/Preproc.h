#if !defined(AFX_PREPROC_H__E6DF0FA5_8BA9_4E88_94E8_F3BDDEAE5346__INCLUDED_)
#define AFX_PREPROC_H__E6DF0FA5_8BA9_4E88_94E8_F3BDDEAE5346__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Poseidon/IO/Streams/QStream.hpp>

#include <Poseidon/Foundation/Containers/HashMap.hpp>




namespace Poseidon
{
enum PreprocessError
  {
  prNoError,
  prStreamOpenError, //OnEnterInclude vratil nullptr
  prIncludeError, //Spatna syntaxe include
  prIncludeMaxRecursion, //Maximalni pocet rekurzi byl dosazen
  prDefineError, //Spatna syntaxe define
  prDefineParamError, //Spatna syntaxe parametru
  prParseExit, //Interni
  prInvalidPreprocessorCommand, //preprocesor nerozpoznal prikaz
  prUnexceptedEndOfFile,
  prToManyParameters,
  prToFewParameters,
  prUnexceptedSymbol, 
  prEndIfExcepted,  
  };


enum PreprocLexItem
  {
  lxInclude,
  lxDefine,
  lxIfDef,
  lxIfNDef,
  lxElse,
  lxEndIf,
  lxLeft,
  lxRight,
  lxComma,
  lxHash,
  lxNewLine,
  lxLineBreak,
  lxBeginLineComment,
  lxBeginBlockComment,
  lxText,
  lxUnknown,
  lxEof,
  lxUhozy,
  lxNoUhozy,
  lxLevaZlomena,
  lxPravaZlomena,
  lx2Hash,
  lxUndef,
  };


class Preproc  
  {
  QOStream *out;
  
  public:

  class Symbol
	{
	char *key;
	public:
	  Symbol() {key=strDup("00NULL00");}
	  Symbol(const char *x) {key=strDup(x);}
	  Symbol(const Symbol &other) {key=strDup(other.key);}
	  Symbol& operator=(const Symbol &other)
    {
      char *nkey=strDup(other.key);
      free(key);
      key = nkey;
      return *this;
    }
	  virtual ~Symbol() {free(key);key = nullptr;}
	  const char *GetKey() const {return key;}

      bool operator == ( const Symbol &src ) const
		{
		return strcmp(key,src.key)==0;
		}
      bool operator != ( const Symbol &src ) const
		{
		return strcmp(key,src.key)!=0;
		}
	};

	
	class MacroParam:public Symbol
	  {
	  char *value;
	  public:
		MacroParam() {value=nullptr;}
		MacroParam(const char *x) : Symbol(x) {value=nullptr;}
		MacroParam(const MacroParam &other):Symbol(other) {value=strDup(other.value);}
		MacroParam& operator=(const MacroParam &other)
    {
      Symbol::operator=(other);
      char *nvalue=strDup(other.value);
      free(value);
      value = nvalue;
      return *this;
    }
		~MacroParam() override {free(value); value = nullptr;}
		void SetValue(const char *val);	  
		const char *GetValue() const {return value;}	

		// Safe to use memmove despite virtual destructor (proper copy/assignment defined)
		#pragma clang diagnostic push
		#pragma clang diagnostic ignored "-Wdynamic-class-memaccess"
    #pragma clang diagnostic pop
	};


	typedef MapStringToClass<MacroParam, AutoArray<MacroParam> > MacroParams;

	class DefineSymb:public MacroParam
	  {
	  MacroParams params;
	  int blocked;
	  public:
		DefineSymb() {blocked=1;}
		DefineSymb(const char *x,const char *expand):MacroParam(x)
		  {
		  SetValue(expand);
		  blocked=1;
		  }
		DefineSymb(const DefineSymb &other) = default;		
		DefineSymb& operator=(const DefineSymb &other) 
		  {params=other.params;MacroParam::operator=(other);return *this;}

	  public: 
		const char *GetParam(const char *name);
		void AddParam(const char *name, int poradi);
  		bool SetParam(int poradi, const char *text, MacroParams& parlist);
		void Block() {blocked++;}
		void Unblock() {if (blocked>0) { blocked--;
}}
		bool Blocked() {return blocked!=0;}

		// Safe to use memmove despite virtual destructor (proper copy/assignment defined)
		#pragma clang diagnostic push
		#pragma clang diagnostic ignored "-Wdynamic-class-memaccess"
    #pragma clang diagnostic pop
	  };

  typedef MapStringToClass<DefineSymb, AutoArray<DefineSymb> > DefTable;

  class MacTableList:public MacroParams
	{
	public:
	  MacTableList *next;
	  MacTableList() {next=nullptr;}
	  const MacroParam& GetFromList(const char *name)
		{
		const MacroParam &out=Get(name);
		if (IsNull(out) && next!=nullptr) { return next->GetFromList(name);
}
		return out;
		}
	  MacTableList *Add(MacTableList *item) {item->next=this;return item;}
	  MacTableList *Remove() {MacTableList *out=next;next=nullptr;delete this;return out;}
	  virtual ~MacTableList() {delete next;}
	};

  protected:
	int recurse;
	DefTable deftable;  //tabulka #define
	MacTableList *maclist; //seznam parametru pro expanzi makra (lokalni vyhledavaci tabulky)
	
	PreprocLexItem item;
	char text[128];
  public:
	int curline;
	RString filename;
	PreprocessError error;
	int maxrecurse;
  public:
  	Preproc();
	bool Process(QOStream *out,const char *name);

  protected:
	virtual void AtBeginLine(QOStream &out) {}
	virtual QIStream *OnEnterInclude(const char *filename)=0;
	virtual void OnExitInclude(QIStream *stream)=0;

  private:
	bool DoIfDefBlock(QIStream &in, bool cond);
	bool TryExpandMacro(QIStream& in,QOStream &out);
	void ReadDefineText(QIStream &str, DefineSymb& def);
	void ReadDefineParams(QIStream &str,DefineSymb& def);
	bool LoadMacroParam(RString& p,QIStream &in);
    DefineSymb * CreateExpandMacro(QIStream &in, MacTableList **params); //text obsahuje jmeno makra
	bool DoDefineBlock(QIStream &in);
	bool DoIncludeBlock(QIStream& in);
	void ReadNext(QIStream& in);
	bool GlobalScan(QIStream& in,QOStream *out);
	bool DoUndefBlock(QIStream &str);
  };



#endif // !defined(AFX_PREPROC_H__E6DF0FA5_8BA9_4E88_94E8_F3BDDEAE5346__INCLUDED_)
} // namespace Poseidon
