#pragma once
#include <Poseidon/Graphics/Rendering/ColorsFloat.hpp>

#include <Poseidon/Foundation/Types/EnumDecl.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/Containers/StaticArray.hpp>
#include <Poseidon/Foundation/Time/Time.hpp>
#include <Poseidon/AI/TargetId.hpp>
namespace Poseidon { class AIUnit; }
namespace Poseidon { class Font; }
using Poseidon::Font;
using Poseidon::Font;
#include <Poseidon/Game/UIActionType.hpp>

namespace Poseidon { class InGameUI; }
using Poseidon::InGameUI;
using Poseidon::TargetId;
using Poseidon::TargetType;

class UIAction
{
	public:
	UIActionType type;
	TargetId target;
	int param;
	int param2;
	RString param3;
	float priority;
	bool showWindow;
	bool hideOnUse;

	RString GetDisplayName(AIUnit *unit) const;
	void Process(AIUnit *unit) const;
};

class UIActions : public StaticArray<UIAction>
{
	typedef StaticArray<UIAction> base;
protected:
	UIAction _selected;

	float _right, _bottom, _w, _h;	// _h is height of one row
	int _rows;										// max rows

	PackedColor _bgColor, _textColor, _selColor;
	Ref<Font> _font;
	float _size;

	Poseidon::Foundation::UITime _dimStart;

public:
	UIActions();

	int Add
	(
		UIActionType type, TargetType *target, float priority,
		int param = 0, bool showWindow = false, bool hideOnUse=true,
		int param2 = 0, RString param3 = ""
	);
	int FindSelected();
	void SelectPrev(bool cycle);
	void SelectNext(bool cycle);
	void ProcessAction(AIUnit *unit);

	void Sort();

	void OnDraw();

	void Refresh(bool user);
	void Hide();
	float GetAge() const;
	float GetAlpha() const;
};

