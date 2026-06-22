// Internal helpers shared between ParamFile.cpp and ParamFileParse.cpp.
// Include only inside namespace Poseidon after the file-level includes.
// Never include from a header.
// DEFINE_FAST_ALLOCATOR calls and explicit template instantiations live
// exclusively in ParamFile.cpp; this file only holds definitions that are
// safe to compile in multiple translation units.

using Poseidon::Foundation::FastCAlloc;

typedef char WordBuf[2048];

static bool GetWord(char* buf, int bufSize, QIStream& in, const char* termin, bool* quot = nullptr)
{
    // char buf[2048];
    int len = 0;
    buf[len] = 0;
    int c = in.get();
    // LTrim the word
    while (isspace(c))
    {
        c = in.get();
    }
    if (c == '"')
    {
        if (quot)
        {
            *quot = true;
        }
        c = in.get();
        for (;;)
        {
            if (c == EOF)
            {
                // Unterminated string: stop at end of input rather than spinning here,
                // since QIStream::get keeps returning EOF without advancing the cursor.
                ErrorMessage("Config: End of file encountered inside a string");
                return len > 0;
            }
            if (c == '"')
            {
                c = in.get();
                if (c != '"')
                {
                    in.unget();
                    return true; // word parsed
                }
            }
            if (c == '\n' || c == '\r')
            {
                ErrorMessage("Config: End of line encountered after %s", buf);
            }
            if (len < bufSize - 1)
            {
                buf[len++] = c, buf[len] = 0;
            }
            c = in.get();
        }
    }
    else
    {
        if (quot)
        {
            *quot = false;
        }
        while (!strchr(termin, c) && c != EOF)
        {
            if (c == '\n' || c == '\r')
            {
                // word terminated - only white spaces or termin now
                for (;;)
                {
                    c = in.get();
                    if (!isspace(c))
                    {
                        break;
                    }
                }
                if (!strchr(termin, c))
                {
                    ErrorMessage("Config: '%c' after %s", c, buf);
                }
                else
                {
                    in.unget();
                }
                goto Return;
            }
            if (len < bufSize - 1)
            {
                buf[len++] = c, buf[len] = 0;
            }
            c = in.get();
        }
        if (c != EOF)
        {
            in.unget();
        }
    Return:
        // RTrim the word
        while (len > 0 && isspace(buf[len - 1]))
        {
            buf[--len] = 0;
        }
        return len > 0;
    }
    /*NOTREACHED*/
}

static void GetAlphaWord(char* buf, int bufSize, QIStream& in)
{
    int len = 0;
    int c = in.get();
    while (isspace(c))
    {
        c = in.get();
    }
    while (isalnum(c) || c == '_')
    {
        if (len < bufSize)
        {
            buf[len++] = c;
        }
        c = in.get();
    }
    in.unget();
    buf[len] = 0;
}

// Meyer's singleton for access string constant - no global constructor!
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
static const RStringB& GetAccessString()
{
    static RStringB instance("access");
    return instance;
}
#pragma clang diagnostic pop

// scan some special value types

static int ScanHex(const char* val, bool& ok)
{
    ok = false;
    if (!strnicmp(val, "0x", 2))
    {
        char c;
        const char* ptr = (const char*)val + 2;
        ok = isxdigit(*ptr) != 0;
        if (!ok)
        {
            return 0;
        }
        // unsigned accumulator: an over-long hex literal overflows int (UB); the
        // unsigned wraparound reproduces the original two's-complement result.
        unsigned iValue = 0;
        while (c = *(ptr++), isxdigit(c))
        {
            iValue *= 16u;
            if (isdigit(c))
            { // 0..9
                iValue += c - '0';
            }
            else if (c <= 'F')
            { // A..F
                iValue += 10 + c - 'A';
            }
            else
            { // a..f
                iValue += 10 + c - 'a';
            }
        }
        return (int)iValue;
    }
    else
    {
        return 0;
    }
}

