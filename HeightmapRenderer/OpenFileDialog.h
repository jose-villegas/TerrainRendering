#pragma once


class OpenFileDialog
{
    public:
        OpenFileDialog(void);

        TCHAR*DefaultExtension;
        TCHAR*FileName;
        TCHAR*Filter;
        int FilterIndex;
        int Flags;
        TCHAR*InitialDir;
        HWND Owner;
        TCHAR*Title;

        bool ShowDialog();
};

