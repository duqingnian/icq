#pragma once

#include "../device_monitoring_impl.h"
#include "libudev.h"

namespace device {

class DeviceMonitoringLinux : public DeviceMonitoringImpl
{
    Q_OBJECT
public:
    DeviceMonitoringLinux();
    virtual ~DeviceMonitoringLinux();

    bool Start() override;
    void Stop() override;

private:
    void Stop_internal();
    void OnEvent();
    int GetDescriptor();

    void *_libUdev = nullptr;
    struct udev *(*_udev_new)(void) = nullptr;
    struct udev_device *(*_udev_monitor_receive_device)(struct udev_monitor *udev_monitor) = nullptr;
    int (*_udev_monitor_get_fd)(struct udev_monitor *udev_monitor) = nullptr;
    struct udev_monitor *(*_udev_monitor_new_from_netlink)(struct udev *udev, const char *name) = nullptr;
    int (*_udev_monitor_filter_add_match_subsystem_devtype)(struct udev_monitor *udev_monitor, const char *subsystem, const char *devtype) = nullptr;
    int (*_udev_monitor_enable_receiving)(struct udev_monitor *udev_monitor) = nullptr;
    const char *(*_udev_device_get_subsystem)(struct udev_device *udev_device) = nullptr;
    void (*_udev_monitor_unref)(struct udev_monitor *udev_monitor) = nullptr;
    void (*_udev_device_unref)(struct udev_device *udev_device) = nullptr;
    void (*_udev_unref)(struct udev *udev) = nullptr;

    struct udev *_udev = nullptr;
    struct udev_monitor *_monitor = nullptr;
    bool _registered = false;
    std::unique_ptr<QSocketNotifier> _notifier;
};

}