static float ScanDb(const char* ptr, bool& ok)
{
    ok = false;
    if (ptr[0] != 'd' || ptr[1] != 'b')
    {
        return 0;
    }
    ok = true;
    char* end;
    float db = strtod(ptr + 2, &end);
    if (*end != 0)
    {
        LOG_DEBUG(Core, "invalid db value {}", ptr);
    }
    return pow(10, db * (1.0f / 20));
}

static float ScanFloatPlain(const char* ptr, bool& ok)
{
    char* end;
    float db = strtod(ptr, &end);
    ok = (*end == 0);
    return db;
}

static int ScanIntPlain(const char* ptr, bool& ok)
{
    char* end;
    long db = strtol(ptr, &end, 10);
    ok = (*end == 0);
    return (int)db;
}

static int ScanInt(const char* ptr, bool& ok)
{
    ok = false;
    if (!*ptr)
    {
        return 0;
    }
    int val = ScanIntPlain(ptr, ok);
    if (ok)
    {
        return val;
    }
    val = ScanHex(ptr, ok);
    if (ok)
    {
        return val;
    }
    return 0;
}

static float ScanFloat(const char* ptr, bool& ok)
{
    ok = false;
    if (!*ptr)
    {
        return 0;
    }
    float val = ScanFloatPlain(ptr, ok);
    if (ok)
    {
        return val;
    }
    val = ScanDb(ptr, ok);
    if (ok)
    {
        return val;
    }
    return 0;
}

enum SpecValueType
{
    SVGeneric, // generic - string
    SVFloat,
    SVInt,
    SVArray,
    NSpecValueType
    // note: char is used to contain values of this type
};

class ParamRawValue
{
    RStringB _value;
    ParamFile* _file;

  public:
    ParamRawValue() { _file = nullptr; }

    SpecValueType GetValueType() { return SVGeneric; }

    void SetValue(const RStringB& value);
    void SetValue(float val);
    void SetValue(int val);
    void SetFile(ParamFile* file) { _file = file; }

    const RStringB GetValue() const;
    const RStringB GetValueRaw() const;
    float GetFloat() const;
    int GetInt() const;

    operator RStringB() const { return _value; }
    operator float() const { return GetFloat(); }
    operator int() const { return GetInt(); }

    bool IsTextValue() const { return true; }
    bool IsFloatValue() const { return false; }
    bool IsIntValue() const { return false; }

    void Save(QOStream& f, int indent) const;
    void SerializeBin(SerializeBinStream& f);
    void CalculateCheckValue(PASumCalculator& sum) const;
};

class ParamRawValueFloat
{
    float _value;

  public:
    ParamRawValueFloat() = default;

    SpecValueType GetValueType() { return SVFloat; }

    void SetValue(const RStringB& value) { Fail("Float value set as string"); }
    void SetValue(float val) { _value = val; }
    void SetValue(int val) { _value = val; }
    void SetFile(ParamFile* file) {}

    RStringB GetValue() const;
    RStringB GetValueRaw() const { return GetValue(); }

    float GetFloat() const { return _value; }
    int GetInt() const { return toLargeInt(_value); }

    operator RStringB() const { return GetValue(); }
    operator float() const { return GetFloat(); }
    operator int() const { return GetInt(); }

    bool IsTextValue() const { return false; }
    bool IsFloatValue() const { return true; }
    bool IsIntValue() const { return false; }

    void Save(QOStream& f, int indent) const;
    void SerializeBin(SerializeBinStream& f);
    void CalculateCheckValue(PASumCalculator& sum) const;
};

class ParamRawValueInt
{
    int _value;

  public:
    ParamRawValueInt() = default;

    SpecValueType GetValueType() { return SVInt; }

    void SetValue(const RStringB& value) { Fail("Float value set as string"); }
    void SetValue(float val) { _value = toLargeInt(val); }
    void SetValue(int val) { _value = val; }
    void SetFile(ParamFile* file) {}

    RStringB GetValue() const;
    RStringB GetValueRaw() const { return GetValue(); }
    float GetFloat() const { return _value; }
    int GetInt() const { return _value; }

    operator RStringB() const { return GetValue(); }
    operator float() const { return GetFloat(); }
    operator int() const { return GetInt(); }

    bool IsTextValue() const { return false; }
    bool IsFloatValue() const { return false; }
    bool IsIntValue() const { return true; }

