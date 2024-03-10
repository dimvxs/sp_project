#include "Windows.h"
#include "tchar.h"
#include "resource.h"
#include <vector>
#include <ctime>
#include <CommCtrl.h>
#include <fstream>
using namespace std;

HWND hProgressBar;  // Дескриптор элемента управления ProgressBar
int progressStep;   // Шаг, на который будет увеличиваться ProgressBar
const wchar_t* bannedWords[] = { L"rap", L"weed", L"smoke" };
UINT_PTR timerId = 0;
//int words = 0;
DWORD words = 0;
HWND hDialog2 = NULL;
TCHAR message[256];
HANDLE hThread = nullptr;
HANDLE hThread1 = nullptr;
HANDLE hThread2 = nullptr;
HANDLE hMutex = nullptr; //мьютекс
DWORD bannedWordsCount[sizeof(bannedWords) / sizeof(bannedWords[0])] = { 0 }; //это объявление массива bannedWordsCount, который используется для отслеживания количества вхождений каждого запрещенного слова
vector<wstring> customBannedWords; //вектор для записи слов из IDC_EDIT

INT_PTR CALLBACK DlgProc2(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DlgProc3(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DlgProc4(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


DWORD WINAPI FindThread(LPVOID lp) {
    WaitForSingleObject(hMutex, INFINITE);
    hMutex = OpenMutex(MUTEX_ALL_ACCESS, false, L"{6B31E244-3766-42E6-A998-485DE4B39359}");

 
    // Открываем файл
    FILE* file = nullptr;
    errno_t err = _wfopen_s(&file, L"data.txt", L"r");
    if (err == 0 && file != nullptr)
    {
        // Считываем содержимое файла
        TCHAR buffer[512];
        while (fgetws(buffer, sizeof(buffer) / sizeof(buffer[0]), file) != nullptr)
        {
    
            if (_tcslen(buffer) > 0)
            {
                buffer[_tcslen(buffer) - 1] = L'\0'; // Установка нуль-терминатора

                // Сравнение строк с массивом
                for (size_t i = 0; i < sizeof(bannedWords) / sizeof(bannedWords[0]); i++)
                {
                    if (_tcsncmp(buffer, bannedWords[i], _tcslen(buffer)) == 0)
                    {
                        // Совпадение найдено
                        words++;
                        bannedWordsCount[i]++;

                    }
                
                }      
            }

        }

        // Закрываем файл
        fclose(file);


    }
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
}

DWORD WINAPI CreateCopyThread(LPVOID lp)
{
    HANDLE hMutex = CreateMutex(NULL, FALSE, L"{6B31E244-3766-42E6-A998-485DE4B39359}");
    if (hMutex == nullptr) {
        return 0;  
    }
    WaitForSingleObject(hMutex, INFINITE);

    FILE* originalFile = nullptr;
    errno_t err = _wfopen_s(&originalFile, L"data.txt", L"r");
    if (err != 0 || originalFile == nullptr)
    {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
        
    }

    FILE* newFile = nullptr;
    err = _wfopen_s(&newFile, L"new_data.txt", L"w");
    if (err != 0 || newFile == nullptr)
    {
        fclose(originalFile);
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
       
    }

    wchar_t buffer[512];  // Используем фиксированный буфер

    while (fgetws(buffer, sizeof(buffer) / sizeof(buffer[0]), originalFile) != nullptr)
    {
        // Обработка запрещенных слов
        for (size_t i = 0; i < sizeof(bannedWords) / sizeof(bannedWords[0]); i++)
        {
            const wchar_t* pos = wcsstr(buffer, bannedWords[i]);
            while (pos != NULL)
            {
                size_t index = pos - buffer;
                for (size_t j = 0; j < wcslen(bannedWords[i]); j++)
                {
                    buffer[index + j] = L'*';
                }
                pos = wcsstr(buffer + index + wcslen(bannedWords[i]), bannedWords[i]);
            }
        }

        // Записываем строку в новый файл
        fwprintf(newFile, L"%s", buffer);
    }

    fclose(originalFile);
    fclose(newFile);

    ReleaseMutex(hMutex);  // Освобождаем мьютекс
    CloseHandle(hMutex);

    return 0;
}

DWORD WINAPI ReportThread(LPVOID lp) {
    wofstream reportFile(L"report.txt");
    if (reportFile.is_open())
    {
        reportFile << "Report Information:" << endl;
        reportFile << "-------------------" << endl;
        reportFile << "Number of occurrences of banned words:" << endl;

        for (size_t i = 0; i < sizeof(bannedWords) / sizeof(bannedWords[0]); i++)
        {
            reportFile << bannedWords[i] << ": " << bannedWordsCount[i] << endl;
        }

        reportFile.close();
    }

    return 0;
}


DWORD WINAPI FindThread1(LPVOID lp) {
    hMutex = OpenMutex(MUTEX_ALL_ACCESS, false, L"{6B31E244-3766-42E6-A998-485DE4B39359}");
    vector<wstring>* bannedWordsVector = reinterpret_cast<vector<wstring>*>(lp);
    WaitForSingleObject(hMutex, INFINITE);

    // Открываем файл
    FILE* file = nullptr;
    errno_t err = _wfopen_s(&file, L"data.txt", L"r");
    if (err == 0 && file != nullptr)
    {
        // Считываем содержимое файла
        TCHAR buffer[512];
        while (fgetws(buffer, sizeof(buffer) / sizeof(buffer[0]), file) != nullptr)
        {
            if (_tcslen(buffer) > 0)
            {
                buffer[_tcslen(buffer) - 1] = L'\0';

                for (size_t i = 0; i < sizeof(bannedWords) / sizeof(bannedWords[0]); i++)
                {
                    const wchar_t* pos = buffer;
                    while (wcslen(pos) >= wcslen(bannedWords[i]) && _wcsnicmp(pos, bannedWords[i], wcslen(bannedWords[i])) == 0)
                    {
                        // Слово найдено
                        words++;
                        bannedWordsCount[i]++;
                        pos += wcslen(bannedWords[i]);

                        // Проверяем, если найденное слово полностью совпадает с запрещенным
                        if (pos[0] == L' ' || pos[0] == L'\0')
                            break;
                    }
                }
            }
        }

        // Закрываем файл
        fclose(file);
    }

    ReleaseMutex(hMutex);

    return 0;
}






INT_PTR CALLBACK DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HINSTANCE hInstance = NULL;

    switch (uMsg)
    {
    case WM_INITDIALOG:

      
        break;

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_RADIO1:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                if (IsDlgButtonChecked(hwnd, IDC_RADIO1) == BST_CHECKED)
                {
                    if (IsDlgButtonChecked(hwnd, IDC_BUTTON1) == BST_CHECKED)
                    {
                        DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG2), hwnd, DLGPROC(DlgProc2));
                    }
                }
            }
            break;

        case IDC_BUTTON1: {
            if (HIWORD(wParam) == BN_CLICKED)
            {
                if (IsDlgButtonChecked(hwnd, IDC_RADIO1) == BST_CHECKED)
                {
                    DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG2), hwnd, DLGPROC(DlgProc2));
                }
                else if (IsDlgButtonChecked(hwnd, IDC_RADIO2) == BST_CHECKED)
                {
                    DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG3), hwnd, DLGPROC(DlgProc3));
                }
            }
        
        }
                        break;

        }
    }
    break;


   

    case WM_CLOSE:
        KillTimer(hwnd, timerId);
        EndDialog(hwnd, 0);
        return TRUE;
    }

    return FALSE;
}

