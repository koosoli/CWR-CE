
#include <Poseidon/UI/Locale/AutoComplete.hpp>
#include <Poseidon/Foundation/Containers/BankArray.hpp>
#include <ctype.h>
#include <Evaluator/express.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/platform.hpp>

namespace Poseidon
{

class AutoComplete : public IAutoComplete
{
    RString _name;
    FindArray<RStringS> _fixedWords;
    FindArray<RStringS> _userWords;

  public:
    AutoComplete(const RString& name);
    const RString& GetName() const { return _name; }

    // public interface
    RString Guess(RString text, int caret, bool& certain, RString& beg) override;
    void WordDone(RString word) override;
    void AfterChar(RString text, int caret) override;

  private:
    void AddWord(const char* word);
    RString ToComplete(RString text, int caret, bool& certain, RString* beg);
    void AddDictionary(GameState* state);
};

} // namespace Poseidon
namespace Poseidon::Foundation
{
template class Ref<AutoComplete>;
} // namespace Poseidon::Foundation
namespace Poseidon
{

static BankArray<AutoComplete> AutoCompleteBank;

IAutoComplete* CreateAutoComplete(const char* type)
{
    // empty name means no autocomplete
    if (!*type)
    {
        return nullptr;
    }
    return AutoCompleteBank.New(type);
}

AutoComplete::AutoComplete(const RString& name)
{
    _name = name;
    if (!strcmpi(name, "scripting"))
    {
        AddDictionary(&GGameState);
    }
}

void AutoComplete::AddWord(const char* word)
{
    _fixedWords.Add(word);
}

inline bool IsPartOfWord(char c)
{
    // quote marks supposed to be a part of word
    // this makes autocomplete more context sensitive, as it can distinguish
    // between language keywords and often used user strings
    return isalnum(c) || c == '_' || c == '"';
}
static bool WordFilter(const char* word)
{
    char c = *word;
    return isalnum(c) || c == '_';
}

void AutoComplete::AddDictionary(GameState* state)
{
    state->AppendFunctionList(_fixedWords, WordFilter);
    state->AppendNularOpList(_fixedWords, WordFilter);
    state->AppendOperatorList(_fixedWords, WordFilter);
}

static RString GetTypedWord(RString text, int caret)
{
    // typed word ends at caret position
    const char* beg = text;
    const char* endWord = beg + caret;
    const char* begWord = endWord;
    while (begWord > beg)
    {
        char c = begWord[-1];
        if (!IsPartOfWord(c))
        {
            break;
        }
        begWord--;
    }
    // begWord to beg is word part
    return RString(begWord, endWord - begWord);
}

static int CountMatch(const char* s1, const char* s2)
{
    int cnt = 0;
    for (;;)
    {
        char c1 = s1[cnt];
        char c2 = s2[cnt];
        if (c1 == c2 || tolower(c1) == tolower(c2))
        {
            cnt++;
        }
        else
        {
            break;
        }
    }
    if (s2[cnt] != 0)
    {
        return 0;
    }
    return cnt;
}

static RString CommonStart(const char* s1, const char* s2)
{
    int cnt = 0;
    for (;;)
    {
        char c1 = s1[cnt];
        char c2 = s2[cnt];
        if (c1 == c2 || tolower(c1) == tolower(c2))
        {
            cnt++;
        }
        else
        {
            break;
        }
    }
    return RString(s1, cnt);
}

RString AutoComplete::Guess(RString text, int caret, bool& certain, RString& beg)
{
    certain = false;
    RString begWord = GetTypedWord(text, caret);
    beg = begWord;
    // search all dictionaries for any match
    // search user dictionary - use last match
    int bestLen = 0;
    RString bestWord;
    int matches = 0;
    RString longestCommon;
    for (int i = 0; i < _userWords.Size(); i++)
    {
        int m = CountMatch(_userWords[i], begWord);
        if (m >= bestLen)
        {
            RString oldBestWord = bestWord;
            bestWord = _userWords[i];
            if (m > bestLen)
            {
                bestLen = m;
                matches = 0;
                longestCommon = bestWord;
            }
            else
            {
                longestCommon = CommonStart(longestCommon, _userWords[i]);
            }
            if (strcmpi(oldBestWord, _userWords[i]))
            {
                matches++;
            }
        }
    }
    // check if we could find better match in predefined directory
    for (int i = 0; i < _fixedWords.Size(); i++)
    {
        int m = CountMatch(_fixedWords[i], begWord);
        if (m >= bestLen)
        {
            RString oldBestWord = bestWord;
            if (m > bestLen)
            {
                bestLen = m, bestWord = _fixedWords[i];
                matches = 0;
                longestCommon = bestWord;
            }
            else
            {
                longestCommon = CommonStart(longestCommon, _fixedWords[i]);
            }
            if (strcmpi(oldBestWord, _fixedWords[i]))
            {
                matches++;
            }
        }
    }

    if (bestLen < 1)
    {
        return RString();
    }
    if (longestCommon.GetLength() > begWord.GetLength() + 2)
    {
        certain = true;
        return longestCommon;
    }
    if (bestWord.GetLength() < begWord.GetLength() + 2)
    {
        return RString();
    }

    // search this word in fixed dictionary
    bool found = false;
    for (int i = 0; i < _fixedWords.Size(); i++)
    {
        if (!strcmpi(bestWord, _fixedWords[i]))
        {
            bestWord = _fixedWords[i];
            found = true;
            break;
        }
    }
    certain = found && matches < 2;
    return bestWord;
}

void AutoComplete::WordDone(RString word)
{
    if (word.GetLength() <= 0)
    {
        return;
    }
    // always add this word to the end of the list - to mark it as most recent
    int index = _userWords.Find(word);
    if (index >= 0)
    {
        _userWords.DeleteAt(index);
    }
    _userWords.Add(word);
}

void AutoComplete::AfterChar(RString text, int caret)
{
    if (caret < 0 || caret > text.GetLength())
    {
        return;
    }
    // check what user typed
    // if we think he terminated word we can add it to temporary list
    if (caret <= 0)
    {
        return;
    }
    char c = text[caret - 1];
    if (IsPartOfWord(c))
    {
        return; // user typed normal char - continue word
    }
    RString word = GetTypedWord(text, caret - 1);
    if (word.GetLength() <= 0)
    {
        return;
    }
    if (isdigit(word[0]))
    {
        return;
    }
    WordDone(word);
}

} // namespace Poseidon
