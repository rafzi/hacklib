#ifndef HACKLIB_CONSOLEEX_H
#define HACKLIB_CONSOLEEX_H

#include <Windows.h>
#include <functional>
#include <future>
#include <string>
#include <thread>


namespace hl
{
struct CONSOLEEX_PARAMETERS
{
    // width of console buffer in characters
    int cellsX = 80;
    // height of visible console buffer in characters
    int cellsYVisible = 20;
    // height of total console buffer in characters
    int cellsYBuffer = 500;
    // background color for output and input child windows
    COLORREF bgColor = RGB(0, 0, 0);
    // text color for output and input child windows
    COLORREF textColor = RGB(192, 192, 192);
    // specifies if the console can be closed via windows shell
    bool closemenu = false;
};


/*
 * A console with simultaneous input and output for Windows.
 */
class ConsoleEx
{
    ConsoleEx(const ConsoleEx&) = delete;
    ConsoleEx& operator=(const ConsoleEx&) = delete;

public:
    // fetches default initialisation of parameter struct
    static CONSOLEEX_PARAMETERS GetDefaultParameters();


public:
    ConsoleEx(HINSTANCE hModule = NULL);
    ~ConsoleEx();

    // opens console and starts handling of all events
    // waits for private thread to run (eg. will deadlock when called in DllMain)
    bool create(const std::string& windowTitle, CONSOLEEX_PARAMETERS* parameters = nullptr);

    // registers a callback that will be called on input
    void registerInputCallback(void (*cbInput)(std::string));
    template <class F>
    void registerInputCallback(F cbInput)
    {
        m_cbInput = cbInput;
    }

    void close();
    bool isOpen() const;
    HWND getWindowHandle() const;

    // member functions to send output to console
    void vprintf(const char* format, va_list valist);
    void printf(const char* format, ...);
    void print(const std::string& str);


private:
    static LRESULT CALLBACK s_WndProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam);
    static LRESULT CALLBACK s_EditOutSubclassProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                                  UINT_PTR subclassId, DWORD_PTR refData);


private:
    // member functions to process window and thread callbacks
    LRESULT wndProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam);
    LRESULT editOutSubclassProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam);
    void threadProc(std::promise<bool>& p);

    // writes arbitrary string
    void writeString(const char* strOut, int len);
    // writes arbitrary string to ConsoleBuffer
    void writeStringToRawBuffer(const char* strOut, int len);

    void updateScrollState(bool checkTrackPos);


    std::thread m_thread;
    HINSTANCE m_hModule = NULL;
    HWND m_hWnd = NULL;
    HWND m_hEditOut = NULL;
    HWND m_hEditIn = NULL;
    HBRUSH m_hbrBackGround = NULL;
    std::string m_title;
    CONSOLEEX_PARAMETERS m_parameters = GetDefaultParameters();
    std::function<void(std::string)> m_cbInput;
    bool m_isOpen = false;
    bool m_isScrolledToBottom = true;
    std::vector<char> m_bufferForScrollLock;
    std::vector<int> m_bufferForScrollLockLengths;


    class RingBuffer
    {
    public:
        void resize(int width, int height)
        {
            m_wid = width;
            m_hei = height;
            m_buffer.resize((width + 2) * height);
        }
        void writeChar(wchar_t c, int pos) { m_buffer[translatePos(pos)] = c; }
        void writeToLine(wchar_t* str, int len, int pos)
        {
            memcpy(&m_buffer[translatePos(pos)], str, len * sizeof(wchar_t));
        }
        void scrollUp()
        {
            if (m_start == (m_wid + 2) * (m_hei - 1))
            {
                m_start = 0;
            }
            else
            {
                m_start += m_wid + 2;
            }
        }
        void copyTo(wchar_t* dst)
        {
            int startBlockSz = (m_wid + 2) * m_hei - m_start;
            memcpy(dst, &m_buffer[m_start], startBlockSz * sizeof(wchar_t));
            // don't overwrite the final null terminator
            memcpy(dst + startBlockSz, &m_buffer[0], (m_start - 2) * sizeof(wchar_t));
        }

    private:
        int translatePos(int pos)
        {
            pos += m_start;
            int sz = (m_wid + 2) * m_hei;
            return pos >= sz ? pos - sz : pos;
        }

        int m_start = 0;
        int m_wid = 0, m_hei = 0;
        std::vector<wchar_t> m_buffer;
    };

    // abstraction of dirty raw buffer modifications
    class ConsoleBuffer
    {
        ConsoleBuffer(const ConsoleBuffer&) = delete;
        ConsoleBuffer& operator=(const ConsoleBuffer&) = delete;

    public:
        ConsoleBuffer() = default;
        ~ConsoleBuffer();

        void free();
        HLOCAL getBuffer() const;
        void setBuffer(HLOCAL buffer);

        void lock();
        void unlock();

        bool resize(int width, int height, int heightVisible);
        void clear();
        void flushBackBuffer();

        // function expects input string to not contain special characters like '\r','\n','\t', etc
        void write(wchar_t* str);
        void scrollUp();

    private:
        int getBufferOffsetFromCursorPos();
        void writeChar(wchar_t c, int pos)
        {
            m_buffer[pos] = c;
            m_backBuffer.writeChar(c, pos);
        }
        void writeToLine(wchar_t* str, int len, int pos)
        {
            memcpy(&m_buffer[pos], str, len * sizeof(wchar_t));
            m_backBuffer.writeToLine(str, len, pos);
        }

        mutable std::mutex m_writeMutex;
        HLOCAL m_bufferHandle = NULL;
        wchar_t* m_buffer = nullptr;
        int m_cursorPos = 0;
        int m_wid = 0, m_hei = 0, m_heiVisible = 0;

        RingBuffer m_backBuffer;

    } m_rawBuffer;
};
}

#endif
