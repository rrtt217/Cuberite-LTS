// AdvancementTestData.cpp

#include "Globals.h"

#include "AdvancementTestData.h"

#include <chrono>





static Int64 GetCurrentTimeMillis()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()
	).count();
}





cAdvancementUpdate BuildAdvancementTestTree()
{
	cAdvancementUpdate Update;
	Update.Reset = true;

	cAdvancement Root;
	Root.ID = "cuberite:m0/root";
	Root.HasDisplay = true;
	Root.Display.Title = R"({"text":"M0 Root"})";
	Root.Display.Description = R"({"text":"Protocol smoke test"})";
	Root.Display.Icon = cItem(E_ITEM_DIAMOND);
	Root.Display.X = 0;
	Root.Display.Y = 0;
	Root.Criteria.emplace_back("root_criterion");
	Root.Requirements.push_back({ "root_criterion" });
	Update.Advancements.push_back(Root);

	cAdvancement Child;
	Child.ID = "cuberite:m0/child";
	Child.Parent = Root.ID;
	Child.HasDisplay = true;
	Child.Display.Title = R"({"text":"M0 Child"})";
	Child.Display.Description = R"({"text":"A child node"})";
	Child.Display.Icon = cItem(E_ITEM_STICK);
	Child.Display.X = 1;
	Child.Display.Y = 0;
	Child.Criteria.emplace_back("child_criterion");
	Child.Requirements.push_back({ "child_criterion" });
	Update.Advancements.push_back(Child);

	// Mark the root as achieved, so that the client creates the tab for the tree;
	// the child stays in progress so that only the pass-through tree is exercised here:
	cAdvancementProgress RootProgress;
	RootProgress.ID = Root.ID;
	RootProgress.AchievedCriteria["root_criterion"] = GetCurrentTimeMillis();
	Update.Progress.push_back(RootProgress);

	return Update;
}