    void Save(QOStream& f, int indent) const;
    void SerializeBin(SerializeBinStream& f);
    void CalculateCheckValue(PASumCalculator& sum) const;
};

template <class ParamRawValueSpec>
class ParamValueSpec : public ParamEntry, public ParamRawValueSpec
{
  public:
    ParamValueSpec();
    ParamValueSpec(const RStringB& name);

    RStringB GetValue() const override { return ParamRawValueSpec::GetValue(); }
    RStringB GetValueRaw() const override { return ParamRawValueSpec::GetValueRaw(); }
    float GetFloat() const { return ParamRawValueSpec::GetFloat(); }
    int GetInt() const override { return ParamRawValueSpec::GetInt(); }

    operator RStringB() const override { return ParamRawValueSpec::GetValue(); }
    operator float() const override { return ParamRawValueSpec::GetFloat(); }
    operator int() const override { return ParamRawValueSpec::GetInt(); }
    operator bool() const override { return ParamRawValueSpec::GetInt() != 0; }

    void SetValue(const RStringB& val) override { ParamRawValueSpec::SetValue(val); }
    void SetValue(float val) override { ParamRawValueSpec::SetValue(val); }
    void SetValue(int val) override { ParamRawValueSpec::SetValue(val); }
    void SetFile(ParamFile* file) override { ParamRawValueSpec::SetFile(file); }

    bool IsTextValue() const override { return ParamRawValueSpec::IsTextValue(); }
    bool IsFloatValue() const override { return ParamRawValueSpec::IsFloatValue(); }
    bool IsIntValue() const override { return ParamRawValueSpec::IsIntValue(); }
    bool IsArrayValue() const { return false; }

    void Save(QOStream& f, int indent) const override;
    void SerializeBin(SerializeBinStream& f) override;
    void CalculateCheckValue(PASumCalculator& sum) const override;

    USE_FAST_ALLOCATOR
};

template <class ParamRawValueSpec>
class ParamArrayValueSpec : public IParamArrayValue, public ParamRawValueSpec
{
  public:
    ParamArrayValueSpec(const RStringB& val) { ParamRawValueSpec::SetValue(val); }
    ParamArrayValueSpec(float val) { ParamRawValueSpec::SetValue(val); }
    ParamArrayValueSpec(int val) { ParamRawValueSpec::SetValue(val); }

    RStringB GetValue() const override { return ParamRawValueSpec::GetValue(); }
    RStringB GetValueRaw() const override { return ParamRawValueSpec::GetValueRaw(); }
    int GetInt() const override { return ParamRawValueSpec::GetInt(); }
    float GetFloat() const override { return ParamRawValueSpec::GetFloat(); }

    void SetValue(const RStringB& val) override { ParamRawValueSpec::SetValue(val); }
    void SetValue(float val) override { ParamRawValueSpec::SetValue(val); }
    void SetValue(int val) override { ParamRawValueSpec::SetValue(val); }
    void SetFile(ParamFile* file) override { ParamRawValueSpec::SetFile(file); }

    bool IsTextValue() const override { return ParamRawValueSpec::IsTextValue(); }
    bool IsFloatValue() const override { return ParamRawValueSpec::IsFloatValue(); }
    bool IsIntValue() const override { return ParamRawValueSpec::IsIntValue(); }
    bool IsArrayValue() const override { return false; }

    void Save(QOStream& f, int indent) const override;
    void SerializeBin(SerializeBinStream& f) override;

    const IParamArrayValue* GetItem(int i) const override { return nullptr; }
    int GetItemCount() const override
    {
        ErrorMessage(EMError, "Value not an array.");
        return 0;
    }

    void AddValue(float val) override { ErrorMessage(EMError, "Value not an array."); }
    void AddValue(int val) override { ErrorMessage(EMError, "Value not an array."); }
    void AddValue(const RStringB& val) override { ErrorMessage(EMError, "Value not an array."); }
    IParamArrayValue* AddArrayValue() override
    {
        ErrorMessage(EMError, "Value not an array.");
        return nullptr;
    }