INT_PTR CALLBACK DlgProc2(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   

    switch (uMsg)
    {
    case WM_INITDIALOG:
        // Инициализация ProgressBar
        hProgressBar = GetDlgItem(hwnd, IDC_PROGRESS1);
        progressStep = 5; // Рассчитываем шаг ProgressBar
        SendMessage(hProgressBar, PBM_SETRANGE32, 0, 100); // Устанавливаем диапазон от 0 до 60
        SendMessage(hProgressBar, PBM_SETPOS, 0, 0); // Устанавливаем первоначальную точку
        hThread = CreateThread(NULL, 0, FindThread, 0, 0, NULL);
        hThread1 = CreateThread(NULL, 0, CreateCopyThread, 0, 0, NULL);
        hThread2 = CreateThread(NULL, 0, ReportThread, 0, 0, NULL);
        // Создаем таймер с интервалом 100 миллисекунд
        timerId = SetTimer(hwnd, 1, 100, NULL);

        break;


    case WM_COMMAND:{
        bool isPaused = false;
        switch (LOWORD(wParam))
        {
        case IDC_BUTTON1:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                if (isPaused)
                {
                    ResumeThread(hThread);
                    SetWindowText(GetDlgItem(hwnd, IDC_BUTTON1), _T("Пауза"));
                    SetTimer(hwnd, 1, 100, NULL);
                }
                else
                {
                    SuspendThread(hThread);
                    SetWindowText(GetDlgItem(hwnd, IDC_BUTTON1), _T("Возобновить"));
                    KillTimer(hwnd, 1);
                }
                isPaused = !isPaused;
            }
            break;

        case IDC_BUTTON2:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                TerminateThread(hThread, 0);
                EnableWindow(GetDlgItem(hwnd, IDC_BUTTON2), false);
                KillTimer(hwnd, 1);

            }
            break;
        }
    }
        break;
      
        


    case WM_TIMER:
    {
        HWND hEdit1 = GetDlgItem(hwnd, IDC_EDIT1);
        int currentPos = SendMessage(hProgressBar, PBM_GETPOS, 0, 0);
        SendMessage(hProgressBar, PBM_SETPOS, currentPos + progressStep, 0);

     

        if (currentPos + progressStep >= 100)
        {
            KillTimer(hwnd, timerId);
            WaitForSingleObject(hThread, INFINITE);
            CloseHandle(hThread);
          _sntprintf_s(message, _countof(message), _T("Найдено %d запрещенных слов."), words);
                SetWindowText(hEdit1, message);
        }
        break;
    }
       

    case WM_CLOSE:
        EndDialog(hwnd, 0);
        break;
    }
    return FALSE;
}


