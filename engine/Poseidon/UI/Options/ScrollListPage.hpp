#pragma once

// PoseidonOptionsTest — base for any shell page hosting an
// OptionsScrollList (Audio, Display, KB&M, Gamepad).
//
// Mount loads the shared slot bundle (RscOptionsPageScrollList) into
// the shell's notebook and constructs an OptionsScrollList over the
// shell + the page's Provider.  Unmount destroys the list and lets
// the base OptionsPage::Unmount RemoveControl every slot IDC.
//
// Subclasses provide the Provider via ProviderRef() and override
// `TitleText()` / `DefaultFocusIdc()` (typically idc 590 hint or the
// first slot's value cell).
//
// The slot IDC range (500-700, 590, 850) is reused across every
// scroll-list page — only one is mounted at a time.

#include <Poseidon/UI/Options/OptionsPage.hpp>
#include <Poseidon/UI/Options/OptionsScrollList.hpp>

#include <memory>


namespace Poseidon
{
class ScrollListPage : public OptionsPage
{
public:
	const char* ResourceClassName() const override { return "RscOptionsPageScrollList"; }
	int         DefaultFocusIdc()   const override;
	ControllerUiScene GetControllerUiScene() const override
	{
		ControllerUiScene scene = MenuControllerScene();
		scene.activeSection = PagerSection();
		scene.sceneCapabilities = scene.activeSection.capabilities;
		return scene;
	}

	void Mount(OptionsShell& shell) override;
	void Unmount(OptionsShell& shell) override;
	void OnReshown(OptionsShell& shell) override;

	bool OnButtonClicked(OptionsShell& shell, int idc) override;
	bool OnKeyDown(OptionsShell& shell, unsigned nChar) override;
	void OnSimulate(OptionsShell& shell) override;

protected:
	// Subclasses provide the provider object.  Caller owns it; this
	// reference must outlive the OptionsScrollList created by Mount.
	virtual OptionsScrollList::Provider& ProviderRef() = 0;

	OptionsScrollList* List() { return m_list.get(); }

private:
	std::unique_ptr<OptionsScrollList> m_list;
};

} // namespace Poseidon
