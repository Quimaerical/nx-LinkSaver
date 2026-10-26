#pragma once

#include <borealis.hpp>
#include <string>

using namespace std;

class DirectoryManagerView : public brls::Box {
public:
    DirectoryManagerView();
    ~DirectoryManagerView() override = default;

    void refreshList();

private:
    brls::List* list = nullptr;
    brls::Button* addBtn = nullptr;

    void onAddNew();
    void onEditDirectory(const string& tagId);
};
