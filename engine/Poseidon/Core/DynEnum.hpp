#pragma once

#include <Poseidon/Foundation/Enums/EnumNames.hpp>



namespace Poseidon
{
typedef Foundation::EnumName DynEnumValue;

typedef MapStringToClass<DynEnumValue,AutoArray<DynEnumValue> > DynEnumBankType;

// Case-sensitive dynamic enum.

class DynEnumCS
{
	// quick lookup by name
	DynEnumBankType _values;
	// quick lookup by value
	AutoArray<Foundation::EnumName> _names;

	protected:
	int FindValueString(const char *name) const;
	int AddValueString(const RStringB &name);

	public:
	DynEnumCS();
	~DynEnumCS();

	void Clear();
	// call once all values are added
	void Close();

	int AddValue(const RStringB &name);
	int GetValue(const char *name) const;

	const RStringB &GetName( int value ) const;
	// number of valid items (== first invalid value)
	int FirstInvalidValue() const {return _values.NItems();}
	// list of names, for serialization templates
	const Foundation::EnumName *GetEnumNames() const {return _names.Data();}
};

// Case-insensitive dynamic enum.

class DynEnum: public DynEnumCS
{
	public:
	// replace case sensitive functions with insensitive variants
	int AddValue( const char *name );
	int GetValue( const char *name ) const;
};
} // namespace Poseidon
