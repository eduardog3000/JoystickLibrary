#pragma once
#include "Types.hpp"

namespace JoystickLibrary
{
    typedef std::function<void(DeviceStateChange)> DeviceChangeCallback;

    struct EnumeratorImpl
    {
        // common implementation fields //
        std::map<int, JoystickData> jsMap;

#ifdef _WIN64
        HWND enumerationHwnd;
        HDEVNOTIFY enumerationHNotify;
        HANDLE enumThread;
        LPDIRECTINPUT8 di;

        EnumeratorImpl()
        {
            enumerationHwnd = nullptr;
            enumerationHNotify = nullptr;
            di = nullptr;
            enumThread = nullptr;
        }

        ~EnumeratorImpl()
        {
            if (enumerationHwnd)
                DestroyWindow(enumerationHwnd);
            if (enumerationHNotify)
                UnregisterDeviceNotification(enumerationHNotify);
            if (di)
                di->Release();
            if (enumThread)
                TerminateThread(enumThread, 0);

            enumerationHwnd = nullptr;
            enumerationHNotify = nullptr;
            di = nullptr;
            enumThread = nullptr;
        }
#elif __linux__
        struct udev *udev;
        struct udev_monitor *udev_monitor;
        int udev_mon_fd;
        std::thread deviceListenerThread;
        std::mutex jsMapLock;
        int udev_select_pipe[2];

        EnumeratorImpl()
        {
            udev = nullptr;
            udev_monitor = nullptr;
        }

        ~EnumeratorImpl()
        {
            if (udev)
                udev_unref(udev);
            if (udev_monitor)
                udev_monitor_unref(udev_monitor);
            if (deviceListenerThread.joinable())
            {
                // write some random byte to the pipe to break out of the select loop
                uint8_t zero = 0;
                write(udev_select_pipe[1], &zero, sizeof(uint8_t));
                // join the select loop thread
                deviceListenerThread.join();
                // clean up the pipe
                close(udev_select_pipe[0]);
                close(udev_select_pipe[1]);
            }
        }
#else
        #error Not currently supported!
#endif
    };

    class Enumerator
    {
        friend class JoystickService;
    public:
        static Enumerator& GetInstance()
        {
            static Enumerator instance;
            return instance;
        }

        Enumerator(Enumerator const&) = delete;
        void operator=(Enumerator const&) = delete;
        ~Enumerator();
        bool Start();
        int GetNumberConnected();
        void __run_enum(const void *context = nullptr);
        void __run_remove(const void *context = nullptr);

    private:
        Enumerator();

        void RegisterInstance(DeviceChangeCallback callback);

#ifdef __linux__
        void udev_thread();
        void evdev_thread();
#endif

        EnumeratorImpl *impl;
        std::vector<DeviceChangeCallback> callbacks;
        bool started;
        int connectedJoysticks;
        int nextJoystickID;
    };
}