    void CalculateCheckValue(PASumCalculator& sum) const override { ParamRawValueSpec::CalculateCheckValue(sum); }
    USE_FAST_ALLOCATOR
};

typedef ParamValueSpec<ParamRawValue> ParamValue;
typedef ParamValueSpec<ParamRawValueFloat> ParamValueFloat;
typedef ParamValueSpec<ParamRawValueInt> ParamValueInt;

typedef ParamArrayValueSpec<ParamRawValue> ParamArrayValue;
typedef ParamArrayValueSpec<ParamRawValueFloat> ParamArrayValueFloat;
typedef ParamArrayValueSpec<ParamRawValueInt> ParamArrayValueInt;

template <class ParamRawValueSpec>
ParamValueSpec<ParamRawValueSpec>::ParamValueSpec() : ParamEntry(nullptr)
{
}

template <class ParamRawValueSpec>
ParamValueSpec<ParamRawValueSpec>::ParamValueSpec(const RStringB& name) : ParamEntry(name)
{
}

template <class ParamRawValueSpec>
void ParamArrayValueSpec<ParamRawValueSpec>::Save(QOStream& f, int indent) const
{
    ParamRawValueSpec::Save(f, indent);
}

template <class ParamRawValueSpec>
void ParamArrayValueSpec<ParamRawValueSpec>::SerializeBin(SerializeBinStream& f)
{
    if (f.IsSaving())
    {
        char type = ParamRawValueSpec::GetValueType();
        f.Transfer(type);
    }
    ParamRawValueSpec::SerializeBin(f);
}

static ParamEntry* CreateParamValue(SerializeBinStream& f)
{
    PoseidonAssert(f.IsLoading());
    char type;
    f.Transfer(type);
    switch (type)
    {
        case SVGeneric:
            return new ParamValue();
        case SVFloat:
            return new ParamValueFloat();
        case SVInt:
            return new ParamValueInt();
        default:
            LOG_ERROR(Core, "Unknown value type {}", type);
            return new ParamValue();
    }
}
class ParamRawArray
{
  protected:
    RefArray<IParamArrayValue> _value;

  public:
    void AddValue(float val);
    void AddValue(int val);
    // void AddValue(bool val);
    void AddValue(const RStringB& val);
    // void AddValue(const char *val);
    IParamArrayValue* AddArrayValue();

    void Compact() { _value.Compact(); }
    void Clear() { _value.Clear(); }
    void Reserve(int count) { _value.Reserve(count, count); }
    void Copy(const ParamRawArray& src) { _value = src._value; }

    void SetFile(ParamFile* file);

    int GetSize() const { return _value.Size(); }
    IParamArrayValue& GetValue(int i) const;
    const IParamArrayValue& operator[](int i) const { return GetValue(i); }

    void SetValue(int index, const RStringB& string);
    void SetValue(int index, float val);
    /*
        PackedColor GetPackedColor() const;
        Color GetColor() const;
        operator SoundPars() const;
    */

    void Parse(QIStream& in, ParamFile* file);
    void Save(QOStream& f, int indent) const;
    void SerializeBin(SerializeBinStream& f);
    void CalculateCheckValue(PASumCalculator& sum) const;
};

class ParamArrayValueArray : public IParamArrayValue, public ParamRawArray
{
  public:
    ParamArrayValueArray() = default;

    RStringB GetValue() const override { return Foundation::RStringBEmpty; }
    RStringB GetValueRaw() const override { return Foundation::RStringBEmpty; }
    int GetInt() const override { return 0; }
    float GetFloat() const override { return 0; }

    void SetValue(const RStringB& val) override {}
    void SetValue(float val) override {}
    void SetValue(int val) override {}
    void SetFile(ParamFile* file) override { ParamRawArray::SetFile(file); }

    /*
        PackedColor GetPackedColor() const;
        SoundPars GetSoundPars() const;
    */

    bool IsTextValue() const override { return false; }
    bool IsFloatValue() const override { return false; }
    bool IsIntValue() const override { return false; }
    bool IsArrayValue() const override { return true; }

    void Save(QOStream& f, int indent) const override;
    void SerializeBin(SerializeBinStream& f) override;

