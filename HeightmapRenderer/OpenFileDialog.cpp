#include "Commons.h"
#include "OpenFileDialog.h"


OpenFileDialog::OpenFileDialog()
{
    this->DefaultExtension = 0;
    this->FileName = new TCHAR[MAX_PATH];
    this->Filter = 0;
    this->FilterIndex = 0;
    this->Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    this->InitialDir = 0;
    this->Owner = 0;
    this->Title = 0;
}

bool OpenFileDialog::ShowDialog()
{
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = this->Owner;
    ofn.lpstrDefExt = this->DefaultExtension;
    ofn.lpstrFile = this->FileName;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = this->Filter;
    ofn.nFilterIndex = this->FilterIndex;
    ofn.lpstrInitialDir = this->InitialDir;
    ofn.lpstrTitle = this->Title;
    ofn.Flags = this->Flags;
    GetOpenFileName(&ofn);

    if(_tcslen(this->FileName) == 0) return false;

    return true;
}