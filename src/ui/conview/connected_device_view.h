#pragma once

namespace ConnectedDeviceView {
    void open();
    void close();
    void navigateNext();
    void navigatePrev();
    void selectCurrent();
    void tick();
    bool isOpen();
    void draw();
}