INT_PTR CALLBACK DlgProc3(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static vector<wstring> customBannedWords;
    switch (uMsg)
    {

    case WM_INITDIALOG: {
 
    }
    break;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_BUTTON1) {
            HWND hEdit = GetDlgItem(hwnd, IDC_EDIT2);
            int textLength = GetWindowTextLength(hEdit);
            wchar_t* buffer = new wchar_t[textLength + 1];
            GetWindowText(hEdit, buffer, textLength + 1);

            // Разбиваем текст на слова и добавляем их в вектор
            wchar_t* nextToken = nullptr;
            wchar_t* token = wcstok_s(buffer, L" ", &nextToken);
            while (token != nullptr)
            {
                customBannedWords.push_back(token);
                token = wcstok_s(nullptr, L" ", &nextToken);
            }

            delete[] buffer;

            // Вызываем DlgProc4, передавая пользовательский вектор запрещенных слов
            DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG4), hwnd, DLGPROC(DlgProc4), reinterpret_cast<LPARAM>(&customBannedWords));
        }
        break;



    case WM_CLOSE:
        EndDialog(hwnd, 0);
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK DlgProc4(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  
   
    switch (uMsg)
    {
    case WM_INITDIALOG: {
       
        // Инициализация ProgressBar
        hProgressBar = GetDlgItem(hwnd, IDC_PROGRESS1);
        progressStep = 5; // Рассчитываем шаг ProgressBar
        SendMessage(hProgressBar, PBM_SETRANGE32, 0, 100); // Устанавливаем диапазон от 0 до 60
        SendMessage(hProgressBar, PBM_SETPOS, 0, 0); // Устанавливаем первоначальную точку
        hThread = CreateThread(NULL, 0, FindThread1, static_cast<LPVOID>(&customBannedWords), 0, NULL);
        hThread1 = CreateThread(NULL, 0, CreateCopyThread, 0, 0, NULL);
        hThread2 = CreateThread(NULL, 0, ReportThread, 0, 0, NULL);
        // Создаем таймер с интервалом 100 миллисекунд
        timerId = SetTimer(hwnd, 1, 100, NULL);
    }
        break;


    case WM_COMMAND: {
        bool isPaused = false;
        switch (LOWORD(wParam))
        {
        case IDC_BUTTON1:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                if (isPaused)
                {
                    ResumeThread(hThread);
                    SetWindowText(GetDlgItem(hwnd, IDC_BUTTON1), _T("Пауза"));
                    SetTimer(hwnd, 1, 100, NULL);
                }
                else
                {
                    SuspendThread(hThread);
                    SetWindowText(GetDlgItem(hwnd, IDC_BUTTON1), _T("Возобновить"));
                    KillTimer(hwnd, 1);
                }
                isPaused = !isPaused;
            }
            break;

        case IDC_BUTTON2:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                TerminateThread(hThread, 0);
                EnableWindow(GetDlgItem(hwnd, IDC_BUTTON2), false);
                KillTimer(hwnd, 1);

            }
            break;
        }
    }
                   break;




    case WM_TIMER:
    {
        HWND hEdit1 = GetDlgItem(hwnd, IDC_EDIT1);
        int currentPos = SendMessage(hProgressBar, PBM_GETPOS, 0, 0);
        SendMessage(hProgressBar, PBM_SETPOS, currentPos + progressStep, 0);



        if (currentPos + progressStep >= 100)
        {
            KillTimer(hwnd, timerId);
            WaitForSingleObject(hThread, INFINITE);
            CloseHandle(hThread);
            _sntprintf_s(message, _countof(message), _T("Найдено %d запрещенных слов."), customBannedWords.size());
            SetWindowText(hEdit1, message);
        }
        break;
    }


    case WM_CLOSE:
        EndDialog(hwnd, 0);
        break;
    }
    return FALSE;
}




int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd)
{
    hMutex = CreateMutex(NULL, FALSE, L"{6B31E244-3766-42E6-A998-485DE4B39359}");


    return DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DLGPROC(DlgProc));
}