    // may be array of values
    const IParamArrayValue* GetItem(int i) const override { return &ParamRawArray::operator[](i); }
    int GetItemCount() const override { return ParamRawArray::GetSize(); }

    void AddValue(float val) override { ParamRawArray::AddValue(val); }
    void AddValue(int val) override { ParamRawArray::AddValue(val); }
    void AddValue(const RStringB& val) override { ParamRawArray::AddValue(val); }
    IParamArrayValue* AddArrayValue() override { return ParamRawArray::AddArrayValue(); }

    void CalculateCheckValue(PASumCalculator& sum) const override;

    USE_FAST_ALLOCATOR
};

class ParamArray : public ParamEntry, public ParamRawArray
{
    ParamAccessMode _access;
    // ParamFile *_file;

  public:
    ParamArray(const RStringB& name);

    bool IsArray() const override { return true; }

    // void AddValue(const IParamArrayValue &val);

    void SetFile(ParamFile* file) override { ParamRawArray::SetFile(file); }

    void Compact() override;
    void Clear() override;
    void ReserveArrayElements(int count) override;

    void SetAccessMode(ParamAccessMode mode) override { _access = mode; }
    ParamAccessMode GetAccessMode() const override { return _access; }

    bool EnableModification()
    {
        if (_access >= PAReadOnly)
        {
            RptF("Attempt to modify read-only item %s", (const char*)GetName());
            return false;
        }
        if (_access >= PAReadAndCreate)
        {
            RptF("Attempt to modify add-only item %s", (const char*)GetName());
            return false;
        }
        return true;
    }
    void AddValue(float val) override
    {
        if (!EnableModification())
        {
            return;
        }
        ParamRawArray::AddValue(val);
    }
    void AddValue(int val) override
    {
        if (!EnableModification())
        {
            return;
        }
        ParamRawArray::AddValue(val);
    }
    /*
    void AddValue(bool val)
    {
        if (!EnableModification()) return;
        ParamRawArray::AddValue(val);
    }
    */
    void AddValue(const RStringB& val) override
    {
        if (!EnableModification())
        {
            return;
        }
        ParamRawArray::AddValue(val);
    }
    /*
    void AddValue(const char *val)
    {
        if (!EnableModification()) return;
        ParamRawArray::AddValue(val);
    }
    */
    IParamArrayValue* AddArrayValue() override
    {
        if (!EnableModification())
        {
            return nullptr;
        }
        return ParamRawArray::AddArrayValue();
    }

    int GetSize() const override { return ParamRawArray::GetSize(); }
    using ParamEntry::GetValue; // Bring base class GetValue() into scope
    IParamArrayValue& GetValue(int i) const { return ParamRawArray::GetValue(i); }
    void SetValue(int i, const RStringB& string) override { ParamRawArray::SetValue(i, string); }
    void SetValue(int i, float val) override { ParamRawArray::SetValue(i, val); }
    /*
        PackedColor GetPackedColor() const {return ParamRawArray::GetPackedColor();}
        Color GetColor() const {return ParamRawArray::GetColor();}
        operator SoundPars() const {return ParamRawArray::operator SoundPars();}
    */

    const IParamArrayValue& operator[](int i) const override { return GetValue(i); }

    void Parse(QIStream& in, ParamFile* file);
    void Save(QOStream& f, int indent) const override;

    void SerializeBin(SerializeBinStream& f) override;
    void CalculateCheckValue(PASumCalculator& sum) const override;

    USE_FAST_ALLOCATOR
};
static IParamArrayValue* CreateParamArrayValue(SerializeBinStream& f)
{
    // load type and create value
    PoseidonAssert(f.IsLoading());
    char type;
    f.Transfer(type);
    switch (type)
    {
        case SVGeneric:
            return new ParamArrayValue("");
        case SVFloat:
            return new ParamArrayValueFloat(0.0f);
        case SVInt:
            return new ParamArrayValueInt(0);
        case SVArray:
            return new ParamArrayValueArray();
        default:
            LOG_ERROR(Core, "Unknown value type {}", type);
            return new ParamArrayValue("");
    }
}
